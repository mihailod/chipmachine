#pragma once

#include <juce_events/juce_events.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include "../controllers/observers/SongPlayerObserver.h"
#include "../song/Location.h"
#include "../song/Song.h"
#include "../utils/Observers.h"
#include "../utils/WithParent.h"
#include "CellToPlay.h"
#include "PsgRegisters.h"
#include "PsgRegistersProvider.h"
#include "SampleData.h"
#include "SongDataProvider.h"
#include "channel/ChannelPlayer.h"

namespace arkostracker 
{

class EffectContext;

/**
 * The Song Player reads the Song data and generates PSG registers for ALL the PSG of the Song,
 * which makes it simpler for synchronization.
 *
 * Each PSG is played one after the other, and the result stored in queues (one for each PSG). This isn't really useful for real-time, but it is
 * for non-real-time, as the buffer may be big and as such, the getPsgRegisters can be called several times in a row for the same PSG.
 * When one queue is depleted and a data is requested, all the queues are being fed again at the same time.
 *
 * This class holds the information about the subsong/lines being played, which listeners can be notified of.
 *
 * About the Speed: it is managed from 1 to 255, 6 being "standard". 0 means 1. It is simpler to manage it this way.
 *
 * A song is "being played" if the player is allowed to "move to the next line".
 *
 * NOTE: this class does not react to any observation, but must be told what to do.
 *
 * Threading
 * ---------
 * If nothing is indicated, the public methods must be called from the UI thread, BUT the getNextRegisters is called from the Audio Thread!
 */
class SongPlayer final : public PsgRegistersProvider,                 // Provides the PSG registers to play PSG sounds.
                         public SongDataProvider
{
public:
    /**
     * Constructor.
     * @param song the song to play.
     */
    explicit SongPlayer(std::shared_ptr<const Song> song) noexcept;

    /**
     * Sets the Song to play. This stops the player first and clears its data.
     * @param song the Song.
     */
    void setSong(std::shared_ptr<const Song> song) noexcept;

    /** @return true if the song is being played (pattern, song). The line being played is NOT considered "playing". */
    bool isPlaying() const noexcept;

    /**
     * Plays the Song from the given start to given end, but can also play from the currently played location.
     * Called from UI thread.
     * @param startLocation where to go to and start to play, if present. Else, continues where it was.
     * @param loopStartLocation where to go once the end has been reached.
     * @param pastEndLocation the past-end location. When reached, goes to the loopStartLocation. It is NOT the end location, because managing the past-end
     * takes care of corner cases such as a pattern that would have its height changed.
     * Warning, ONLY THE POSITION MUST BE past, not the line. If playing a block, the pastEnd position is the same.
     * @param stopSounds true to stop the sounds and reset effects.
     * @param resetTick if true, the ticks counter is reset. False if useful when setting a new end location, without wanting a "cut" in the stream.
     * @param isFullSongPlayed true to consider the user wants to play the full song (even if starting not from the first position). False if unsure.
     * This is useful to skip EffectContext when looping the song, because it is annoying.
     */
    void play(const OptionalValue<Location>& startLocation, const Location& loopStartLocation, const Location& pastEndLocation, bool stopSounds,
              bool resetTick, bool isFullSongPlayed = false) noexcept;

    /**
     * Plays a Cell. The player must be in "play" mode. It can be either from a Test Area or from the PV (Editor behavior).
     * @param cell the cell to play.
     * @param channelIndex the channel index. Must be valid, else asserts.
     * @param location where the note is. It is useful to use the Effect Context. Test Area behavior won't use this.
     */
    void playCell(const Cell& cell, int channelIndex, const OptionalValue<CellLocationInPosition>& location) noexcept;

    /**
     * Plays a line. Will stop advancing once all the ticks of the lines have been read.
     * Called from UI thread.
     * @param location where to read.
     */
    void playLine(const Location& location) noexcept;

    /**
     * Sets the currently played locations. This is especially meant to be used when not playing, to post a future location when playing again.
     * @param startLocation where to go to and start to play, if present. Else, continues where it was.
     * @param loopStartLocation where to go once the end has been reached.
     * @param pastEndLocation the past-end location. When reached, goes to the loopStartLocation. It is NOT the end location, because managing the past-end
     * takes care of corner cases such as a pattern that would have its height changed.
     */
    void setCurrentlyPlayedLocations(const Location& startLocation, const Location& loopStartLocation, const Location& pastEndLocation) noexcept;

    /**
     * @return the currently played location in Song, ONLY for non-realtime because no lock is performed, so this must not be multithreaded.
     * This should be called BEFORE the getNextRegisters is called, else it may have advanced.
     */
    Location getCurrentLocationInSongOffline() const noexcept;
    /**
     * @return the results of all the ChannelPlayers. The current speed and ticks are set to all items.
     * WARNING, do NOT call this in a loop with the getNextRegisters from each psg! Call all the getNextRegisters (ignoring the results), THEN
     * call getResultsOffline.
     * WARNING, this method is NOT thread safe. Its use is currently for non-real-time use, with the caller being on the same thread as the SongPlayer.
     */
    std::vector<std::unique_ptr<ChannelPlayerResults>> getResultsOffline() const noexcept;

    /**
     * Stops the song from being played. Sounds are stopped, effects are reset.
     * This posts a message for the audio player to stop, so is asynchronous.
     * Called from UI thread.
     * This does not prevent the player from receiving events, this only stops the sounds.
     */
    void stop() noexcept;

    /**
     * Posts a played location. It will be corrected. If out of bounds, the playing location will be forced to the start loop when the player reads it.
     * @param location the location. May be invalid.
     */
    void setPlayedLocation(const Location& location) noexcept;

    /** @return the Observers, to be able to register/unregisters. */
    Observers<SongPlayerObserver>& getSongPlayerObservers() noexcept;

    /**
     * Stores a special observer that will be notified on the AUDIO THREAD. This is for special use, like speed requirements, such as the Serial communication.
     * As it is special, there is no multiple observers for now.
     * Set it to nullptr when done.
     * @param observer the observer, called on the audio thread. Nullptr to un-observe.
     */
    void setPsgRegistersObserverAudioThread(PsgRegistersObserverAudioThread* observer) noexcept;

    /**
     * This is only for OFFLINE generation (WAV output, etc.), as no thread synchronisation is performed.
     * Sets how many loops to reach before the output will become mute.
     * When the job is done, do NOT reuse this instance of the SongPlayer, these counters are never reset.
     * @param loopCount how many loops (should be >= 1).
     */
    void setOfflineSongEndCountBeforeMuting(int loopCount) noexcept;

    /**
     * @return true if the end has been reached, the count having been set by the "setOfflineSongEnd..." counterpart method.
     * This is only for OFFLINE generation (WAV output, etc.), as no thread synchronisation is performed.
     */
    bool hasOfflineSongEndCountReached() const noexcept;

    /** Sets the effect context. May be nullptr. */
    void setEffectContext(EffectContext* effectContext) noexcept;

    // PsgRegistersProvider method implementations.
    // ======================================================
    std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> getNextRegisters(int psgIndex) noexcept override;


    // SongDataProvider method implementations.
    // ======================================================
    OptionalId getInstrumentIdFromAudioThread(int instrumentIndex) const noexcept override;
    InstrumentType getInstrumentTypeFromAudioThread(const OptionalId& instrumentId) const noexcept override;
    PsgInstrumentFrameData getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, int cellIndex) const noexcept override;
    SampleInstrumentFrameData getSampleInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId) const noexcept override;
    OptionalId getExpressionIdFromAudioThread(bool isArpeggio, int expressionIndex) const noexcept override;
    ExpressionMetadata getExpressionMetadataFromAudioThread(bool isArpeggio, const OptionalId& expressionId) const noexcept override;
    int getExpressionValueFromAudioThread(bool isArpeggio, const OptionalId& expressionId, int cellIndex) const noexcept override;
    int getTranspositionFromAudioThread(int channelIndexInSong) const noexcept override;
    std::pair<int, float> getPsgFrequencyFromChannelFromAudioThread(int channelIndexInSong) const noexcept override;
    bool isEffectContextEnabled() const noexcept override;
    LineContext determineEffectContextFromAudioThread(CellLocationInPosition location) const noexcept override;
    LineContext determineEffectContextFromAudioThread(int channelIndexInSong) const noexcept override;
    SidPlayerCapability getSidPlayerCapability() const noexcept override;

private:
    class DataFromUiThread;

    /**
     * Posts data from the UI thread.
     * @param dataFromUiThread the data.
     */
    void postDataFromUiThread(std::unique_ptr<DataFromUiThread> dataFromUiThread) noexcept;

    /**
     * Checks the channel player count is well related to the Subsong PSG count, and create or deletes some if needed.
     * Must be called only from the Audio Thread!
     */
    void resizeChannelPlayersIfNeeded() noexcept;

    /**
     * Fills the given PsgRegisters with the data from the given ChannelOutputRegisters.
     * @param targetChannel the channel index to fill in the Psg Registers (0-2).
     * @param psgRegistersToFill the PSG Registers to fill.
     * @param sampleDataToFill the Sample data to fill.
     * @param inputChannelPlayerResults the results (PSG and samples) from the Channel Player.
     * @param event a possible event, or 0 if not present.
     */
    static void fillWithChannelResult(int targetChannel, PsgRegisters& psgRegistersToFill, SampleData& sampleDataToFill,
                                      const ChannelPlayerResults& inputChannelPlayerResults, int event) noexcept;

    /**
     * Advances in the ticks/line/pattern. This is called from a method called from the Audio Thread!
     * @param song the Song.
     */
    void advanceInSong(const std::shared_ptr<const Song>& song) noexcept;

    /** Notifies the listener from the new PSG registers. This is called from Audio Thread, but it creates a message to the UI thread. */
    void notifyListenersFromNewPsgRegisters() const noexcept;

    /** Notifies the listener from the instrument being played (and what cell). This is called from Audio Thread, but it creates a message to the UI thread. */
    void notifyListenersForInstrumentPlayed(const InstrumentPlayedInfo& messageInstrumentPlayed) const noexcept;

    /** Message listener to pass data from the worker thread, to the UI thread. It passes the new location. */
    class MessageListenerNewPlayerLocationFromWorkerThread final : public juce::MessageListener, public WithParent<SongPlayer>
    {
    public:
        explicit MessageListenerNewPlayerLocationFromWorkerThread(SongPlayer& parent) : WithParent(parent) {}
        void handleMessage(const juce::Message& message) override;
    };

    /** Message listener to pass data from the worker thread, to the UI thread. It passes the new PSG registers. */
    class MessageListenerNewPsgRegistersFromWorkerThread final : public juce::MessageListener, public WithParent<SongPlayer>
    {
    public:
        explicit MessageListenerNewPsgRegistersFromWorkerThread(SongPlayer& parent) : WithParent(parent) {}
        void handleMessage(const juce::Message& message) override;
    };

    /** Message listener to pass data from the worker thread, to the UI thread. It passes the new speed. */
    class MessageListenerNewSpeedFromWorkerThread final : public juce::MessageListener, public WithParent<SongPlayer>
    {
    public:
        explicit MessageListenerNewSpeedFromWorkerThread(SongPlayer& parent) : WithParent(parent) {}
        void handleMessage(const juce::Message& message) override;
    };

    /** Message listener to pass data from the worker thread, to the UI thread. It passes the instrument being played, and its cell. */
    class MessageListenerInstrumentPlayedFromWorkerThread final : public juce::MessageListener, public WithParent<SongPlayer>
    {
    public:
        explicit MessageListenerInstrumentPlayedFromWorkerThread(SongPlayer& parent) : WithParent(parent) {}
        void handleMessage(const juce::Message& message) override;
    };

    std::shared_ptr<const Song> song;                                           // The song being played.
    Observers<SongPlayerObserver> songPlayerObservers;                          // Observers can register to be notified of events.

    /** Holds the data from the UI Thread. */
    class DataFromUiThread
    {
    public:
        /**
         * Constructor.
         * @param pNewStartLocation the new start location. Absent to use the already known value.
         * @param pNewLoopStartLocation the new loop start location. Absent to use the already known value.
         * @param pNewPastEndLocation the new past-end location. Absent to use the already known value.
         * @param pStopSounds true to stop the sounds (posted). It does NOT stop the player.
         * @param pResetTick true to reset the tick counter.
         * @param pCanReadSongLine if true, the song can read the current line at least till the last tick. Absent not to change the current value.
         * @param pCanMoveToNextLine if true, moves to the next line when reaching the last tick. Absent not to change the current value.
         * @param pIsFullSongPlayed true to consider the user wants to play the full song (even if starting not from the first position). False if unsure.
         * This is useful to skip EffectContext when looping the song, because it is annoying.
         */
        DataFromUiThread(const OptionalValue<Location>& pNewStartLocation, const OptionalValue<Location>& pNewLoopStartLocation,
                         const OptionalValue<Location>& pNewPastEndLocation, const bool pStopSounds, const bool pResetTick,
                         const OptionalBool pCanReadSongLine, const OptionalBool pCanMoveToNextLine,
                         const OptionalBool pIsFullSongPlayed) :
                newStartLocation(pNewStartLocation),
                newLoopStartLocation(pNewLoopStartLocation),
                newPastEndLocation(pNewPastEndLocation),
                stopSounds(pStopSounds),
                resetTick(pResetTick),
                canReadSongLine(pCanReadSongLine),
                canMoveToNextLine(pCanMoveToNextLine),
                isFullSongPlayed(pIsFullSongPlayed)
        {}

        const OptionalValue<Location> newStartLocation;             // NOLINT(*-non-private-member-variables-in-classes)
        const OptionalValue<Location> newLoopStartLocation;         // NOLINT(*-non-private-member-variables-in-classes)
        const OptionalValue<Location> newPastEndLocation;           // NOLINT(*-non-private-member-variables-in-classes)
        const bool stopSounds;                                      // NOLINT(*-non-private-member-variables-in-classes)
        const bool resetTick;                                       // NOLINT(*-non-private-member-variables-in-classes)
        const OptionalBool canReadSongLine;                         // NOLINT(*-non-private-member-variables-in-classes)
        const OptionalBool canMoveToNextLine;                       // NOLINT(*-non-private-member-variables-in-classes)
        const OptionalBool isFullSongPlayed;                        // NOLINT(*-non-private-member-variables-in-classes)
    };


    // -----------------------------------------------
    mutable std::mutex dataFromUiThreadMutex;
    std::queue<DataFromUiThread> queueNewDataFromUiThread_ProtectedByMutex;     // Queue with UI-thread posted data. Use the mutex when accessing it!

    mutable std::mutex cellsToPlayThreadMutex;
    std::vector<std::unique_ptr<CellToPlay>> cellsToPlay_ProtectedByMutex;      // Only present when a cell must be played. Use the mutex when accessing it!
    // -----------------------------------------------

    MessageListenerNewPlayerLocationFromWorkerThread listenerNewPlayerLocationFromWorkerThread; // To pass the new player location from Audio Thread to UI Thread.
    MessageListenerNewPsgRegistersFromWorkerThread listenerNewPsgRegistersFromWorkerThread;     // To pass the new PSG registers from Audio Thread to UI Thread.
    MessageListenerNewSpeedFromWorkerThread listenerNewSpeedFromWorkerThread;                   // To pass the new speed from Audio Thread to UI Thread.
    MessageListenerInstrumentPlayedFromWorkerThread listenerInstrumentPlayedFromWorkerThread; // To pass the instrument being played from Audio Thread to UI Thread.

    // The following members must ONLY be accessed in the audio thread!
    // -----------------------------------------------------------------
    Location playedLocation_AudioThread;                                        // What is being played.
    Location playStartLocation_AudioThread;                                     // Where the play has started.
    Location loopStartLocation_AudioThread;                                     // Where to loop when the end is reached.
    Location pastEndLocation_AudioThread;                                       // The past-end location (end position + 1). When reached, loops or stops.

    int currentTick_AudioThread;                                                // Increases from 0 to speed, excluded.
    int currentSpeed_AudioThread;                                               // The current speed (>0, 1 to 255 included).

    bool canReadSongLine_AudioThread;                                           // If true, the song can read the current line at least till the last tick.
    bool canMoveToNextLine_AudioThread;                                         // If true, moves to the next line when reaching the last tick.

    std::vector<std::unique_ptr<ChannelPlayer>> channelPlayers_AudioThread;     // The PSG count * three channel players.
    /** Links a PSG index to the PSG registers/samples it must read. */
    std::unordered_map<int, std::queue<std::pair<PsgRegisters, std::unique_ptr<SampleData>>>> psgIndexToPendingRegistersAndSamples_AudioThread;

    PsgRegisters outputPsgRegisters_AudioThread;                                // Kept from one frame to another, useful for YM exporters (hardware regs were cleared).

    bool isFullSongPlayed_AudioThread;

    bool mustClearQueues_AudioThread;

    // -----------------------------------------------------------------
    // Fast observer.
    mutable std::mutex psgRegisterFastObserverMutex;
    PsgRegistersObserverAudioThread* psgRegisterFastObserver_ProtectedByMutex;  // Protected by psgRegisterFastObserverMutex.

    // -----------------------------------------------------------------

    std::atomic_bool playing;                                                   // A generic flag to know if the player is playing. Can be read from the UI.
    std::atomic_bool canReadSongLine;                                           // Set from the Audio Thread. Must not be set by the UI thread!
    std::atomic_bool canMoveToNextLine;                                         // Set from the Audio Thread. Must not be set by the UI thread!

    int offlineSongEndedCount;              // Increases whenever the song has reached an end. No synchronisation, this is only for offline generation!
    int offlineSongEndCountBeforeMuting;    // If reached, don't produce any sound anymore. No synchronisation, this is only for offline generation!
    int offlineCurrentTick;                 // The current tick. No synchronisation, this is only for offline generation!

    EffectContext* effectContext;                                               // The possible Effect Context.
};

}   // namespace arkostracker
