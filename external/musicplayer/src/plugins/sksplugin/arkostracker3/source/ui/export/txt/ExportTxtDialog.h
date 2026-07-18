#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/backgroundTask/BackgroundTask.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"

namespace arkostracker
{

class ExportTxtDialog : public ExportDialog,
                        public BackgroundTaskListener<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportTxtDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    /** The user exited the Failure Dialog. */
    void onFailureDialogExit() noexcept;

    juce::Label introText;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ModalDialog> failureDialog;
};

}   // namespace arkostracker
