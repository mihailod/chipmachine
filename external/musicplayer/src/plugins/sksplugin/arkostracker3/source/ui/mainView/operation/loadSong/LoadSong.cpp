#include "LoadSong.h"

#include "../../../../app/preferences/LastFolderTempStorage.h"
#include "../../../../app/preferences/PreferencesManager.h"
#include "../../../../controllers/MainController.h"
#include "../../../../utils/FileExtensions.h"
#include "../../../components/FileChooserCustom.h"
#include "../../../components/dialogs/ErrorReportDialog.h"
#include "../../../import/midi/MidiImportPanel.h"
#include "../../../import/mod/ModImportPanel.h"

namespace arkostracker 
{

LoadSong::LoadSong(MainController& pMainController, Callback& pCallback, bool pSetSongPath) noexcept :
        mainController(pMainController),
        callback(pCallback),
        setSongPath(pSetSongPath),
        dialog(),
        backgroundOperation(),
        fileToLoad(),
        isNativeFormat(false),
        importConfiguration(),
        importPanel(),
        errorReport(),
        song(),
        checkIfSongSavedAndSave()
{
}

void LoadSong::startLoadSongProcess(const juce::String& possiblePathToOpen) noexcept
{
    // Checks whether the current song is modified.
    checkIfSongSavedAndSave = std::make_unique<CheckIfSongSavedAndSave>(mainController, [&, possiblePathToOpen](const bool success) {
        if (success) {
            loadSongAfterSaveCheck(possiblePathToOpen);
        } else {
            callback.onLoadedSongFailure();
        }
    });
    checkIfSongSavedAndSave->perform();
}

void LoadSong::loadSongAfterSaveCheck(const juce::String& possiblePathToOpen) noexcept
{
    // Opens a file picker, unless a file is given.
    if (!possiblePathToOpen.isEmpty()) {
        // The file must exist.
        fileToLoad = juce::File(possiblePathToOpen);
        if (!fileToLoad.existsAsFile()) {
            exitBecauseFileNotFound();
            return;
        }
    } else {
        // Opens the file picker.
        FileChooserCustom fileChooser(juce::translate("Open a song"), FolderContext::loadOrSaveSong, FileExtensions::loadSongExtensionsFilter);
        const auto success = fileChooser.browseForFileToOpen();
        if (!success) {
            callback.onLoadedSongFailure();     // Cancellation from the user.
            return;
        }
        fileToLoad = fileChooser.getResult();
    }

    startImportWithKnownFile();
}

void LoadSong::startImportWithKnownFile() noexcept
{
    // Security: check the file, as this method can be called a second time later if an import panel is shown.
    if (!fileToLoad.existsAsFile()) {
        exitBecauseFileNotFound();
        return;
    }

    // Makes a copy of the configuration, because the SongLoader requires a unique_ptr too. Not copyable!
    const auto localImportConfigurationOptional = (importConfiguration == nullptr) ? OptionalValue<ImportConfiguration>() :
                                                                          OptionalValue(*importConfiguration);
    const auto localFileToLoad = fileToLoad;

    // Starts the import.
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<std::unique_ptr<SongLoader::Result>>>(juce::translate("Loading"),
            juce::translate("Please wait..."),
            [&](std::unique_ptr<SongLoader::Result> result) { onBackgroundOperationFinished(std::move(result)); },
            [localImportConfigurationOptional, localFileToLoad]() -> std::unique_ptr<SongLoader::Result> {
                // Worker thread.
                // This copies the local import configuration.
                auto threadedImportConfiguration = localImportConfigurationOptional.isPresent() ? std::make_unique<ImportConfiguration>(localImportConfigurationOptional.getValue()) :
                                                   std::unique_ptr<ImportConfiguration>();

                SongLoader songLoader;
                auto result = songLoader.loadSong(localFileToLoad, true, std::move(threadedImportConfiguration));       // The configuration is null on first attempt.

                return result;          // When done, the onBackgroundOperationFinished is called.
            });
    backgroundOperation->performOperation();
}

void LoadSong::showErrorDialogAndExit(const bool fileMissing) noexcept
{
    const auto text = fileMissing ? juce::translate("The file could not be found.") : juce::translate("The file is corrupted or not in any known format.");
    jassert(dialog == nullptr);     // Already present?
    dialog = SuccessOrErrorDialog::buildForError(text, [&] {
        dialog.reset();
        callback.onLoadedSongFailure();
    });
}

void LoadSong::onBackgroundOperationFinished(std::unique_ptr<SongLoader::Result> result) noexcept
{
    // Main thread.

    backgroundOperation.reset();

    jassert(result != nullptr);     // Abnormal.

    // Configuration needed?
    if (result->status == SongLoader::ImportStatus::configurationRequired) {
        manageConfigurationNeeded(result->configurationType, result->importReturnData);
        return;
    }

    // No report? Abnormal.
    if (result->errorReport == nullptr) {
        showErrorDialogAndExit(false);
        return;
    }

    // Store the data first. Don't use "result" anymore!
    isNativeFormat = (result->format == ImportedFormat::native);
    errorReport = std::move(result->errorReport);
    song = std::move(result->song);

    // No error/warning in the error report? Then directly notifies the callback.
    if (errorReport->isEmpty()) {
        jassert(song != nullptr);
        exitForSuccess();
        return;
    }

    // Shows the error report. The song can still be valid.
    dialog = std::make_unique<ErrorReportDialog>(*errorReport, [&] {
        dialog.reset();
        afterErrorReportShown();
    });
}

void LoadSong::afterErrorReportShown() noexcept
{
    // If there were errors, not a success.
    if (!errorReport->isOk()) {
        callback.onLoadedSongFailure();
    } else {
        exitForSuccess();
    }
}

void LoadSong::exitForSuccess() noexcept
{
    // Adds the entry to the possibly known ones.
    PreferencesManager::getInstance().addOrUpdateRecentFile(fileToLoad.getFullPathName());
    // Loading a song, even via "recent", updates the load folder. This is already done by the FileChooserCustom, but not in case
    // of "recent", as the picker is not shown. So in this case, it is done twice, but it shouldn't be a problem.
    LastFolderTempStorage::setRecentFolder(FolderContext::loadOrSaveSong, fileToLoad.getParentDirectory());     // Don't take the file itself!

    if (setSongPath) {
        // This loaded Song become the ones to be saved too, but only if native! We don't want imports to "Save" directly, a new file must be picked.
        mainController.setSongPathAndNotify(isNativeFormat ? fileToLoad.getFullPathName() : juce::String());
    }

    callback.onLoadedSongSuccess(std::move(song));
}

void LoadSong::exitBecauseFileNotFound() noexcept
{
    // Removes the entry from the possibly known ones.
    PreferencesManager::getInstance().removeRecentFile(fileToLoad.getFullPathName());

    showErrorDialogAndExit(true);
}

void LoadSong::manageConfigurationNeeded(const ConfigurationType configurationType, const ImportFirstPassReturnData& importFirstPassReturnData) noexcept
{
    jassert(importPanel == nullptr);                // Already shown?

    // Shows the right panel.
    switch (configurationType) {
        default:
        case ConfigurationType::none:
            jassertfalse;           // Abnormal!
            break;
        case ConfigurationType::mod:
        {
            const auto trackCount = importFirstPassReturnData.getTrackCount();
            importPanel = std::make_unique<ModImportPanel>(trackCount,
                                                           [&] { onImportConfigurationPanelOk(); },
                                                           [&] { onImportConfigurationPanelCancel(); });
            break;
        }
        case ConfigurationType::midi:
        {
            const auto trackCount = importFirstPassReturnData.getTrackCount();
            const auto originalSongPpq = importFirstPassReturnData.getMidiSongPpq();
            const auto& trackIndexesWithNotes = importFirstPassReturnData.getMidiTrackIndexesWithNotes();
            importPanel = std::make_unique<MidiImportPanel>(trackCount, originalSongPpq, trackIndexesWithNotes,
                                                            [&] { onImportConfigurationPanelOk(); },
                                                            [&] { onImportConfigurationPanelCancel(); });
            break;
        }
    }
}

void LoadSong::onImportConfigurationPanelOk() noexcept
{
    // Retrieves the configuration from the panel BEFORE removing it!
    importConfiguration = importPanel->getConfiguration();
    jassert(importConfiguration != nullptr);            // We are supposed to know it now!!

    importPanel.reset();

    // Makes another try.
    startImportWithKnownFile();
}

void LoadSong::onImportConfigurationPanelCancel() noexcept
{
    importPanel.reset();
}

}   // namespace arkostracker
