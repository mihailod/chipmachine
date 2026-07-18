#pragma once

#include <future>

#include <juce_events/juce_events.h>

#include "../../../utils/WithParent.h"
#include "../../../utils/task/Task.h"

namespace arkostracker 
{

/** How a task finished. */
enum class TaskOutputState : uint8_t
{
    finished,
    error,
    canceled
};

/** Listener to the task progress and result. */
template<typename RESULT>
class BackgroundTaskListener
{
public:
    /** Destructor. */
    virtual ~BackgroundTaskListener() = default;

    /**
     * The task has finished. This is called from the UI thread.
     * @param taskOutputState how the operation went.
     * @param result the result, if any.
     */
    virtual void onBackgroundTaskFinished(TaskOutputState taskOutputState, RESULT result) noexcept = 0;
};


/** A background task, performed asynchronously. It handles progress and cancellation. */
template<typename RESULT>
class BackgroundTask
{
public:
    /**
     * Constructor.
     * @param pListener the listener, called on the UI thread.
     * @param pTask the task to be performed on the WORKER thread.
     */
    BackgroundTask(BackgroundTaskListener<RESULT>& pListener, std::unique_ptr<Task<RESULT>> pTask) noexcept :
            messageListenerToTaskFinished(this),
            task(std::move(pTask)),
            taskFuture(),
            listener(pListener)
    {
    }

    /** Starts the task. Call this on the UI thread. */
    void performTask() noexcept
    {
        // UI thread.

        jassert(task != nullptr);

        // Performs the task asynchronously.
        taskFuture = std::async(std::launch::async, [&]() -> std::pair<bool, RESULT> {
            // WORKER thread.

            // Performs the task.
            auto resultPair = task->performTask();

            // Done, then notifies the UI thread. It will have to "wait" for the result, returned just below. This should be instantaneous.
            auto* message = new juce::Message();        // Reference counted object.
            messageListenerToTaskFinished.postMessage(message);

            return resultPair;
        });
    }

    /** Asks for the task to cancel. */
    void askForCancel() noexcept
    {
        // UI thread.

        task->askToCancelTask();
    }

    /**
     * Sets the progress listener.
     * @param progressListener the listener, or nullptr.
     */
    void setProgressListener(TaskProgressListener* progressListener) noexcept
    {
        task->setProgressListener(progressListener);
    }

private:
    /** Called when the task is finished. */
    void onFinishedTask() noexcept
    {
        // UI Thread.

        // Gets the result. This is blocking, but the result is already present when the current method is called, so it should be instantaneous.
        auto resultPair = taskFuture.get();

        // Checks the Cancel first. It supersedes the Success.
        auto taskOutputState = TaskOutputState::canceled;
        const auto canceled = task->isCanceled();
        const auto success = resultPair.first;
        if (!canceled) {
            taskOutputState = success ? TaskOutputState::finished : TaskOutputState::error;
        }

        // Keeps the result only if success and not canceled.
        listener.onBackgroundTaskFinished(taskOutputState, success && !canceled ? std::move(resultPair.second) : nullptr);
    }

    /** Implementation of the MessageListener to the result of the threaded task. */
    class MessageListenerToTaskFinished final : public juce::MessageListener,
                                                public WithParent<BackgroundTask>
    {
    public:
        explicit MessageListenerToTaskFinished(BackgroundTask* parent) :
                WithParent<BackgroundTask>(*parent)
        {
        }
        void handleMessage(const juce::Message& /*message*/) override
        {
            this->parentObject.onFinishedTask();
        }
    };
    MessageListenerToTaskFinished messageListenerToTaskFinished;      // Listener to events from the worker thread. Can be safely used on UI thread.

    std::unique_ptr<Task<RESULT>> task;
    std::future<std::pair<bool, RESULT>> taskFuture;       // Holds a success flag, and the result of the async task.
    BackgroundTaskListener<RESULT>& listener;              // The listener to the events of the task. Called on the UI thread.
};

}   // namespace arkostracker

