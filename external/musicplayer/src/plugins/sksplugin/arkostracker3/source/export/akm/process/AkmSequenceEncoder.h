#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

#include "../../../song/tracks/Track.h"

namespace arkostracker 
{

class SongExportResult;

/** Encodes a sequence (Track) into the AKM format. */
class AkmSequenceEncoder
{
public:
    /** Prevents instantiation. */
    AkmSequenceEncoder() = delete;

    // This one must be first, because it may not be used if no effects, so will be replaced by a note index.
    static constexpr auto valueNoteAndEffects = 12;                                              // Value indicating there is a note, and there are effects.

    /**
     * Encodes the Cells of a whole Track.
     * @param songExportResult to reach the status report.
     * @param track the Track to encode.
     * @param areEffectsPresentInSong true if there are at least one effect used.
     * @param primaryDefaultInstrumentIndex the index of the most used instrument.
     * @param secondaryDefaultInstrumentIndex the index of the second most used instrument.
     * @param primaryWait the most used wait value.
     * @param secondaryWait the second most used wait value.
     * @param startNoteInTrack a default Instrument used as default at the beginning of Track.
     * @param startInstrumentInTrack a start Instrument used as default at the beginning of Track.
     * @param startWaitTrack a start Wait used as default at the beginning of Track.
     * @param mostUsedNoteIndexes from most to least used notes.
     * @return the bytes to encode.
     */
    static std::vector<std::pair<unsigned int, juce::String>> encodeTrack(SongExportResult& songExportResult, const Track& track,
                                                   bool areEffectsPresentInSong,
                                                   int primaryDefaultInstrumentIndex, int secondaryDefaultInstrumentIndex, int primaryWait, int secondaryWait,
                                                   int startNoteInTrack, int startInstrumentInTrack, int startWaitTrack,
                                                   const std::vector<int>& mostUsedNoteIndexes) noexcept;

private:
    static constexpr auto valueNoNoteMaybeEffects = valueNoteAndEffects + 1;                     // Value indicating there is no note. There may be effects.
    static constexpr auto valueNewEscapeNote = valueNoNoteMaybeEffects + 1;                      // Value indicating the note must be escaped.
    static constexpr auto valueSameEscapeNote = valueNewEscapeNote + 1;                          // Value indicating the note is the same (it is an escape note).

    static constexpr auto instrumentFlagsSameEscape = 0b00;                          // The same escape instrument (so NOT primary or secondary!)
    static constexpr auto instrumentFlagsPrimary = 0b01;
    static constexpr auto instrumentFlagsSecondary = 0b10;
    static constexpr auto instrumentFlagsNewInstrument = 0b11;                       // New, but still NOT primary or secondary!

    static constexpr auto waitFlagsSameEscape = 0b00;                                // The same escape wait (so NOT primary or secondary!)
    static constexpr auto waitFlagsPrimary = 0b01;
    static constexpr auto waitFlagsSecondary = 0b10;
    static constexpr auto waitFlagsNewWait = 0b11;                                   // New, but still NOT primary or secondary!

    static constexpr auto instrumentFlagsWhenNoNoteButEffect = 0b01;                 // Instrument flag, diverted, when "no note but effect".

    static constexpr unsigned int effectNumberResetWithInvertedVolume = 0;          // Number of effect for reset with inverted volume.
    static constexpr unsigned int effectNumberVolume = 1;                           // Number of effect for volume.
    static constexpr unsigned int effectNumberPitchUpDown = 2;                      // Number of effect for (fast)pitch up/down.
    static constexpr unsigned int effectNumberArpeggioTableEffect = 3;              // Number of effect for arpeggio table.
    static constexpr unsigned int effectNumberPitchTableEffect = 4;                 // Number of effect for pitch table.
    static constexpr unsigned int effectNumberForceInstrumentSpeedEffect = 5;       // Number of effect for Force instrument speed.
    static constexpr unsigned int effectNumberForceArpeggioSpeedEffect = 6;         // Number of effect for Force arpeggio speed.
    static constexpr unsigned int effectNumberForcePitchSpeedEffect = 7;            // Number of effect for Force pitch speed.

    static const std::set<Effect> knownEffects;                                 // The effects known in this format.

    /** Cell with the possible wait after it. */
    class AccumulatedCell
    {
    public:
        /** Constructor to use when no Cell has been read yet. */
        AccumulatedCell() :
                set(false),
                cell(),
                accumulatedWait(0)
        {
        }
        /**
         * Constructor to use when a Cell is found.
         * @param pCell the Cell.
         */
        explicit AccumulatedCell(Cell pCell) :
                set(true),
                cell(std::move(pCell)),
                accumulatedWait(0)
        {
        }

        void increaseAccumulatedWait() {
            ++accumulatedWait;
        }

        bool isSet() const
        {
            return set;
        }

        const Cell& getCell() const
        {
            return cell;
        }

        int getAccumulatedWait() const
        {
            return accumulatedWait;
        }

        void reset()
        {
            set = false;
            cell = Cell();
            accumulatedWait = 0;
        }

        void forceWaitToMax()
        {
            accumulatedWait = 127;
        }

    private:
        bool set;                   // The Cell has been set.
        Cell cell;
        int accumulatedWait;
    };

    /**
     * Encodes the cell, which may contain the Waits that are after.
     * @param outputData the data to fill.
     * @param accumulatedCell the Cell. If not "set", nothing is done.
     * @param areEffectsPresentInSong true if there are at least one effect used.
     * @param currentNote the current note, or -1 if not known. May be modified.
     * @param currentInstrumentIndex the current instrument index, or -1 if not known. May be modified.
     * @param currentWait the current wait, or -1 if not known. May be modified.
     * @param encodeWaits true to encode the Waits, if there are present.
     * @param primaryDefaultInstrumentIndex the index of the most used instrument.
     * @param secondaryDefaultInstrumentIndex the index of the second most used instrument.
     * @param primaryWait the most used wait value.
     * @param secondaryWait the second most used wait value.
     * @param mostUsedNoteIndexes from most to least used notes.
     */
    static void encodeAccumulatedCell(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult,
                               const AkmSequenceEncoder::AccumulatedCell& accumulatedCell,
                               bool areEffectsPresentInSong,
                               int& currentNote, int& currentInstrumentIndex, int& currentWait, bool encodeWaits,
                               int primaryDefaultInstrumentIndex, int secondaryDefaultInstrumentIndex,
                               int primaryWait, int secondaryWait,
                               const std::vector<int>& mostUsedNoteIndexes) noexcept;

    /**
     * Encodes the effects, if they are present.
     * @param outputData the data to fill.
     * @param cell the Cell which effects must be encoded.
     */
    static void encodeEffects(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult, const Cell& cell) noexcept;

    /**
    * @return the encoded first byte for an effect. It may contain a quartet data (useful for small data like volume, etc.).
    * @param effectNumber the effect number (as the Z80 player knows it).
    * @param effectValueQuartet a possible 4-bit value to encode in the effect first byte.
    * @param moreEffects true if there are more effects to encode after this one.
    */
    static int getEncodedEffectFirstByte(unsigned int effectNumber, int effectValueQuartet, bool moreEffects) noexcept;

    /**
     * Encodes a pitch up/down, if any. The pitch may be fast or slow.
     * @param outputData the data to fill.
     * @param effect the effect.
     * @param effectValue the value.
     * @param moreEffects true if there are more effects to encode after this one.
     */
    static void encodeFastOrSlowPitch(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult,
                                      Effect effect, int effectValue, bool moreEffects) noexcept;

    /**
     * Encodes a pitch, if any.
     * @param outputData the data to fill.
     * @param isPitchUsed true if there is a pitch. If not, nothing is done.
     * @param pitchValue a POSITIVE pitch, even if it goes down. May be 0.
     * @param up true if pitch up, false if pitch down.
     */
    static void encodePitch(std::vector<std::pair<unsigned int, juce::String>>& outputData, bool isPitchUsed, int pitchValue, bool up) noexcept;

    /**
     * Encodes an effect with one byte, which can be "escaped" if higher than 14.
     * @param outputData the data to fill.
     * @param effectNumber the effect number (as the Z80 player knows it).
     * @param effectValueWithoutShift the effect value.
     * @param moreEffects true if there are more effects to encode after this one.
     * @param comment the comment.
     */
    static void encodeEffectWithMaybeEscapeValue(std::vector<std::pair<unsigned int, juce::String>>& outputData, unsigned int effectNumber,
                                          int effectValueWithoutShift, bool moreEffects, const juce::String& comment) noexcept;

    /**
     * Removes the unknown effects from the given cell, raising a warning if this happens.
     * @param cell the Cell. Will be modified if needed.
     * @param songExportResult the export result to raise the warnings.
     */
    static void removeUnknownEffects(Cell& cell, SongExportResult& songExportResult) noexcept;
};


}   // namespace arkostracker

