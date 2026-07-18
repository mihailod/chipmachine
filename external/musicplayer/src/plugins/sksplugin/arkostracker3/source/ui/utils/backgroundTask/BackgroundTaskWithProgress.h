#pragma once

#include "../../../utils/message/MessageWithOneParameter.h"
#include "../../../utils/task/Task.h"
#include "../../components/dialogs/ProgressDialog.h"
#include "BackgroundTask.h"

namespace arkostracker 
{

/**
 * Manages a background task, returning a result. It can show a Progress Dialog, depending on the constructor used.
 * There is a difference with a "background operation" in that a Task can be canceled and shows progress.
 * Note that the dialog is deleted only when this object is deleted too.
 *
 * WARNING: do NOT reset (via unique_ptr) this directly when the task is done, else a Progress could still
 * be sent, and the parent won't exist anymore. Use the clear method, then reset after a bit.
 */
template<typename RESULT>
class BackgroundTaskWithProgress final : public TaskProgressListener
{
public:
    /**
     * Constructor, for operation with progress bar.
     * @param pTitle the title of the Dialog.
     * @param pListener the listener, called on the UI thread.
     * @param pTask the task to perform on the background thread.
     */
    BackgroundTaskWithProgress(juce::String pTitle, BackgroundTaskListener<RESULT>& pListener, std::unique_ptr<Task<RESULT>> pTask) noexcept :
            messageListenerToProgressReceived(this),
            showProgressBar(true),
            title(std::move(pTitle)),
            text(),
            progressDialog(),
            backgroundTask(pListener, std::move(pTask))
    {
        backgroundTask.setProgressListener(this);
    }

    /**
     * Constructor, for operation without progress bar.
     * @param pTitle the title of the Dialog.
     * @param pText the text inside the Dialog.
     * @param pListener the listener, called on the UI thread.
     * @param pTask the task to perform on the background thread.
     */
    BackgroundTaskWithProgress(juce::String pTitle, juce::String pText, BackgroundTaskListener<RESULT>& pListener, std::unique_ptr<Task<RESULT>> pTask) noexcept :
            messageListenerToProgressReceived(this),
            showProgressBar(false),
            title(std::move(pTitle)),
            text(std::move(pText)),
            progressDialog(),
            backgroundTask(pListener, std::move(pTask))
    {
        backgroundTask.setProgressListener(this);
    }

    /** Starts the task. Shows a pop-up. */
    void performTask() noexcept
    {
        // This is called on the UI thread.

        // Opens the Progress Dialog.
        jassert(progressDialog == nullptr);
        if (showProgressBar.load()) {
            progressDialog = std::make_unique<ProgressDialog>(title, 100, [&] {
                onCanceledClicked();
            });
        } else {
            progressDialog = std::make_unique<ProgressDialog>(title, text, [&] {
                onCanceledClicked();
            });
        }

        // Let the inner object do to job.
        backgroundTask.performTask();
    }

    /** Call this when the task is done, to make sure no more events will be sent. Use this instead of a reset (do it afterward). */
    void clear() noexcept
    {
        backgroundTask.askForCancel();
        progressDialog.reset();
    }


    // TaskProgressListener method implementations.
    // ===============================================
    void onTaskProgressed(const int progress, const int progressMaximumValue) noexcept override
    {
        // Worker Thread.
        if (!showProgressBar.load()) {
            return;
        }

        auto* message = new MessageWithOneParameter<std::pair<int, int>>({ progress, progressMaximumValue });     // Managed by JUCE.
        messageListenerToProgressReceived.postMessage(message);
    }

private:
    void onCanceledClicked()
    {
        backgroundTask.askForCancel();
    }

    /**
     * Updates the progress. This MUST be called from the UI thread.
     * @param value the value.
     * @param maximumValue the maximum value (100 for example).
     */
    void showProgressPercent(const int value, const int maximumValue) const noexcept
    {
        // UI thread.
        if (progressDialog != nullptr) {
            progressDialog->setProgressMaximumValue(maximumValue);
            progressDialog->setProgress(value);
        }
    }


    /** Implementation of the MessageListener to the result of the threaded task. */
    class MessageListenerToProgressReceived final : public juce::MessageListener,
                                                    public WithParent<BackgroundTaskWithProgress>
    {
    public:
        explicit MessageListenerToProgressReceived(BackgroundTaskWithProgress* parent) :
                WithParent<BackgroundTaskWithProgress>(*parent)
        {
        }

        void handleMessage(const juce::Message& message) override
        {
            const auto* rightMessage = dynamic_cast<const MessageWithOneParameter<std::pair<int, int>>*>(&message);
            if (rightMessage != nullptr) {
                const auto& data = rightMessage->getData();
                this->parentObject.showProgressPercent(data.first, data.second);
            } else {
                jassertfalse;           // Shouldn't happen!
            }
        }
    };
    MessageListenerToProgressReceived messageListenerToProgressReceived;      // Listener to event from the worker thread. Can be safely used on UI thread.

    std::atomic_bool showProgressBar;
    juce::String title;
    juce::String text;

    std::unique_ptr<ProgressDialog> progressDialog;
    BackgroundTask<RESULT> backgroundTask;
};

}   // namespace arkostracker
