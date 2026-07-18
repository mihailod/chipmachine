#include "ExportTxtDialog.h"

#include "../../../export/txt/SongTxtExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{
ExportTxtDialog::ExportTxtDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to text"), 500, 120),
        introText(juce::String(), juce::translate("All subsongs will be exported.")),
        backgroundTask(),
        fileToSaveTo(),
        failureDialog()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto left = bounds.getX();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    introText.setBounds(left, top, width, labelsHeight);

    addComponentToModalDialog(introText);
}

void ExportTxtDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::txt, FileExtensions::txtExtensionWithWildcard);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    (void)fileToSaveTo.deleteFile();
    auto fileOutputStream = std::make_unique<juce::FileOutputStream>(fileToSaveTo);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporter = std::make_unique<SongTxtExporter>(*mainController.getSongController().getSong(), std::move(fileOutputStream));

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(exporter));
    backgroundTask->performTask();
}

void ExportTxtDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportTxtDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportTxtDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<bool> result) noexcept
{
    backgroundTask->clear();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if ((taskOutputState != TaskOutputState::finished) || (result == nullptr) || !*result) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to TXT! Please report this."),
                                                            [&] { onFailureDialogExit(); });
    } else {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Export to TXT successful!"),
                                                            [&] { listener(); });
    }
}

}   // namespace arkostracker
