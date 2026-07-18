#include "SaveSong.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/at3/SongExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

SaveSong::SaveSong(const bool pForceAskForPath, MainController& pMainController, std::function<void(bool)> pCallback) noexcept :
        forceAskForPath(pForceAskForPath),
        mainController(pMainController),
        song(pMainController.getSongController().getSong()),
        compressXml(PreferencesManager::getInstance().areSavedFilesCompressed()),
        callback(std::move(pCallback)),
        dialog(),
        backgroundOperation(),
        fileToSaveTo()
{
}

void SaveSong::saveSong() noexcept
{
    // Do we have a path to save to? If not, asks the user.
    const auto songPath = mainController.getSongPath();
    if (!forceAskForPath && songPath.isNotEmpty()) {
        fileToSaveTo = juce::File(songPath);
    } else {
        // Opens the file picker.
        FileChooserCustom fileChooser(juce::translate("Save the song"), FolderContext::loadOrSaveSong, FileExtensions::saveSongExtensionFilter);
        const auto success = fileChooser.browseForFileToSave(true);
        if (!success) {
            callback(false);     // Cancellation from the user.
            return;
        }
        fileToSaveTo = fileChooser.getResultWithExtensionIfNone(FileExtensions::saveSongExtension);
    }

    song->updateModificationDate();

    // Saves the song asynchronously.
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Saving"),
            juce::translate("Please wait..."),
            [&](const bool success) { onBackgroundOperationFinished(success); },
            [=]() -> bool {
                // Worker thread.
                const auto xmlElement = SongExporter::exportSong(*song);
                if (xmlElement == nullptr) {
                    jassertfalse;       // Unable to create the XML Node??
                    return false;
                }

                // Zip.
                return FileUtil::writeXmlToFile(*xmlElement, fileToSaveTo, compressXml);          // When done, the onBackgroundOperationFinished is called.
            });
    backgroundOperation->performOperation();
}

void SaveSong::onBackgroundOperationFinished(const bool success) noexcept
{
    // Main thread.

    if (!success) {
        jassertfalse;       // Shouldn't happen.
        jassert(dialog == nullptr);     // Already present?
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred when saving the file."), [&] {
            dialog.reset();
            callback(false);
        });
    } else {
        // Success. Marks as save, and puts the song in the recent files.
        mainController.markSongAsSaved();

        const auto path = fileToSaveTo.getFullPathName();
        jassert(path.isNotEmpty());
        mainController.setSongPathAndNotify(path);
        PreferencesManager::getInstance().addOrUpdateRecentFile(path);

        callback(true);
    }
}

}   // namespace arkostracker
