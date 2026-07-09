#include "ExportVgmDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/vgm/VgmExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../common/task/SaveStreamToFile.h"

namespace arkostracker 
{

ExportVgmDialog::ExportVgmDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to VGM"), 440, 240),
        subsongChooser(pMainController, [&](const auto& nameAndId) {
            onSubsongChanged(nameAndId.second);
        } ),
        zipOutputToggle(juce::translate("Compress with GZIP (recommended)")),
        warningLabel(),

        progressTask(),
        saveOperation(),
        dialog(),
        fileToSaveTo()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    zipOutputToggle.setBounds(left, subsongChooser.getBottom() + margins, width, labelsHeight);
    zipOutputToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    warningLabel.setBounds(left, zipOutputToggle.getBottom() + margins, width, labelsHeight * 3);
    warningLabel.setJustificationType(juce::Justification::topLeft);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(zipOutputToggle);
    addComponentToModalDialog(warningLabel);
}

void ExportVgmDialog::onSubsongChanged(const Id& subsongId) noexcept
{
    const auto result = VgmExporter::performPreliminaryCheck(*songController.getSong(), subsongId);
    warningLabel.setText(result.second, juce::NotificationType::dontSendNotification);
}

void ExportVgmDialog::onExportButtonClicked() noexcept
{
    const auto compressed = zipOutputToggle.getToggleState();

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::vgm,
                                           compressed ? FileExtensions::vgzExtensionWithoutDot : FileExtensions::vgmExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto song = songController.getSong();

    // Creates the exporter, and the Task to perform it asynchronously.
    auto vgmExporterTask = std::make_unique<VgmExporter>(song, subsongId, compressed);

    progressTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryOutputStream>>>(juce::translate("Exporting..."), *this,
                                                                                                           std::move(vgmExporterTask));
    progressTask->performTask();
}

void ExportVgmDialog::onCancelButtonClicked() const noexcept
{
    exit();
}

void ExportVgmDialog::closeShownDialog() noexcept
{
    dialog.reset();
}

void ExportVgmDialog::exit() const noexcept
{
    listener();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportVgmDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<juce::MemoryOutputStream> result) noexcept
{
    progressTask.reset();

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }
    if ((taskOutputState == TaskOutputState::error) || (result == nullptr)) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to VGM!"), [&] { closeShownDialog(); });
        return;
    }

    const auto localFileToSaveTo = fileToSaveTo;
    const auto dataToSave = result->getMemoryBlock();
    constexpr auto zipped = false;      // Already managed by the exporter.

    saveOperation = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Export"), juce::translate("Saving..."),
                                                                          [&](const bool success) { onBackgroundOperationFinished(success); },
                                                                          [&, localFileToSaveTo, dataToSave]() -> bool {
                                                                              auto inputStream = std::make_unique<juce::MemoryInputStream>(dataToSave, false);        // No need to copy, dataToSave is already a copy.
                                                                              SaveStreamToFile saveStreamToFile(localFileToSaveTo, std::move(inputStream), zipped);
                                                                              return saveStreamToFile.perform();
                                                                          });
    saveOperation->performOperation();
}

void ExportVgmDialog::onBackgroundOperationFinished(const bool success) noexcept
{
    // The saving has ended.
    saveOperation.reset();
    jassert(success);

    if (!success) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to VGM!"), [&] { closeShownDialog(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export to VGM finished!"), [&] { exit(); });
    }
}

}   // namespace arkostracker
