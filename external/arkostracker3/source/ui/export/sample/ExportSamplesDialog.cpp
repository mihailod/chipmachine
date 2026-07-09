#include "ExportSamplesDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/events/EventsExporter.h"
#include "../../../export/samples/SampleExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../utils/TextEditorUtil.h"

namespace arkostracker
{

ExportSamplesDialog::ExportSamplesDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Export samples"), 430, 675,
                    [&] { onExportButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        songController(pMainController.getSongController()),
        preferences(PreferencesManager::getInstance()),
        listener(std::move(pListener)),
        sampleExport(false, true),
        pitchLabel(juce::String(), juce::translate("Additional pitch in semi-tones (negative and decimal possible)")),
        pitchEditText(),
        exportAs(false, juce::String(), false),
        restrictionPitch(TextEditorUtil::buildRestrictionForDecimal(6)),
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
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    sampleExport.setBounds(left, top, width, SampleExport::componentDesiredHeightIgnoreUnusedAndGenerateIndexPresent);
    pitchLabel.setBounds(left, sampleExport.getBottom() + margins, width, labelsHeight);
    pitchEditText.setBounds(left, pitchLabel.getBottom(), 70, labelsHeight);
    exportAs.setBounds(left, pitchEditText.getBottom() + margins * 2, width, ExportAs::desiredHeight);

    addComponentToModalDialog(sampleExport);
    addComponentToModalDialog(pitchLabel);
    addComponentToModalDialog(pitchEditText);
    addComponentToModalDialog(exportAs);

    pitchEditText.setInputFilter(&restrictionPitch, false);

    // Sets the Sample UI part from the Preferences.
    const auto sampleEncoderFlags = preferences.getSampleEncoderFlags();
    sampleExport.setValuesAndRefreshUi(sampleEncoderFlags);
    const auto pitch = preferences.getSampleExportPitch();
    pitchEditText.setText(juce::String(pitch));
}

void ExportSamplesDialog::onExportButtonClicked() noexcept
{
    // Saves the data from the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAs.storeChanges());

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::samples, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    // Saves the preferences of the samples.
    const auto sampleEncoderFlags = sampleExport.getSampleEncoderFlags();
    preferences.setSampleEncoderFlags(sampleEncoderFlags);

    const auto pitch =  pitchEditText.getText().getDoubleValue();
    preferences.storeSampleExportPitch(pitch);
    const auto subsongId = songController.getCurrentSubsongId();        // Does not matter.
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const ExportConfiguration exportConfiguration(sourceConfiguration, { subsongId }, baseLabel, address, sampleEncoderFlags);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporter = std::make_unique<SampleExporter>(*song, exportConfiguration, pitch);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(exporter));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportSamplesDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to export samples! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(),
                                                              fileToSaveTo, false, saveToBinary,
                                                              false, PlayerConfiguration(),
                                                              sourceConfiguration,
                                                              juce::translate("Export samples finished successfully!"),
                                                              juce::translate("Export samples failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) {
                                                                  onSaveSourceDialogOkClicked(success);
                                                              });
    saveSourceOrBinary->perform();
}

// ======================================================


void ExportSamplesDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportSamplesDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportSamplesDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

}   // namespace arkostracker
