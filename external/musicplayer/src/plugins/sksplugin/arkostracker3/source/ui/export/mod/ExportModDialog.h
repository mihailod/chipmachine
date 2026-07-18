#pragma once

#include "../../components/EditText.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker
{

class MainController;
class SongController;

/** Dialog to export to MOD. */
class ExportModDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportModDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:

    /** The user exited a Dialog. */
    void onDialogExit() noexcept;

    SubsongChooser subsongChooser;
    juce::Label minimumDurationSecondsLabel;
    EditText minimumDurationSecondsEditText;
    juce::TextEditor::LengthAndCharacterRestriction minimumDurationSecondsRestriction;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ModalDialog> dialog;

    std::unique_ptr<juce::OutputStream> outputStream;
};

}   // namespace arkostracker
