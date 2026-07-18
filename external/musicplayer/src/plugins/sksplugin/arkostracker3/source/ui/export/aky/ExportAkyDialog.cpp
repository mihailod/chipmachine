#include "ExportAkyDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

ExportAkyDialog::ExportAkyDialog(const MainController& pMainController, std::function<void()> pListener) noexcept:
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to AKY"), 400, 570),
        preferences(PreferencesManager::getInstance()),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& nameAndId) { onSelectedSubsongChanged(nameAndId.first, nameAndId.second); }),
        exportAs(false, FileExtensions::akyExtensionWithoutDot),
        encodeAllAddressesAsRelativeToggle(juce::translate("Encode all addresses as relative to the song start (useful for Atari ST player mostly)")),
        backgroundTask(),
        fileToSaveTo(),
        exportAsResult(),
        failureDialog(),
        saveSourceOrBinary(),
        sidPlayerFrequencyGroup(juce::String(), juce::translate("SID player frequency")),
        sidPlayerFrequencyText()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    exportAs.setBounds(left, subsongChooser.getBottom() + margins, width, ExportAs::desiredHeight);
    encodeAllAddressesAsRelativeToggle.setBounds(left, exportAs.getBottom(), width, labelsHeight * 2);

    sidPlayerFrequencyGroup.setBounds(left, encodeAllAddressesAsRelativeToggle.getBottom() + margins, width, 52);
    sidPlayerFrequencyText.setBounds(sidPlayerFrequencyGroup.getX() + LookAndFeelConstants::groupMarginsX, sidPlayerFrequencyGroup.getY() + LookAndFeelConstants::groupMarginsY,
        sidPlayerFrequencyGroup.getWidth() - (2 * LookAndFeelConstants::groupMarginsY) - margins, labelsHeight);

    encodeAllAddressesAsRelativeToggle.setToggleState(preferences.isEncodeAllAddressesAsRelativeToSongStart(), juce::NotificationType::dontSendNotification);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(exportAs);
    addComponentToModalDialog(encodeAllAddressesAsRelativeToggle);
    addComponentToModalDialog(sidPlayerFrequencyGroup);
    addComponentToModalDialog(sidPlayerFrequencyText);
}

void ExportAkyDialog::onSelectedSubsongChanged(const juce::String& subsongName, const Id& subsongId) noexcept
{
    exportAs.onSubsongChanged(subsongName);

    juce::String sidPlayerCapabilityText;
    mainController.getSongController().getSong()->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        sidPlayerCapabilityText = subsong.getSidPlayerCapability().toDisplayedString();
    });
    sidPlayerFrequencyText.setText(sidPlayerCapabilityText, juce::NotificationType::dontSendNotification);
}

void ExportAkyDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportAkyDialog::onExportButtonClicked() noexcept
{
    // Saves the data from the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAs.storeChanges());
    const auto encodeAllAddressesAsRelativeToSongStart = encodeAllAddressesAsRelativeToggle.getToggleState();
    preferences.setEncodeAllAddressesAsRelativeToSongStart(encodeAllAddressesAsRelativeToSongStart);

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::aky, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const ExportConfiguration exportConfiguration(sourceConfiguration, { subsongId }, baseLabel, address, SampleEncoderFlags::buildNoExport(),
                                            encodeAllAddressesAsRelativeToSongStart);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto akyExporterTask = std::make_unique<AkyExporter>(song, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(akyExporterTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportAkyDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to AKY! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    const auto& playerConfiguration = result->getPlayerConfigurationRef();

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto exportPlayerConfiguration = exportAsResult->getExportConfigurationFile();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(),
                                                              fileToSaveTo, false, saveToBinary,
                                                              exportPlayerConfiguration, playerConfiguration,
                                                              sourceConfiguration,
                                                              juce::translate("Export to AKY finished successfully!"),
                                                              juce::translate("Export to AKY failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) { onSaveSourceDialogOkClicked(success); });
    saveSourceOrBinary->perform();
}

void ExportAkyDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportAkyDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

}   // namespace arkostracker
