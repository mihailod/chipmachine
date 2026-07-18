#pragma once

#include <map>
#include <utility>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../../song/cells/Cell.h"

namespace arkostracker 
{

/**
 * Reads a JUCE Midi track, and converts it to a sequence of Cells.
 * Also holds maps to instruments, speed and time signature.
 *
 * The same object should be used for one Song, as it should be given each MIDI Track in order to gather the
 * Instruments, Speed change and so on, to use.
 */
class MidiTrackReader
{
public:
    /** The type of the MIDI instrument, to know what kind of sound to generate. */
    enum class MidiInstrumentType
    {
        shortSound,
        longSound,
        drumBassdrum,
        drumSnare,
        drumCrash,
        drumHihat,
        drumTomHigh,
        drumTomMedium,
        drumTomLow,
        /** Any other drum. */
        drumGeneric,
    };

    /** Information about the current MIDI instrument. */
    class MidiInstrument
    {
    public:
        MidiInstrument(MidiInstrumentType pMidiInstrumentType, juce::String pName) :
            midiInstrumentType(pMidiInstrumentType),
            name(std::move(pName))
        {}

        const MidiInstrumentType midiInstrumentType;        // The type of the MIDI instrument, to know what kind of sound to generate.
        const juce::String name;                            // The name (general MIDI) of the instrument.
    };

    /**
    * Constructor.
    * @param timeFormat the time format, inside the Midi file (see the Juce getTimeFormat method on the MidiFile).
    * @param ppq the "parts per quarter" to use (24, 120, etc.) to regroup events into Cells.
    * @param drumChannel the channel where the drums are (10 most of the time, or 16).
    * @param importVelocities true to import note velocities.
    */
    MidiTrackReader(short timeFormat, int ppq, int drumChannel, bool importVelocities) noexcept;

    /**
     * Reads the given JUCE Midi Track and extracts Cells to be inserted into a Track.
     * Of course, the returned Cells are not fit to a track, since the whole song can be potentially in this map.
     * @param midiMessageSequence the midi Track.
     * @param isConductorTrack if true, this track is the "conductor" Track (only time signature), so all notes are ignored.
     * @return a map linking a cell index to its Cell.
     */
    std::map<int, Cell> readMidiTrack(const juce::MidiMessageSequence& midiMessageSequence, bool isConductorTrack = false) noexcept;

    /** @return a reference to the map linking the cell index to a BPM change. */
    const std::map<int, int>& getCellIndexToBpm() const noexcept;

    /** @return the index of the last Cell found. */
    int getLastCellIndex() const noexcept;

    /**
     * Builds the heights of the Pattern of the song, according to the time signature, but also the notes of course.
     * This must be called after all the Tracks are read.
     * @return the heights of the Pattern.
     */
    std::vector<int> buildPatternHeights() const noexcept;

    /** @return the map linking all the Instruments into what they are. */
    const std::map<int, MidiInstrument>& getInstrumentIndexToMidiInstrumentType() const noexcept;

    /**
     * Builds the raw heights of the Pattern of the song, according to the time signature.
     * "Raw" because the patterns can still be very small, which is not friendly.
     * This must be called after all the Tracks are read.
     * Public for testing... Please bear with me (AT2 code, I have no time to change it).
     * @return the heights of the Pattern, and the advised height for the time signature (4/4 -> 16 for example).
     */
    std::vector<std::pair<int, int>> buildRawPatternHeights() const noexcept;

    /** Only used for testing... Please bear with me (AT2 code, I have no time to change it). */
    void setLastCellIndex(int lastCellIndex) noexcept;
    /** Only used for testing... Please bear with me (AT2 code, I have no time to change it). */
    void addCellIndexToTimeSignature(int cellIndex, const std::pair<int, int>& timeSignature) noexcept;

private:
    /**
     * A note On is found. Stores it in the given map, if the slot is not already used.
     * Stores the current Program Change in the Instrument map. Creates an entry, if needed, linking the PC to
     * a new Instrument.
     * @param cellIndexToCell the map linking a cell index to its Cell, to fill.
     * @param midiMessage the read midi message.
     * @param cellIndex the cell index this item could be inserted to.
     * @param currentProgramChange the current Program Change.
     */
    void onNoteOnFound(std::map<int, Cell>& cellIndexToCell, const juce::MidiMessage& midiMessage, int cellIndex, int currentProgramChange) noexcept;

    /**
     * A note On is found. Stores it in the given map, if the slot is not already used.
     * Stores the current Program Change in the Instrument map. Creates an entry, if needed, linking the PC to
     * a new Instrument.
     * @param cellIndexToCell the map linking a cell index to its Cell, to fill.
     * @param midiMessage the read midi message.
     * @param cellIndex the cell index this item could be inserted to.
     */
    void onNoteOnFoundForDrum(std::map<int, Cell>& cellIndexToCell, const juce::MidiMessage& midiMessage, int cellIndex) noexcept;

    /**
     * Encodes a Cell in the given Midi cells map. The velocity of the given Midi Message is also encoded.
     * @param cellIndexToCell the map.
     * @param cellIndex the Midi cell index.
     * @param noteNumber the note number.
     * @param instrumentIndex the instrument.
     * @param midiMessage the Mid Message.
     */
    void encodeCell(std::map<int, Cell>& cellIndexToCell, int cellIndex, int noteNumber, int instrumentIndex, const juce::MidiMessage& midiMessage) const noexcept;

    /**
     * A Program Change is found. Simple stores it.
     * @param midiMessage the read midi message.
     * @param currentProgramChange the program change to update.
     */
    static void onProgramChangeFound(const juce::MidiMessage& midiMessage, int& currentProgramChange) noexcept;

    /**
     * A Time Signature is found. Stores this in a specific map.
     * @param midiMessage the read midi message.
     * @param cellIndex the cell index this item could be inserted to.
     */
    void onTimeSignatureFound(const juce::MidiMessage& midiMessage, int cellIndex) noexcept;

    /**
     * A tempo is found. Converts and stores this in a specific map.
     * @param midiMessage the read midi message.
     * @param cellIndex the cell index this item could be inserted to.
     */
    void onTempoFound(const juce::MidiMessage& midiMessage, int cellIndex) noexcept;

    /**
     * Updates the last cell index, if the one given is higher.
     * @param cellIndex the possibly new cell index.
     */
    void updateLastCellIndex(int cellIndex) noexcept;

    /**
     * Calculates an advised pattern height, from the given time signature (4/4 for example).
     * @param timeSignatureNumerator the numerator.
     * @param timeSignatureDenominator the denominator.
     * @return the height (<128).
     */
    static int calculatePatternAdvisedHeight(int timeSignatureNumerator, int timeSignatureDenominator) noexcept;

    /**
     * Returns the ideal height, from the signature height. For example 16 will return 64, 12 with return 48.
     * @param signatureHeight the signature height.
     * @return the ideal height.
     */
    static int findIdealHeight(int signatureHeight) noexcept;

    /**
     * Finds what MidiInstrument data matching the program change (for notes only, not drums).
     * @param programChange the program change number.
     * @return the data.
     */
    static MidiInstrument findMidiInstrumentDataForNote(int programChange) noexcept;

    /**
     * Finds what MidiInstrument data matching the note number (for drums only).
     * @param noteNumber the note number. Useful to if it's a drum.
     * @return the data.
     */
    static MidiInstrument findMidiInstrumentDataForDrum(int noteNumber) noexcept;

    /**
     * Indicates whether the midi message is about a drum.
     * @param midiMessage the MIDI message. It should be a note channel.
     * @return true if the note is on the drum channel.
     */
    bool isDrum(const juce::MidiMessage& midiMessage) const noexcept;

    short timeFormat;                                                   // The 16-bit time format from the MIDI file header.
    int ppq;                                                            // The "parts per quarter" to use (24, 120, etc.) to regroup events into Cells.

    std::map<int, int> programChangeToInstrumentIndex;                  // Map linking the Program changes to their Instrument counterpart.
    std::map<int, int> drumIndexToInstrumentIndex;                      // Map linking a drum note index (0-127) to their Instrument counterpart.
    int highestInstrument;                                              // The highest current instrument.

    std::map<int, std::pair<int, int>> cellIndexToTimeSignature;        // Map linking the a cell index to a new time signature (numerator/denominator).
    std::map<int, int> cellIndexToBpm;                                  // Map linking the a cell index to a BPM change.
    std::map<int, MidiInstrument> instrumentIndexToMidiInstrument;      // Map linking an instrument index (from the real Song) to the instrument type to create.

    int lastCellIndex;                                                  // The last Cell index.
    int drumChannel;                                                    // The channel where the drums are encoded (10 or 16 generally).
    bool importVelocities;                                              // True to import note velocities.
};


}   // namespace arkostracker

