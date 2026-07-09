#pragma once

#include <mutex>

#include <juce_core/juce_core.h>

#include "../../utils/Id.h"
#include "SerialAccess.h"
#include "SerialConverter.h"
#include "model/SerialData.h"

namespace arkostracker
{

class Song;

/** Manages the Serial access, especially the threading system. */
class SerialManager final : juce::Thread
{
public:

    /** Listener to be notified of the events. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the communication is started.
         * WARNING! THIS IS CALLED ON A WORKER THREAD!
         * @param success true if started, false if broken.
         */
        virtual void onCommunicationStartedWorkerThread() = 0;

        /**
         * Called when the communication stopped.
         * WARNING! THIS IS CALLED ON A WORKER THREAD!
         * @param errorReason why the communication stopped.
         */
        virtual void onCommunicationStoppedWorkerThread(SerialAccess::SerialOperationResult errorReason) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to which notify the events.
     */
    explicit SerialManager(Listener& listener) noexcept;

    /**
     * Asking to start the communication.
     * Called by called at any time. This may start the thread if not already done.
     * If the port fails to be opened, the callback will be called.
     * This must be called from the UI thread.
     * @param song the Song, to get the PSG metadata. It is not stored.
     * @param subsongId the subsong ID, to know what PSG must be read.
     * @param serialData the serial data.
     */
    void start(const Song& song, const Id& subsongId, const SerialData& serialData) noexcept;

    /**
     * Asking to stop the communication. This should be stopped when no more sending is required, even later.
     * This must be called from the UI thread.
     */
    void stop() noexcept;

    /**
     * Posts the PSGs data (frequency, replay frequency) to use. Should be called after Start is called.
     * This can be called from the any thread.
     * @param psgData the PSG data.
     */
    void sendPsgData(const std::vector<PsgData>& psgData) noexcept;

    /**
     * Posts PSG values. The Start method must have been called first, else nothing happens.
     * Should be called after the sendPsgData have been sent.
     * This can be called from the any thread.
     * @param psgRegisters the PSG registers for the PSGs.
     */
    void sendPsgValues(const std::vector<PsgRegisters>& psgRegisters) noexcept;

    /**
     * Sends a message for the client to stop all sounds.
     * This can be called from the any thread.
     */
    void sendStopSounds() noexcept;

private:
    const int waitTimeOutMs = 1000;

    /** Stops the Serial port (if any) and the thread. */
    void stopSerialAndThread() noexcept;

    // Thread method implementations.
    // ======================================================
    void run() override;

    // ======================================================

    Listener& listener;             // The listener to give events to.

    // --------------------------------------------
    // Guarded data.
    std::mutex serialDataMutex;                     // Mutex to the SerialData.
    bool isNewDataPostedGuardedByMutex;             // True once new data is posted.
    std::unique_ptr<SerialData> pendingSerialDataGuardedByMutex;                                    // Set to null once read.
    std::unique_ptr<std::vector<PsgData>> pendingPsgMetaDataGuardedByMutex;                         // Set to null once read.
    std::unique_ptr<std::vector<PsgRegisters>> pendingPsgRegistersGuardedByMutex;                   // Set to null once read.
    bool pendingMustStopSoundGuardedByMutex;        // True if the sound must stop, set to null once read.
    // --------------------------------------------
    std::unique_ptr<serial::Serial> serialPortWorkerThread;                 // The port to use. Only accessed from the worker thread!
};

}   // namespace arkostracker
