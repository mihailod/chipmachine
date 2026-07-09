#include "ExportAkmDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/akm/AkmExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

ExportAkmDialog::ExportAkmDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to AKM"), 440, 506),
        subsongsChooser(*pMainController.getSongController().getSong(), [&](const std::vector<Id>& /*subsongIds*/) {
            // Nothing to do.
        }),
        exportAsDialog(true, FileExtensions::akmExtensionWithoutDot),
        backgroundTask(),
        fileToSaveTo(),
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

    addComponentToModalDialog(subsongsChooser);
    addComponentToModalDialog(exportAsDialog);
}

void ExportAkmDialog::onExportButtonClicked() noexcept
{
    // Stores the data of the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAsDialog.storeChanges());

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::akm, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const auto subsongIds = subsongsChooser.getSelectedSubsongIds();
    const ExportConfiguration exportConfiguration(sourceConfiguration, subsongIds, baseLabel, address);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto akmExporterTask = std::make_unique<AkmExporter>(*song, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."),
                                                                                     *this, std::move(akmExporterTask));
    backgroundTask->performTask();
}

void ExportAkmDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportAkmDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportAkmDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportAkmDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if ((taskOutputState != TaskOutputState::finished)) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to AKM! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    const auto& playerConfiguration = result->getPlayerConfigurationRef();

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto exportPlayerConfiguration = exportAsResult->getExportConfigurationFile();
    const auto saveToSeveralFiles = exportAsResult->isExportToSeveralFiles();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(),
                                                              result->getSubsongData(),
                                                              fileToSaveTo, saveToSeveralFiles, saveToBinary,
                                                              exportPlayerConfiguration, playerConfiguration, sourceConfiguration,
                                                              juce::translate("Export to AKM finished successfully!"), juce::translate("Export to AKM failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) { onSaveSourceDialogOkClicked(success); });
    saveSourceOrBinary->perform();
}

}   // namespace arkostracker
