#pragma once

#include "../song/CellLocationInPosition.h"
#include "../utils/Observers.h"
#include "MidiController.h"
#include "observers/SongPlayerObserver.h"

namespace arkostracker 
{

class Cell;
class EffectContext;

/**
 * Controller of the Player.
 * It actually does not do much by itself, but asks the SongPlayer to do actions. It does not hold it, but
 * asks the MainController when it needs it.
 *
 * It also holds the logic of what to do when the PatternViewer wants to move.
 */
class PlayerController : public MidiController::MidiListener
{
public:
    /** Destructor. */
    ~PlayerController() override = default;

    /** @return an object to which can be added Observation to the Song Player. */
    virtual Observers<SongPlayerObserver>& getSongPlayerObservers() noexcept = 0;


    // ========================================================

    /** @return the current octave. */
    virtual int getCurrentOctave() const noexcept = 0;
    /**
     * Sets the octave. This does not notify anything.
     * @param desiredOctave the desired octave. It will be corrected.
     */
    virtual void setOctave(int desiredOctave) noexcept = 0;

    /** Plays the song from the start. */
    virtual void playSongFromStart() noexcept = 0;
    /** Plays the song from the current location. If it was already playing, it continues seamlessly (useful when playing pattern, then the song). */
    virtual void playSongFromCurrentLocation() noexcept = 0;
    /** Plays the current Pattern from the start. */
    virtual void playPatternFromStart() noexcept = 0;
    /** Plays the current Pattern from the shown location (where the cursor is), or from a Block select, if any. */
    virtual void playPatternFromShownLocationOrBlock() noexcept = 0;

    /**
     * Plays the song from the given location. This is useful when the user clicks on a position in the Linker for example.
     * If before End, plays normally. If after, will only play the given pattern.
     * @param location the location.
     */
    virtual void playSongFromLocation(const Location& location) noexcept = 0;

    /**
     * Plays the line from the given location. This will prevent the player from going to the next line.
     * @param location the location.
     */
    virtual void playLine(const Location& location) noexcept = 0;

    /**
     * Sets the currently played location. This is especially meant to be used when not playing, to post a future location when playing again.
     * @param location the location.
     */
    virtual void setCurrentlyPlayedLocation(const Location& location) noexcept = 0;

    /**
     * Stops the player asynchronously. It does not prevent notes from being played, only stops playing and stop the notes.
     */
    virtual void stopPlaying() noexcept = 0;

    /** @return true if the song is being played (song, pattern). False does not mean there is no sound: it can be a sound from the Test Area, or from a Play Line. */
    virtual bool isPlaying() const noexcept = 0;

    /** @return true if the latest PlayXXX loops on one Pattern (including block). */
    virtual bool isPlayingOnPattern() const noexcept = 0;

    // ========================================================

    /**
     * Plays a note and instrument, using the Test Area behavior. The note becomes the new default note.
     * The selected arpeggio and pitch are used if they are activated in the TA (or this controller was asked to).
     * @param newNote the note. If absent, uses the previous or default one.
     * @param isBaseNote only relevant if a note is present. If true, considers the note is a base note (>=0, probably <24), to which the current octave will be added.
     * @param newInstrumentId the ID of the instrument to play (it should exist), or none to use the selected one. If given, the selected Instrument does not change.
     */
    virtual void play(OptionalInt newNote, bool isBaseNote, OptionalId newInstrumentId) noexcept = 0;

    /**
     * Plays a note and instrument, using an Editor behavior (such as the Pattern Viewer), record is off.
     * @param note the note.
     * @param instrumentIndex the instrument to play, or the last one if none is specified (useful for legato).
     * @param location where the note is played. Needed to know the channel, and for the effect context feature. If invalid, nothing is played and asserts.
     */
    virtual void playFromEditor(int note, OptionalInt instrumentIndex, CellLocationInPosition location) noexcept = 0;

    /**
     * Plays a note and instrument, using an Editor behavior (such as the Pattern Viewer), record is off.
     * @param cell the Cell.
     * @param location where the note is played. Needed to know the channel, and for the effect context feature. If invalid, nothing is played and asserts.
     */
    virtual void playFromEditor(const Cell& cell, CellLocationInPosition location) noexcept = 0;


    // ========================================================

    /**
     * Set the playing location. For example, the user used the mouse wheel the mouse wheel while playing.
     * @param location where to go. May be invalid.
     */
    virtual void setPlayedLocation(const Location& location) noexcept = 0;

    /** @return true if the selected Arpeggio is used when playing a note (not from an Editor such as the PV). */
    virtual bool isSelectedArpUsedWhenPlayingNote() const noexcept = 0;
    /** @return true if the selected Pitch is used when playing a note (not from an Editor such as the PV). */
    virtual bool isSelectedPitchUsedWhenPlayingNote() const noexcept = 0;
    /** @return true if the monophonic is used, false if polyphonic, when testing a sound (not from an Editor such as the PV). */
    virtual bool isUsingMonophony() const noexcept = 0;

    /** @return the new state after asking to toggle the "is selected arpeggio used" state. */
    virtual bool toggleIsSelectedArpUsed() noexcept = 0;
    /** @return the new state after asking to toggle the "is selected pitch used" state. */
    virtual bool toggleIsSelectedPitchUsed() noexcept = 0;
    /** @return the new state after asking to toggle the "is using monophonic" state. */
    virtual bool toggleIsUsingMonophony() noexcept = 0;

    /** @return the monophonic channel to use. Check first if the cursor is used instead. The two are unrelated. */
    virtual int getMonophonicChannel() const noexcept = 0;
    /** @return the polyphonic channels to use. */
    virtual std::set<int> getPolyphonicChannels() const noexcept = 0;

    /**
     * Sets the monophonic channel to use.
     * @param channel the channel. Should be valid.
     */
    virtual void setMonophonicChannelToUse(int channel) noexcept = 0;
    /**
     * Sets the monophonic channel to use.
     * @param channels the channels. Should be valid.
     */
    virtual void setPolyphonicChannelsToUse(const std::set<int>& channels) noexcept = 0;

    /** @return true if the monophonic uses the cursor position. False if using a specific channel. */
    virtual bool isMonophonicUsesCursorPosition() const noexcept = 0;
    /**
     * Sets the flag to use the cursor position or not in the monophonic mode.
     * @param useCursorPosition true to use the cursor position, false to use a specific channel.
     */
    virtual void setMonophonicUsesCursorPosition(bool useCursorPosition) noexcept = 0;


    // ========================================================

    /** @return the possible effect context. */
    virtual const EffectContext* getEffectContext() noexcept = 0;
};

}   // namespace arkostracker

