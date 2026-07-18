#pragma once

#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Shows a Dialog to export to VGM. */
class ExportVgmDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<juce::MemoryOutputStream>>         // The export itself.
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportVgmDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<juce::MemoryOutputStream> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    /**
     * Called when the operation finishes.
     * @param success true if success.
     */
    void onBackgroundOperationFinished(bool success) noexcept;

    /** Closes the possible shown Modal Dialog. */
    void closeShownDialog() noexcept;
    /** Exits this dialog by calling the listener. */
    void exit() const noexcept;

    /**
     * Called when the Subsong changed in the dropdown.
     * @param subsongId the ID of the newly selected Subsong.
     */
    void onSubsongChanged(const Id& subsongId) noexcept;

    SubsongChooser subsongChooser;
    juce::ToggleButton zipOutputToggle;
    juce::Label warningLabel;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryOutputStream>>> progressTask;
    std::unique_ptr<BackgroundOperationWithDialog<bool>> saveOperation;

    std::unique_ptr<ModalDialog> dialog;

    juce::File fileToSaveTo;
};

}   // namespace arkostracker
