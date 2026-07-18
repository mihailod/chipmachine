#pragma once

#include <functional>
#include <future>

#include <juce_events/juce_events.h>

#include "../../../utils/WithParent.h"

namespace arkostracker 
{

/** A background operation, performed asynchronously. It does not handle progress or cancellation (see BackgroundTask for that). */
template<typename RESULT>
class BackgroundOperation
{
public:
    /**
     * Constructor.
     * @param pCallback the callback, called on the UI thread, with the result.
     * @param pWork the work to be performed on the WORKER thread.
     */
    BackgroundOperation(std::function<void(RESULT)> pCallback, std::function<RESULT()> pWork) noexcept :
            messageListenerToOperationFinished(this),
            backgroundOperation(std::move(pWork)),
            operationFuture(),
            callback(std::move(pCallback))
    {
    }

    /** Starts the operation. Call this on the UI thread. */
    void performOperation() noexcept
    {
        // UI thread.

        jassert(backgroundOperation != nullptr);

        // Performs the operation asynchronously.
        operationFuture = std::async(std::launch::async, [&]() -> RESULT {
            // WORKER thread.

            // Performs the operation.
            auto result = backgroundOperation();

            // Done, then notifies the UI thread. It will have to "wait" for the result, returned just below. This should be instantaneous.
            auto* message = new juce::Message();        // Reference counted object.
            messageListenerToOperationFinished.postMessage(message);

            return result;
        });
    }

private:
    /** Called when the operation is finished. */
    void onFinishedOperation() noexcept
    {
        // UI Thread.

        // Gets the result. This is blocking, but the result is already present when the current method is called, so it should be instantaneous.
        auto result = operationFuture.get();
        callback(std::move(result));
    }

    /** Implementation of the MessageListener to the result of the threaded operation. */
    class MessageListenerToOperationFinished final : public juce::MessageListener,
                                                     public WithParent<BackgroundOperation>
    {
    public:
        explicit MessageListenerToOperationFinished(BackgroundOperation* parent) :
                WithParent<BackgroundOperation>(*parent)
        {
        }
        void handleMessage(const juce::Message& /*message*/) override
        {
            this->parentObject.onFinishedOperation();
        }
    };
    MessageListenerToOperationFinished messageListenerToOperationFinished;      // Listener to even from the working thread. Can be safely used on UI thread.

    std::function<RESULT()> backgroundOperation;
    std::future<RESULT> operationFuture;                        // Holds the result and the async operation.
    std::function<void(RESULT)> callback;                       // The callback of the operation, with the result. Called on the UI thread.
};

}   // namespace arkostracker
