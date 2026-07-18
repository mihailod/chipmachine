#pragma once

#include <functional>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../song/Location.h"
#include "../../../song/cells/EffectError.h"
#include "../../../song/cells/Note.h"
#include "../../../utils/OptionalValue.h"
#include "../CursorLocation.h"
#include "dataItem/Digit.h"
#include "dataItem/EffectDigit.h"
#include "dataItem/EffectNumber.h"

namespace arkostracker 
{
class PatternViewerControllerImpl;
class Cell;
class CellEffect;
class SelectedData;

/** Helps the PatternViewerControllerImpl to manage the cursor, as a friend class. */
class CursorManager
{
public:
    /**
     * Constructor.
     * @param controller the controller, friend class.
     */
    explicit CursorManager(PatternViewerControllerImpl& controller) noexcept;

    /**
     * Manages a note being received. May only play the note and/or write it, move the cursor onwards.
     * The behavior depends on the Record and Playing state.
     * The note may be received from the PC keyboard, or a MIDI keyboard.
     * @param note the note.
     * @param forcedInstrument the possible instrument, else uses the current one or the one from the Track (if Write Instrument is off).
     * This is useful to write RST for example.
     */
    void manageNoteReceived(int note, OptionalInt forcedInstrument = {}) const noexcept;

    /**
     * Called when an item must be deleted (via DEL, as the follow/edit step will be managed).
     * @param selectedData the data to delete.
     */
    void manageItemsToDelete(const SelectedData& selectedData) const noexcept;

    /**
     * Called when a key is pressed. This is called BEFORE JUCE manages any command, so it takes precedence over them.
     * @param key the pressed key.
     * @return true if the key is consumed. False to let the parent handle it.
     */
    bool onKeyPressed(const juce::KeyPress& key) const noexcept;

    /** @return the text to display in the information text, and true if there is an error. */
    std::pair<bool, juce::String> getInformationText() const noexcept;

    /** Builds a list of Cursor Locations, from left to right, taking the channel count and the minimized states in account. */
    std::vector<CursorLocation> buildCursorLocations() const noexcept;

    /** @return the text to display for the current effect context, for the given channel index (where the cursor is). May be empty. */
    std::pair<int, juce::String> buildEffectContextText() const noexcept;

private:

    /**
     * Manages a digit typed by the user.
     * @param digit the digit (0-15).
     * @return true if the digit has been used. False if it couldn't be used here.
     */
    bool manageTypedDigitIfPossible(int digit) const noexcept;

    /**
     * Managed an effect typed by the user.
     * @param effect the effect.
     * @return true if the effect has been used. False if it couldn't be used here.
     */
    bool manageTypedEffectIfPossible(Effect effect) const noexcept;

    /**
     * Manages a digit typed by the user on a normal Track.
     * @param digit the digit (0-15).
     * @return true if the digit has been used. False if it couldn't be used here.
     */
    bool manageTypedDigitIfPossibleOnTrack(int digit) const noexcept;

    /**
     * Manages a digit typed by the user on a Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param digit the digit (0-15).
     * @return true if the digit has been used. False if it couldn't be used here.
     */
    bool manageTypedDigitIfPossibleOnSpecialTrack(bool isSpeedTrack, int digit) const noexcept;

    /**
     * We want to write cell data (note, instrument, effects...). If not recording, nothing happens.
     * The note being played, location increased by the step, etc. are managed here.
     * @param channelIndex the channel index. Must be valid.
     * @param newNote the note, or absent not to write any.
     * @param newInstrument the instrument, or absent not to write any.
     * @param newEffectDigit the possible effect digit.
     * @param newEffectNumber the possible effect number.
     * @param newInstrumentDigit the possible instrument digit.
     */
    void manageItemsToWrite(int channelIndex, OptionalValue<Note> newNote, OptionalInt newInstrument = { }, OptionalValue<EffectDigit> newEffectDigit = { },
                            OptionalValue<EffectNumber> newEffectNumber = { }, OptionalValue<Digit> newInstrumentDigit = { }) const noexcept;

    /** Generic management to write cell data (note, etc.) but also deletion. Recording/follow, goto next step etc. is managed here. */
    void manageWrite(const std::function<void(const Location& whereToWrite)>& modifyCellAction, const std::function<void(const Location& whereToWrite)>& playCellIfNote)
        const noexcept;

    /**
     * We want to write a special cell data. If not recording, nothing happens.
     * The location being increased by the step, etc. is managed here.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param digit the digit.
     */
    void manageSpecialItemsToWrite(bool isSpeedTrack, const Digit& digit) const noexcept;

    /**
     * Finds out what effect is related to the given key. It is compared to the user defined effect keys.
     * @param key the key.
     * @return the possible effect.
     */
    OptionalValue<Effect> determineEffectFromKey(const juce::KeyPress& key) const noexcept;

    /**
     * Determines the text to display when the cursor is over an effect digit/number.
     * @param rank the rank of the cursor. Must be on an effect number.
     * @param cell the Cell.
     * @return the error state, and the text.
     */
    std::pair<bool, juce::String> determineInformationTextOnEffect(CellCursorRank rank, const Cell& cell) const noexcept;

    /**
     * Determines the text to display when there is an error on an effect.
     * @param effectError the effect error.
     * @return the error state, and the text.
     */
    static std::pair<bool, juce::String> determineInformationTextOnEffectError(EffectError effectError) noexcept;

    /**
     * Determines the text if the given effect is an arpeggio/pitch table.
     * @param cellEffect the cell effect. MUST contain an arpeggio/pitch table.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @return the error state, and the text.
     */
    std::pair<bool, juce::String> determineInformationTextForExpression(const CellEffect& cellEffect, bool isArpeggio) const noexcept;

    PatternViewerControllerImpl& controller;
};

}   // namespace arkostracker
