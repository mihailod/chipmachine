#pragma once

#include "../../utils/ListItemListener.h"
#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Shows a Dialog to export to YM. */
class ExportYmDialog final : public ExportDialog,
                             public BackgroundTaskListener<std::unique_ptr<juce::MemoryOutputStream>>,         // The export itself.
                             juce::Button::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportYmDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<juce::MemoryOutputStream> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    // juce::Button::Listener method implementations.
    // ======================================================
    void buttonClicked(juce::Button*) override;

    /**
     * Called when the operation finishes.
     * @param success true if success.
     */
    void onBackgroundOperationFinished(bool success) noexcept;

    /** Closes the possible shown Modal Dialog. */
    void closeShownDialog() noexcept;
    /** Exits this dialog by calling the listener. */
    void exit() const noexcept;

    SubsongChooser subsongChooser;

    juce::GroupComponent ymFormatGroup;               // Group for the YM format options.
    juce::ToggleButton ym6ToggleButton;
    juce::ToggleButton ym3ToggleButton;

    juce::GroupComponent optionsGroup;                // Group for the interleaved options.
    juce::ToggleButton interleavedToggleButton;       // The interleaved Toggle Button.
    juce::Label interleavedToggleLabel;               // Text for the interleaved Toggle.
    juce::ToggleButton nonInterleavedToggleButton;    // The non-interleaved Toggle Button.
    juce::Label nonInterleavedToggleLabel;            // Text for the non-interleaved Toggle.

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryOutputStream>>> progressTask;
    std::unique_ptr<BackgroundOperationWithDialog<bool>> saveOperation;

    std::unique_ptr<ModalDialog> dialog;

    juce::File fileToSaveTo;
};

}   // namespace arkostracker
