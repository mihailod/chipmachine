#pragma once

#include <atomic>
#include <utility>

#include "CancelProvider.h"
#include "TaskProgressListener.h"

namespace arkostracker 
{

/**
 * A generic Task, which returns a result, can be canceled, and submit progress.
 * ALL the methods should be called on the same thread. The progress listener will also be called on this same thread.
 * The caller should manage threading by itself (see the BackgroundTaskWithProgress class).
 *
 * The caller must set the ProgressListener to be able to get any progress event. This has been done so because the Task is mostly created without the knowledge
 * of whom the progress listener is yet.
 */
template<typename RESULT>
class Task : public CancelProvider,
             public TaskProgressListener
{
public:
    /** Destructor. */
    ~Task() override = default;

    /** Constructor. */
    Task() :
            canceled(false),
            progressListener(nullptr)           // At first.
    {
    }

    /**
     * Sets the progress listener.
     * @param pProgressListener the listener, or nullptr.
     */
    void setProgressListener(TaskProgressListener* pProgressListener) noexcept
    {
        this->progressListener = pProgressListener;
    }

    /**
     * Performs the task, synchronously.
     * @return how it went, and the result. The bool indicates true if success, false if error or cancel. The "cancel" flag can be checked to make the difference.
     */
    virtual std::pair<bool, RESULT> performTask() noexcept = 0;

    /**
     * Asks the task to be canceled. It may happen immediately, or later, but the "cancel" flag is set directly.
     * The task should call the isCanceled method to know whether to stop.
     * Can be called from any thread.
     */
    void askToCancelTask() noexcept
    {
        canceled = true;
        onAskedToCancelTask();
    }

    /**
     * Called after the cancel flag is set. Implementation may do something. This is useful when encapsulating tasks
     * and wanting to tell the inner tasks to cancel.
     */
    virtual void onAskedToCancelTask() noexcept
    {
        // The default implementation does nothing.
    }

    /**
     * Notifies the possible Progress Listener about progress.
     * @param progress the progress.
     * @param progressMaximum the maximum value of the progress.
     */
    void publishTaskProgress(const int progress, const int progressMaximum) const
    {
        if (progressListener != nullptr) {
            progressListener->onTaskProgressed(progress, progressMaximum);
        }
    }


    // CancelProvider method implementations.
    // ========================================

    /** @return true if the task is canceled. Can be called from any thread. */
    bool isCanceled() const override
    {
        return canceled;
    }

    // TaskProgressListener method implementations.
    // ==============================================
    void onTaskProgressed(const int progress, const int progressMaximumValue) noexcept override
    {
        publishTaskProgress(progress, progressMaximumValue);
    }

private:
    std::atomic_bool canceled;
    TaskProgressListener* progressListener;
};


}   // namespace arkostracker

