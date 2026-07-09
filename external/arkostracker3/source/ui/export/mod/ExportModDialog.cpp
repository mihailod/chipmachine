#include "ExportModDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/mod/ModExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/StringUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/TextEditorUtil.h"

namespace arkostracker
{

ExportModDialog::ExportModDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to MOD"), 440, 200),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& /*nameAndId*/) { /** Nothing to do. */ }),
        minimumDurationSecondsLabel(juce::String(), juce::translate("Minimum duration in seconds for PSG instruments - optional")),
        minimumDurationSecondsEditText(),
        minimumDurationSecondsRestriction(TextEditorUtil::buildRestrictionForInt(2)),
        backgroundTask(),
        fileToSaveTo(),
        dialog(),
        outputStream()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto minimumDurationSecondsEditTextWidth = 40;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    minimumDurationSecondsLabel.setBounds(left, subsongChooser.getBottom() + margins, width, labelsHeight);
    minimumDurationSecondsEditText.setBounds(left, subsongChooser.getBottom() + margins, width, labelsHeight);
    minimumDurationSecondsEditText.setBounds(left, minimumDurationSecondsLabel.getBottom(), minimumDurationSecondsEditTextWidth, labelsHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(minimumDurationSecondsLabel);
    addComponentToModalDialog(minimumDurationSecondsEditText);

    minimumDurationSecondsEditText.setInputFilter(&minimumDurationSecondsRestriction, false);
}

void ExportModDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportModDialog::onDialogExit() noexcept
{
    dialog.reset();

    listener();
}

void ExportModDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::mod, FileExtensions::modExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    outputStream = std::make_unique<juce::FileOutputStream>(fileToSaveTo);

    // Gather data from the UI.
    auto intSuccess = true;
    auto minimumDurationMs = StringUtil::stringToInt(minimumDurationSecondsEditText.getText(), intSuccess) * 1000;
    if (!intSuccess) {
        minimumDurationMs = 0;      // Happens when empty.
    }

    const auto song = songController.getSong();

    // Creates the exporter, and the Task to perform it asynchronously.
    const auto subsongId = subsongChooser.getSelectedSubsongId();
    auto midiExporterTask = std::make_unique<ModExporter>(song, subsongId, minimumDurationMs, *outputStream);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(juce::translate("Exporting..."),
        *this, std::move(midiExporterTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportModDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<bool> result) noexcept
{
    const auto success = (result != nullptr) && *result;
    if (success) {
        outputStream->flush();
    }
    outputStream.reset();

    backgroundTask->clear();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to MOD! Please report this."),
                                                            [&] { onDialogExit(); });
        return;
    }

    if (success) {
        dialog = SuccessOrErrorDialog::buildForSuccess("Export to MOD finished!", [&] { onDialogExit(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForError("Unable to save the MOD file.", [&] { onDialogExit(); });
    }
}

}   // namespace arkostracker
