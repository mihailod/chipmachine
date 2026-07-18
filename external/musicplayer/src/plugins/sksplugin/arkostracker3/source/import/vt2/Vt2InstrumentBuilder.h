#pragma once

#include <utility>

#include <juce_core/juce_core.h>

#include "../../song/instrument/psg/PsgInstrumentCell.h"

namespace arkostracker 
{

class SongBuilder;

/** Helps builds an Instrument from the VT2 format. */
class Vt2InstrumentBuilder
{
public:
    /**
     * Constructor.
     * @param songBuilder the Song Builder. The instrument has already been started.
     * @param instrumentName the name of the Instrument.
     */
    Vt2InstrumentBuilder(SongBuilder& songBuilder, juce::String instrumentName) noexcept;

    /**
    * Parses the given line and creates the matching Instrument Cell. The Cell results 
    * @param line the line to parse.
    */
    void addInstrumentLine(const juce::String& line) noexcept;

    /**
     * Called when all the lines has been added. The Instrument is marked as ended.
     * @return true if the parsing went well, false if critical error.
     */
    bool parseEnded() noexcept;

private:
    /**
     * Parses the given instrument line and generates an Instrument Cell, also according to the previous shifts.
     * @param line the line to parse.
     * @param successOutput true if everything went fine.
     * @param foundIncDecOutput true if, on this line, an inc/dec of the pitch/volume/noise has been found.
     */
    PsgInstrumentCell parseInstrumentLine(const juce::String& line, bool& successOutput, bool& foundIncDecOutput) noexcept;

    /**
     * Builds an Instrument Cell from the given parameters, and the currently stored period and noise.
     * @param isTone true if there is a Tone.
     * @param isNoise true if there is a Noise.
     * @param shiftPeriod shift of the Period.
     * @param isShiftPeriodAddition true if the shiftPeriod is an addition to the current shift period.
     * @param noise the noise, or 0.
     * @param isNoiseAddition true if the noise is an addition.
     * @param volume the volume.
     * @param isVolumeIncrease true if the volume is increasing.
     * @param isVolumeDecrease true if the volume is decreasing.
     * @return the PSG Instrument Cell.
     */
    PsgInstrumentCell buildInstrumentCell(bool isTone, bool isNoise, int shiftPeriod, bool isShiftPeriodAddition,
                                          int noise, bool isNoiseAddition,
                                          int volume, bool isVolumeIncrease, bool isVolumeDecrease) noexcept;

    /**
     * @return the loopTo index (0 if not looping), and a flag indicating whether the sound originally loops.
     */
    std::pair<int, bool> findLoopToIndex() const noexcept;

    /**
     * Compares the cells.
     * @param cells1 the first cells.
     * @param cells2 the second cells.
     * @return true if all the cells are identical, and if of course they are as many.
     */
    static bool areCellsDifferent(const std::vector<PsgInstrumentCell>& cells1, const std::vector<PsgInstrumentCell>& cells2) noexcept;

    /**
     * Encodes a Cell into an already declared Instrument.
     * @param cell the Cell to encode.
     */
    void encodeInstrumentCell(const PsgInstrumentCell& cell) const noexcept;

    SongBuilder& songBuilder;
    juce::String instrumentName;                // THe name of the Instrument.
    std::vector<juce::String> lines;            // The stored lines of the Instrument.

    int currentShiftPeriod;                     // The current shift in the software period.
    int currentShiftNoise;                      // The current shift in the noise.
    int currentShiftVolume;                     // The current shift in the volume. 0, or positive to increase the volume, or negative.
};

}   // namespace arkostracker
