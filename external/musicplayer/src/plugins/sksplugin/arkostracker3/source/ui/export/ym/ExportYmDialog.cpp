#include "ExportYmDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/ym/YmExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../common/task/SaveStreamToFile.h"

namespace arkostracker 
{

ExportYmDialog::ExportYmDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to YM"), 460, 430),
        subsongChooser(pMainController, nullptr),

        ymFormatGroup(juce::String(), juce::translate("YM format")),
        ym6ToggleButton(juce::translate("YM6 (recommended)")),
        ym3ToggleButton(juce::translate("YM3")),

        optionsGroup(juce::String(), juce::translate("Encoding options")),
        interleavedToggleButton(juce::translate("Encode register by register (recommended)")),
        interleavedToggleLabel(juce::String(),
                               juce::translate("Each register stream is encoded one after the other. The compression ratio is usually higher. Called \"interleaved\" in YM format terminology.")),
        nonInterleavedToggleButton(juce::translate("Encode frame by frame")),
        nonInterleavedToggleLabel(juce::String(),
                                  juce::translate("Each frame is encoded one after the other. Some converters may require this format. Called \"non-interleaved\" in YM format terminology. YM3 cannot use this.")),

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
    const auto groupMarginX = LookAndFeelConstants::groupMarginsX;
    const auto groupMarginY = LookAndFeelConstants::groupMarginsY;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);

    // The YM format inside a group.
    constexpr auto ymRadioButtonId = 7788;     // Why not?
    const auto ymToggleButtonWidth = (width - (groupMarginX * 2)) / 2;
    ymFormatGroup.setBounds(left, subsongChooser.getBottom() + (margins * 2), width, 50);
    ym6ToggleButton.setBounds(ymFormatGroup.getX() + groupMarginX, ymFormatGroup.getY() + groupMarginY, ymToggleButtonWidth, labelsHeight);
    ym3ToggleButton.setBounds(ym6ToggleButton.getRight() + margins, ym6ToggleButton.getY(), ymToggleButtonWidth, labelsHeight);

    // The options inside a group.
    constexpr auto interleaveRadioButtonId = ymRadioButtonId + 1;
    constexpr auto interleavedLabelHeight = 60;
    optionsGroup.setBounds(left, ymFormatGroup.getBottom() + (margins * 2), width, 206);
    interleavedToggleButton.setBounds(optionsGroup.getX() + groupMarginX, optionsGroup.getY() + groupMarginY, optionsGroup.getWidth() - (2 * groupMarginX),
                                      labelsHeight);
    interleavedToggleLabel.setBounds(interleavedToggleButton.getX() + margins, interleavedToggleButton.getBottom(), interleavedToggleButton.getWidth() - margins,
                                     interleavedLabelHeight);
    nonInterleavedToggleButton.setBounds(interleavedToggleButton.getX(), interleavedToggleLabel.getBottom() + margins, interleavedToggleLabel.getWidth(),
                                         interleavedToggleButton.getHeight());
    nonInterleavedToggleLabel.setBounds(interleavedToggleLabel.getX(), nonInterleavedToggleButton.getBottom(), interleavedToggleLabel.getWidth(), interleavedLabelHeight);
    interleavedToggleButton.setRadioGroupId(interleaveRadioButtonId, juce::NotificationType::dontSendNotification);
    nonInterleavedToggleButton.setRadioGroupId(interleaveRadioButtonId, juce::NotificationType::dontSendNotification);
    interleavedToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    ym6ToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    ym6ToggleButton.setRadioGroupId(ymRadioButtonId, juce::NotificationType::dontSendNotification);
    ym3ToggleButton.setRadioGroupId(ymRadioButtonId, juce::NotificationType::dontSendNotification);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(ymFormatGroup);
    addComponentToModalDialog(ym6ToggleButton);
    addComponentToModalDialog(ym3ToggleButton);
    addComponentToModalDialog(interleavedToggleButton);
    addComponentToModalDialog(interleavedToggleLabel);
    addComponentToModalDialog(nonInterleavedToggleButton);
    addComponentToModalDialog(nonInterleavedToggleLabel);
    addComponentToModalDialog(optionsGroup);

    ym6ToggleButton.addListener(this);
    ym3ToggleButton.addListener(this);
    interleavedToggleButton.addListener(this);
    nonInterleavedToggleButton.addListener(this);
}

void ExportYmDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::ym, FileExtensions::ymExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto song = songController.getSong();
    constexpr auto psgIndex = 0;            // Automatically exports the first PSG.
    const auto interleaved = interleavedToggleButton.getToggleState();
    const auto isYm3 = ym3ToggleButton.getToggleState();

    // Creates the exporter, and the Task to perform it asynchronously.
    auto ymExporterTask = std::make_unique<YmExporter>(song, subsongId, psgIndex, interleaved, isYm3);

    progressTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryOutputStream>>>(
        juce::translate("Exporting"), juce::translate("Please wait..."), *this, std::move(ymExporterTask));
    progressTask->performTask();
}

void ExportYmDialog::onCancelButtonClicked() const noexcept
{
    exit();
}

void ExportYmDialog::exit() const noexcept
{
    listener();
}


// juce::Button::Listener method implementations.
// ======================================================

void ExportYmDialog::buttonClicked(juce::Button* button)
{
    // Sanity checks about the format.
    if (button == &ym3ToggleButton) {
        interleavedToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (button == &nonInterleavedToggleButton) {
        ym6ToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    }
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportYmDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<juce::MemoryOutputStream> result) noexcept
{
    progressTask->clear();

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }
    if ((taskOutputState == TaskOutputState::error) || (result == nullptr)) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to YM!"), [&] { closeShownDialog(); });
        return;
    }

    // Saves and zips if wanted.
    const auto localFileToSaveTo = fileToSaveTo;
    const auto dataToSave = result->getMemoryBlock();

    saveOperation = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Export"), juce::translate("Saving..."),
            [&](const bool success) { onBackgroundOperationFinished(success); },
            [&, localFileToSaveTo, dataToSave]() -> bool {
                auto inputStream = std::make_unique<juce::MemoryInputStream>(dataToSave, false);        // No need to copy, dataToSave is already a copy.
                SaveStreamToFile saveStreamToFile(localFileToSaveTo, std::move(inputStream), false);    // YM are never zipped.
                return saveStreamToFile.perform();
            });
    saveOperation->performOperation();
}

void ExportYmDialog::onBackgroundOperationFinished(const bool success) noexcept
{
    // The saving has ended.
    saveOperation.reset();
    jassert(success);

    if (!success) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to YM!"), [&] { closeShownDialog(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export to YM finished!"), [&] { exit(); });
    }
}


// ======================================================

void ExportYmDialog::closeShownDialog() noexcept
{
    dialog.reset();
}

}   // namespace arkostracker
