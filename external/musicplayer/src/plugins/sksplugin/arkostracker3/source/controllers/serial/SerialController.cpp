#include "SerialController.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../player/SongPlayer.h"
#include "../../utils/PsgValues.h"
#include "../MainController.h"
#include "model/SerialData.h"

namespace arkostracker
{

SerialController::SerialController(MainController& pMainController) noexcept :
        mainController(pMainController),
        serialManager(*this),
        listenerToCommunicationStartedFromWorkerThread(*this),
        listenerToCommunicationStoppedFromWorkerThread(*this),
        communicationState(CommunicationState::stopped),
        mutexMutedChannelIndexes(),
        mutedChannelIndexes_ProtectedByMutex()
{
}

SerialController::~SerialController() noexcept
{
    serialManager.stop();
    mainController.getSongPlayer().setPsgRegistersObserverAudioThread(nullptr);         // Only a security.
}

void SerialController::onUserWantsToStartStopSerial() noexcept
{
    if (communicationState == CommunicationState::stopped) {
        communicationState = CommunicationState::starting;
        DBG("onCommunicationStarting");
        sendEvent(CommunicationState::starting);

        auto& songController = mainController.getSongController();
        const auto song = songController.getSong();
        const auto subsongId = songController.getCurrentSubsongId();

        // Gets the serial port data.
        const auto& preferences = PreferencesManager::getInstance();
        const auto serialProfile = preferences.getSelectedSerialProfile();
        const auto serialData = buildFromSerialProfile(serialProfile, preferences);

        serialManager.start(*song, subsongId, serialData);
    } else if (communicationState == CommunicationState::started) {
        communicationState = CommunicationState::stopping;
        DBG("onCommunicationStopping");
        sendEvent(CommunicationState::stopping);

        stop();
    }
    // Ignore other states.
}

void SerialController::stop() noexcept
{
    serialManager.stop();
}

SerialData SerialController::buildFromSerialProfile(SerialProfile serialProfile, const PreferencesManager& preferencesManager) noexcept
{
    // Default values.
    auto baudRate = 115200;
    auto byteSize = 8;
    auto parity = 0;
    auto stopBit = StopBit::stopBit1;
    auto flowControl = FlowControl::none;

    switch (serialProfile) {
        case SerialProfile::albireo: [[fallthrough]];
        case SerialProfile::usifac: [[fallthrough]];
        case SerialProfile::booster:
            // Set above.
            break;
        case SerialProfile::custom:
            const auto customProfile = preferencesManager.getSerialCustomProfile();
            baudRate = customProfile.getBaudRate();
            byteSize = customProfile.getByteSize();
            parity = customProfile.getParity();
            stopBit = customProfile.getStopBits();
            flowControl = customProfile.getFlowControl();
            break;
    }

    const auto port = preferencesManager.getSerialPort();

    return {
        port, baudRate, byteSize, parity, stopBit, flowControl
    };
}


// SerialManager::Listener method implementations.
// ===================================================

void SerialController::onCommunicationStartedWorkerThread()
{
    // THIS IS CALLED FROM A WORKER THREAD!!

    // Posts the message.
    auto* message = new MessageCommunicationStartedFromWorkerThread();      // Don't worry, reference counted object.
    listenerToCommunicationStartedFromWorkerThread.postMessage(message);
}

void SerialController::onCommunicationStoppedWorkerThread(SerialAccess::SerialOperationResult errorReason)
{
    // THIS IS CALLED FROM A WORKER THREAD!!

    // Posts the message.
    auto* message = new MessageCommunicationStoppedFromWorkerThread(errorReason);      // Don't worry, reference counted object.
    listenerToCommunicationStoppedFromWorkerThread.postMessage(message);
}


// PsgRegistersObserverAudioThread method implementations.
// ===================================================

void SerialController::onNewPsgRegistersOnAudioThread(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept
{
    // THIS IS THE AUDIO THREAD.

    // Orders the PSG Registers.
    std::vector<int> psgIndexes;
    psgIndexes.reserve(psgIndexToPsgRegistersAndSampleData.size());
    for (const auto& [psgIndex, data] : psgIndexToPsgRegistersAndSampleData) {
        psgIndexes.emplace_back(psgIndex);
    }
    std::sort(psgIndexes.begin(), psgIndexes.end());

    // Copies locally the mutex-protected muted channels.
    std::unordered_set<int> mutedChannelIndexes;
    {
        const std::lock_guard lock(mutexMutedChannelIndexes);       // Lock!
        mutedChannelIndexes = mutedChannelIndexes_ProtectedByMutex;
    }

    std::vector<PsgRegisters> psgRegisters;
    psgRegisters.reserve(psgIndexes.size());
    for (auto psgIndex : psgIndexes) {
        auto psgRegister = psgIndexToPsgRegistersAndSampleData.at(psgIndex).first;
        // Must mute channels?
        if (!mutedChannelIndexes.empty()) {
            for (auto channelIndex = 0; channelIndex < PsgValues::channelCountPerPsg; ++channelIndex) {
                const auto channelIndexInPsgs = PsgValues::getChannelIndex(channelIndex, psgIndex);
                if (mutedChannelIndexes.find(channelIndexInPsgs) != mutedChannelIndexes.cend()) {
                    // Mutes the channel.
                    psgRegister.muteChannel(channelIndex);
                }
            }
        }

        psgRegisters.push_back(psgRegister);
    }

    serialManager.sendPsgValues(psgRegisters);
}


// ChannelMuteObserver method implementations.
// ===================================================

void SerialController::onChannelsMuteStateChanged(const std::unordered_set<int>& newMutedChannelIndexes)
{
    const std::lock_guard lock(mutexMutedChannelIndexes);       // Lock!
    mutedChannelIndexes_ProtectedByMutex = newMutedChannelIndexes;
}


// ===================================================

void SerialController::onCommunicationStarted() noexcept
{
    DBG("onCommunicationStarted");

    // Gets the current mute channels, the dynamic ones will use observations.
    {
        const std::lock_guard lock(mutexMutedChannelIndexes);       // Lock!
        mutedChannelIndexes_ProtectedByMutex = mainController.getChannelMuteStates();
    }

    // Registers to the song player PSG registers and mute state.
    mainController.getSongPlayer().setPsgRegistersObserverAudioThread(this);
    mainController.observers().getChannelMuteStateObservers().addObserver(this);

    communicationState = CommunicationState::started;

    // Notifies the controller about the success (change of icon).
    sendEvent(CommunicationState::started);
}

void SerialController::onCommunicationStopped(SerialAccess::SerialOperationResult reason) noexcept
{
    DBG("onCommunicationStopped");

    // Unregisters from the song player PSG registers.
    mainController.getSongPlayer().setPsgRegistersObserverAudioThread(nullptr);
    mainController.observers().getChannelMuteStateObservers().removeObserver(this);

    communicationState = CommunicationState::stopped;

    // Notifies the controller about the success or failure (change of icon, bubble).
    sendEvent(CommunicationState::stopped, reason);
}

void SerialController::sendEvent(SerialController::CommunicationState communicationStateToSend, SerialAccess::SerialOperationResult reason) noexcept
{
    mainController.observers().getGeneralDataObservers().applyOnObservers([&](GeneralDataObserver* observer) {
        observer->onSerialCommunicationEvent(communicationStateToSend, reason);
    });
}

}   // namespace arkostracker
