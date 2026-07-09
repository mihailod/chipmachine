#include "ExportAkgDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/akg/AkgExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

ExportAkgDialog::ExportAkgDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to AKG"), 440, 680),
        preferences(PreferencesManager::getInstance()),
        subsongsChooser(*pMainController.getSongController().getSong(), [&] (const std::vector<Id>& /*subsongIds*/) {
            // Nothing to do.
        }),
        exportAsDialog(true, FileExtensions::akgExtensionWithoutDot),
        backgroundTask(),
        fileToSaveTo(),
        sampleExport(),
        exportAsResult(),
        failureDialog(),
        saveSourceOrBinary()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;

    subsongsChooser.setBounds(left, top, width, SubsongsChooser::desiredHeight);
    exportAsDialog.setBounds(left, subsongsChooser.getBottom() + margins, width, ExportAs::desiredHeight);
    sampleExport.setBounds(left, exportAsDialog.getBottom() + margins, width, SampleExport::componentDesiredHeight);

    addComponentToModalDialog(subsongsChooser);
    addComponentToModalDialog(exportAsDialog);
    addComponentToModalDialog(sampleExport);

    // Sets the Sample UI part from the Preferences.
    const auto sampleEncoderFlags = preferences.getSampleEncoderFlags();
    sampleExport.setValuesAndRefreshUi(sampleEncoderFlags);
}

void ExportAkgDialog::onExportButtonClicked() noexcept
{
    // Stores the data of the UI.
    const auto sampleEncoderFlags = sampleExport.getSampleEncoderFlags();
    preferences.setSampleEncoderFlags(sampleEncoderFlags);

    exportAsResult = std::make_unique<ExportAs::Result>(exportAsDialog.storeChanges());

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::akg, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const auto subsongIds = subsongsChooser.getSelectedSubsongIds();
    const ExportConfiguration exportConfiguration(sourceConfiguration, subsongIds, baseLabel, address, sampleEncoderFlags);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto akgExporterTask = std::make_unique<AkgExporter>(*song, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this, std::move(akgExporterTask));
    backgroundTask->performTask();
}

void ExportAkgDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportAkgDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportAkgDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportAkgDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if ((taskOutputState != TaskOutputState::finished)) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to AKG! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    const auto& playerConfiguration = result->getPlayerConfigurationRef();

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto exportPlayerConfiguration = exportAsResult->getExportConfigurationFile();
    const auto exportToSeveralFiles = exportAsResult->isExportToSeveralFiles();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(), fileToSaveTo,
                                                              exportToSeveralFiles, saveToBinary, exportPlayerConfiguration, playerConfiguration,
                                                              sourceConfiguration,
                                                              juce::translate("Export to AKG finished successfully!"), juce::translate("Export to AKG failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) { onSaveSourceDialogOkClicked(success); });
    saveSourceOrBinary->perform();
}

}   // namespace arkostracker
