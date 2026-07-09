#pragma once

#include <juce_events/juce_events.h>

#include <mutex>

#include "../../utils/Id.h"
#include "../../utils/WithParent.h"
#include "../../utils/message/MessageWithOneParameter.h"
#include "../observers/ChannelMuteObserver.h"
#include "../observers/SongPlayerObserver.h"
#include "SerialManager.h"
#include "model/SerialProfile.h"

namespace arkostracker
{

class SerialData;
class Song;
class MainController;
class PreferencesManager;

class SerialController final : public SerialManager::Listener,
                               public PsgRegistersObserverAudioThread,
                               public ChannelMuteObserver
{
public:
    enum class CommunicationState : uint8_t
    {
        stopped,
        starting,
        started,
        stopping,
    };

    /** Constructor. */
    explicit SerialController(MainController& mainController) noexcept;

    /** Destructor. */
    ~SerialController() noexcept override;

    /**
     * Builds a SerialData from the given SerialProfile.
     * @param serialProfile the profile.
     * @param preferencesManager the preferences manager to get the port and the possible custom data.
     * @return a SerialData.
     */
    static SerialData buildFromSerialProfile(SerialProfile serialProfile, const PreferencesManager& preferencesManager) noexcept;

    /** Called when the user wants to start/stop the serial communication. */
    void onUserWantsToStartStopSerial() noexcept;

    // SerialManager::Listener method implementations.
    // ===================================================
    void onCommunicationStartedWorkerThread() override;
    void onCommunicationStoppedWorkerThread(SerialAccess::SerialOperationResult errorReason) override;

    // PsgRegistersObserverAudioThread method implementations.
    // ===================================================
    void onNewPsgRegistersOnAudioThread(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept override;

    // ChannelMuteObserver method implementations.
    // ===================================================
    void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) override;

private:
    /** Message when the communication starts. */
    using MessageCommunicationStartedFromWorkerThread = juce::Message;
    /** Message when the communication stops. */
    using MessageCommunicationStoppedFromWorkerThread = MessageWithOneParameter<SerialAccess::SerialOperationResult>;

    class MessageListenerToCommunicationStartedFromWorkerThread final : public juce::MessageListener,
                                                                        public WithParent<SerialController>
    {
    public:
        explicit MessageListenerToCommunicationStartedFromWorkerThread(SerialController& parent) : WithParent(parent)
        { }

        void handleMessage(const juce::Message& /*message*/) override
        {
            //const auto& rightMessage = dynamic_cast<const MessageCommunicationStartedFromAudioThread&>(message);
            // Can safely be called, this is the UI thread.
            parentObject.onCommunicationStarted();
        }
    };

    class MessageListenerToCommunicationStoppedFromWorkerThread final : public juce::MessageListener,
                                                                        public WithParent<SerialController>
    {
    public:
        explicit MessageListenerToCommunicationStoppedFromWorkerThread(SerialController& parent) : WithParent(parent)
        { }

        void handleMessage(const juce::Message& message) override
        {
            const auto& rightMessage = dynamic_cast<const MessageCommunicationStoppedFromWorkerThread&>(message);
            // Can safely be called, this is the UI thread.
            parentObject.onCommunicationStopped(rightMessage.getData());
        }
    };

    /** Called when the communication started. Called from UI thread. */
    void onCommunicationStarted() noexcept;
    /**
     * Called when the communication stopped. Called from UI thread.
     * @param reason why it stopped.
     */
    void onCommunicationStopped(SerialAccess::SerialOperationResult reason) noexcept;

    /** Called from the UI thread to stop the Serial communication, updates the UI. */
    void stop() noexcept;

    /**
     * Sends an event to the serial observers.
     * @param communicationState the new state.
     * @param reason how it went.
     */
    void sendEvent(CommunicationState communicationState, SerialAccess::SerialOperationResult reason = SerialAccess::SerialOperationResult::success) noexcept;

    MainController& mainController;
    SerialManager serialManager;                // Manages the threading and access to the Serial port.

    MessageListenerToCommunicationStartedFromWorkerThread listenerToCommunicationStartedFromWorkerThread;     // Listener to when the communication starts from the audio thread. Can be safely used on UI thread.
    MessageListenerToCommunicationStoppedFromWorkerThread listenerToCommunicationStoppedFromWorkerThread;     // Listener to when the communication stops from the audio thread. Can be safely used on UI thread.

    CommunicationState communicationState;

    std::mutex mutexMutedChannelIndexes;
    std::unordered_set<int> mutedChannelIndexes_ProtectedByMutex;                        // Indexes of the channels that are muted. PROTECTED BY MUTEX.
};

}   // namespace arkostracker