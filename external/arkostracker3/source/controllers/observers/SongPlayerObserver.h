#pragma once

#include "../../player/InstrumentPlayedInfo.h"
#include "../../player/PsgRegisters.h"
#include "../../player/SampleData.h"
#include "../../song/Location.h"

namespace arkostracker 
{

/** Observer of the events from the Song Player. The events are called on the UI thread. */
class SongPlayerObserver
{
public:
    enum class Event : std::uint8_t
    {
        playFromStart,
        play,
        playPatternFromStart,
        playPatternContinue,
        playPatternFromTopOrBlock,
        playLine,
        stop,
    };

    /** Simple holder of the locations used by the player. */
    class Locations
    {
    public:
        Locations(Location pPlayStartLocation, Location pLoopStartLocation, Location pPastEndLocation, Location pPlayedLocation) :
                playStartLocation(std::move(pPlayStartLocation)),
                loopStartLocation(std::move(pLoopStartLocation)),
                pastEndLocation(std::move(pPastEndLocation)),
                playedLocation(std::move(pPlayedLocation))
        {
        }

        Location playStartLocation;
        Location loopStartLocation;
        Location pastEndLocation;
        Location playedLocation;
    };

    /** Destructor. */
    virtual ~SongPlayerObserver() = default;

    /**
     * Called on the UI thread when the player has new locations.
     * @param locations the locations.
     */
    virtual void onPlayerNewLocations(const Locations& locations) noexcept
    {
        // The default implementation does nothing.
        (void)locations;
    }

    /**
     * Called on the UI thread when the player has new PSG registers.
     * @param psgIndexToPsgRegistersAndSampleData map linking the PSG index to their registers and the sample data.
     */
    virtual void onNewPsgRegisters(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept
    {
        // The default implementation does nothing.
        (void)psgIndexToPsgRegistersAndSampleData;
    }

    /**
     * Called on the UI thread when the speed changes.
     * @param newSpeed the new speed.
     */
    virtual void onNewSpeed(const int newSpeed) noexcept
    {
        // The default implementation does nothing.
        (void)newSpeed;
    }

    virtual void onNewTestNote(int /*newNote*/) noexcept
    {
        // The default implementation does nothing.
    }

    /**
     * Called when an event from the Player occurs.
     * @param event the event.
     */
    virtual void onPlayerPlayEvent(const Event event) noexcept
    {
        // The default implementation does nothing.
        (void)event;
    }

    /**
     * Called when an instrument is played. Useful to know exactly what part of it is played.
     * Note that the Song Player has no notion of channels being muted or not.
     */
    virtual void onInstrumentPlayed(const InstrumentPlayedInfo& info) noexcept
    {
        (void)info;
    }
};

/** A specific observer when needing full-speed (such as serial communication). */
class PsgRegistersObserverAudioThread
{
public:
    /** Destructor. */
    virtual ~PsgRegistersObserverAudioThread() = default;

    /**
     * Called on the AUDIO THREAD when the player has new PSG registers.
     * @param psgIndexToPsgRegistersAndSampleData map linking the PSG index to their registers and the sample data.
     */
    virtual void onNewPsgRegistersOnAudioThread(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept = 0;
};

}   // namespace arkostracker

