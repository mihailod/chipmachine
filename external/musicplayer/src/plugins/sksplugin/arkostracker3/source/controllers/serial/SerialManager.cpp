#include "SerialManager.h"

#include "../../song/Song.h"

namespace arkostracker
{

SerialManager::SerialManager(SerialManager::Listener& pListener) noexcept :
    Thread("SerialAccessThread"),
    listener(pListener),

    serialDataMutex(),
    isNewDataPostedGuardedByMutex(false),
    pendingSerialDataGuardedByMutex(),
    pendingPsgMetaDataGuardedByMutex(),
    pendingPsgRegistersGuardedByMutex(),
    pendingMustStopSoundGuardedByMutex(false)
{
}

void SerialManager::start(const Song& song, const Id& subsongId, const SerialData& serialData) noexcept
{
    // UI thread.

    // Port name empty? Special message.
    if (serialData.getPort().trim().isEmpty()) {
        listener.onCommunicationStoppedWorkerThread(SerialAccess::SerialOperationResult::portNameEmpty);
        stop();     // Security.
        return;
    }

    // Stores the new data for the worker thread.
    {
        const std::lock_guard lock(serialDataMutex);        // Lock!
        jassert(pendingSerialDataGuardedByMutex == nullptr);            // Else the previous data has not been read!

        // Extracts the PSG metadata.
        const auto allPsgMetadata = song.getSubsongPsgs(subsongId);
        const auto replayFrequency = song.getReplayFrequencyHz(subsongId);
        std::vector<PsgData> allPsgData;
        allPsgData.reserve(allPsgMetadata.size());
        for (const auto& psgMetadata : allPsgMetadata) {
            allPsgData.emplace_back(psgMetadata.getPsgFrequency(), replayFrequency);
        }
        pendingPsgMetaDataGuardedByMutex = std::make_unique<std::vector<PsgData>>(allPsgData);

        pendingSerialDataGuardedByMutex = std::make_unique<SerialData>(serialData);
        pendingPsgRegistersGuardedByMutex = nullptr;
        pendingMustStopSoundGuardedByMutex = false;

        isNewDataPostedGuardedByMutex = true;
    }

    // Starts the thread if needed.
    if (!isThreadRunning()) {
        startThread();
    } else {
        notify();       // Wakes up the thread, as it was already started.
    }
}

void SerialManager::stop() noexcept
{
    if (!isThreadRunning()) {
        return;
    }

    // Before stopping, sends a Stop sound (if still possible).
    sendStopSounds();

    stopSerialAndThread();
}

void SerialManager::sendPsgData(const std::vector<PsgData>& psgData) noexcept
{
    // ANY THREAD.
    {
        const std::lock_guard lock(serialDataMutex);        // Lock!
        pendingPsgMetaDataGuardedByMutex = std::make_unique<std::vector<PsgData>>(psgData);

        isNewDataPostedGuardedByMutex = true;
    }
    notify();       // Wakes up the thread.
}

void SerialManager::sendPsgValues(const std::vector<PsgRegisters>& psgRegisters) noexcept
{
    // ANY THREAD.
    {
        const std::lock_guard lock(serialDataMutex);        // Lock!
        pendingPsgRegistersGuardedByMutex = std::make_unique<std::vector<PsgRegisters>>(psgRegisters);

        isNewDataPostedGuardedByMutex = true;
    }
    notify();       // Wakes up the thread.
}

void SerialManager::sendStopSounds() noexcept
{
    // ANY THREAD.
    {
        const std::lock_guard lock(serialDataMutex);        // Lock!
        pendingMustStopSoundGuardedByMutex = true;

        isNewDataPostedGuardedByMutex = true;
    }
    notify();       // Wakes up the thread.
}


// Thread method implementations.
// ======================================================

void SerialManager::run()
{
    // We are in the background thread.
    SerialAccess::SerialOperationResult operationResult = SerialAccess::SerialOperationResult::success;

    // Continues as long as possible, and long as there are no errors.
    while (!threadShouldExit() && (operationResult == SerialAccess::SerialOperationResult::success)) {

        // Is there new data?
        {
            std::unique_ptr<SerialData> localSerialData = nullptr;
            std::unique_ptr<std::vector<PsgData>> localPsgMetaData = nullptr;
            std::unique_ptr<std::vector<PsgRegisters>> localPendingPsgRegisters = nullptr;
            bool localMustStopSound = false;
            bool newDataPosted;         // NOLINT(*-init-variables)

            {
                const std::lock_guard lock(serialDataMutex);        // Lock!
                newDataPosted = isNewDataPostedGuardedByMutex;
                if (newDataPosted) {
                    // Gets ownership locally.
                    localSerialData = std::move(pendingSerialDataGuardedByMutex);
                    localPsgMetaData = std::move(pendingPsgMetaDataGuardedByMutex);
                    localPendingPsgRegisters = std::move(pendingPsgRegistersGuardedByMutex);
                    localMustStopSound = pendingMustStopSoundGuardedByMutex;

                    pendingSerialDataGuardedByMutex = nullptr;
                    pendingPsgMetaDataGuardedByMutex = nullptr;
                    pendingPsgRegistersGuardedByMutex = nullptr;
                    pendingMustStopSoundGuardedByMutex = false;
                }
                isNewDataPostedGuardedByMutex = false;
            }

            // Out of the lock.
            // The pending data must not be used! Only local!
            if (newDataPosted) {
                if (localSerialData != nullptr) {
                    // A new port must be used.
                    serialPortWorkerThread = SerialAccess::createPortAndOpen(*localSerialData);
                    if (serialPortWorkerThread == nullptr) {
                        operationResult = SerialAccess::SerialOperationResult::channelCouldNotBeOpened;
                    } else {
                        // Notifies the listener the communication works. We it here only no to pollute with too many messages.
                        listener.onCommunicationStartedWorkerThread();
                    }
                }

                if (serialPortWorkerThread != nullptr) {
                    // New PSGs?
                    if (localPsgMetaData != nullptr) {
                        operationResult = SerialAccess::sendInitializationSync(*serialPortWorkerThread, *localPsgMetaData);
                    }

                    // New PSG Registers?
                    if ((localPendingPsgRegisters != nullptr) && (operationResult == SerialAccess::SerialOperationResult::success)) {
                        operationResult = SerialAccess::sendPsgRegistersSync(*serialPortWorkerThread, *localPendingPsgRegisters);
                    }

                    // New Stop sound?
                    if (localMustStopSound && (operationResult == SerialAccess::SerialOperationResult::success)) {
                        operationResult = SerialAccess::sendStopSoundsSync(*serialPortWorkerThread);
                    }
                }
            }
        }

        if (operationResult == SerialAccess::SerialOperationResult::success) {
            // Waits till new events arrive (if no error).
            wait(waitTimeOutMs);
        } else {
            // Something went wrong. Notifies the listener.
            listener.onCommunicationStoppedWorkerThread(operationResult);
        }
    }
}

void SerialManager::stopSerialAndThread() noexcept
{
    serialPortWorkerThread.reset();

    stopThread(1000);
    listener.onCommunicationStoppedWorkerThread(SerialAccess::SerialOperationResult::success);
}

}   // namespace arkostracker
