#pragma once

#include <set>

#include "../business/song/tool/context/EffectContext.h"
#include "../song/cells/Cell.h"
#include "PlayerController.h"
#include "observers/CursorObserver.h"
#include "observers/GeneralDataObserver.h"
#include "observers/SelectedBlockObserver.h"
#include "observers/SubsongMetadataObserver.h"

namespace arkostracker 
{

class MainController;
class SongController;
class SongPlayer;

/** Implementation of the Player Controller. */
class PlayerControllerImpl final : public PlayerController,
                                   public SubsongMetadataObserver,                // To be aware of a change of loop. If playing, must adapt to it.
                                   public SelectedBlockObserver,
                                   public CursorObserver,                         // To know where the cursor is (for monophony).
                                   public GeneralDataObserver
{
public:
    /**
     * Constructor.
     * @param mainController the main Controller, to reach the Song Player.
     */
    explicit PlayerControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~PlayerControllerImpl() override;

    // PlayerController method implementations.
    // ============================================
    int getCurrentOctave() const noexcept override;
    void setOctave(int desiredOctave) noexcept override;

    void playSongFromStart() noexcept override;
    void playSongFromCurrentLocation() noexcept override;
    void playPatternFromStart() noexcept override;
    void playPatternFromShownLocationOrBlock() noexcept override;
    void playSongFromLocation(const Location& location) noexcept override;
    void playLine(const Location& location) noexcept override;
    void setCurrentlyPlayedLocation(const Location& location) noexcept override;
    void stopPlaying() noexcept override;
    bool isPlaying() const noexcept override;
    bool isPlayingOnPattern() const noexcept override;

    Observers<SongPlayerObserver>& getSongPlayerObservers() noexcept override;

    void play(OptionalInt newNote, bool isBaseNote, OptionalId newInstrumentId) noexcept override;
    void playFromEditor(int note, OptionalInt instrumentIndex, CellLocationInPosition location) noexcept override;
    void playFromEditor(const Cell& cell, CellLocationInPosition location) noexcept override;
    void setPlayedLocation(const Location& location) noexcept override;

    bool isSelectedArpUsedWhenPlayingNote() const noexcept override;
    bool isSelectedPitchUsedWhenPlayingNote() const noexcept override;
    bool isUsingMonophony() const noexcept override;
    bool toggleIsSelectedArpUsed() noexcept override;
    bool toggleIsSelectedPitchUsed() noexcept override;
    bool toggleIsUsingMonophony() noexcept override;
    int getMonophonicChannel() const noexcept override;
    std::set<int> getPolyphonicChannels() const noexcept override;
    void setMonophonicChannelToUse(int channel) noexcept override;
    void setPolyphonicChannelsToUse(const std::set<int>& channels) noexcept override;
    bool isMonophonicUsesCursorPosition() const noexcept override;
    void setMonophonicUsesCursorPosition(bool useCursorPosition) noexcept override;

    const EffectContext* getEffectContext() noexcept override;

    // MidiController::MidiListener method implementations.
    // =======================================================
    void onMidiNoteReceived(int noteNumber) override;
    void onMidiProgramChangeReceived(int programChange) override;

    // SubsongMetadataObserver method implementations.
    // ======================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) override;

    // SelectedBlockObserver method implementations.
    // =======================================================
    void onNewSelectedBlock(juce::Range<int> range) noexcept override;

    // CursorObserver method implementations.
    // =======================================================
    void onCursorChannelMoved(const CursorLocation& cursorLocation) override;

    // GeneralDataObserver method implementations.
    // =======================================================
    void onGeneralDataChanged(unsigned int whatChanged) override;

private:
    static const int maximumBaseOctave;
    static const int middleOctave;          // The octave to subtract when adding it to the MIDI note.

    /**
     * @return the played location, loop Start and End locations from the given one. It may be out of bounds or not.
     * @param locationToGoTo where to play from.
     */
    std::tuple<Location, Location, Location> getLocationsToPlayFromLocationToGoTo(const Location& locationToGoTo) const noexcept;

    /**
     * Makes the selected block being the first line, if no selected block is valid.
     * This is useful when playing the song/pattern from start, so than a subsequent Play in the PV re-starts at the top
     * and not from where the cursor was, not very user-friendly.
     */
    void resetStoredSelectedBlockIfEmpty() noexcept;

    /** @return the valid channel to use for mono/polyphony. Also skips mute channels. */
    int determineChannelToUse() noexcept;

    /** Sends an event to the observers. */
    void sendObserverEvent(SongPlayerObserver::Event event) noexcept;

    /** @return true if the octave must be added to the input MIDI notes. */
    bool mustAddOctaveToInputMidiNote() noexcept;

    MainController& mainController;
    SongController& songController;
    std::unique_ptr<EffectContext> effectContext;
    SongPlayer& songPlayer;

    int currentOctave;                                      // The base octave.
    int currentTestNote;                                    // The note, as used in the test area for example.
    bool useSelectedArp;                                    // If false, forced to "no arp" when playing a note.
    bool useSelectedPitch;                                  // If false, forced to "no pitch" when playing a note.

    bool useMonophony;                                      // If false, uses polyphony, else use monophony.
    bool monophonicUsesCursorPosition;                      // True to use the cursor position in monophony. False to use a specific channel for monophony.
    int monophonicChannelIndex;                             // The specific channel used for monophony. Out of bounds channel is handled.
    std::set<int> polyphonicChannelIndexes;                 // The channels to use for polyphony. Out of bounds channels are handled.
    size_t polyphonicCounter;                               // An increasing counter to indicate what polyphonic channel to use.

    juce::Range<int> selectedBlock;                         // Used when wanting to play a block selection. Empty means "from cursor", and the start is the line.

    int cursorChannelIndex;                                 // Used for monophony, if using cursor location.

    bool playingOnPattern;                                  // True if the latest PlayXXX loops on one Pattern (including block).
    OptionalBool cachedMustAddOctaveToInputMidiNote;
};

}   // namespace arkostracker
