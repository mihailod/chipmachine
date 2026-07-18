#pragma once

#include <functional>
#include <juce_core/juce_core.h>

namespace arkostracker 
{

class Song;
class SongExportResult;
class SourceGenerator;
class LowLevelPsgInstrumentCell;

/** Encodes the Instruments for the AKM format. Note that LW also uses it! */
class AkmInstrumentEncoder
{
public:
    /**
     * Constructor.
     * @param song the song to export.
     * @param songExportResult object to fill with the result.
     * @param encodeInvertedRatio true to encode the ratio inverted (7 - ratio). True for LW, false for AKM.
     * @param getInstrumentLabel a function to return the label of an Instrument.
     */
    AkmInstrumentEncoder(const Song& song, SourceGenerator& sourceGenerator, SongExportResult& songExportResult,
                         bool encodeInvertedRatio, std::function<juce::String(int instrumentIndex)> getInstrumentLabel) noexcept;

    /**
     * Encodes an Instrument.
     * @return true if success, false if critical error.
     */
    bool encodeInstrument(int instrumentIndex) noexcept;

private:
    static const int maximumArpeggioValueInInstrument = 0x7f;                           // Maximum value of an Arpeggio value (in an Instrument).
    static const int minimumArpeggioValueInInstrument = -0x7f;                          // Minimum value of an Arpeggio value (in an Instrument).
    static const int maximumPitchValueInInstrument = 0xfff;                             // Maximum value of a Pitch value (in an Instrument).
    static const int minimumPitchValueInInstrument = -0xfff;                            // Minimum value of a Pitch value (in an Instrument).

    /**
     * Encodes the given Instrument Cell.
     * @param cell the Cell to encode.
     * @param instrumentIndex the Instrument index. Only to report the possible error.
     * @param cellIndex the Cell index. Only to report the possible error.
     * @param loopIndex the index where the sound loops, or -1 if it doesn't loop.
     */
    void encodeInstrumentCell(const LowLevelPsgInstrumentCell& cell, int instrumentIndex, int cellIndex, int loopIndex) noexcept;

    /**
     *  Encodes the given Instrument Cell, which has already been identified as a "no software, no hardware".
     *  @param cell the Cell to encode.
     */
    void encodeInstrumentCellNoSoftNoHard(const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
     * Encodes the given Instrument Cell, which has already been identified as a "software only".
     * @param cell the Cell to encode.
     */
    void encodeInstrumentCellSoftwareOnly(const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
     * Encodes the given Instrument Cell, which has already been identified as a "software to hardware".
     * @param cell the Cell to encode.
     */
    void encodeInstrumentCellSoftwareToHardware(const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
     * Encodes the given Instrument Cell, which has already been identified as a "software and hardware".
     * @param cell the Cell to encode.
     */
    void encodeInstrumentCellSoftwareAndHardware(const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
     * Shared code of the encoding of a Cell, both for "software and hardware" and "software to hardware".
     * @param cell the Cell to encode.
     */
    void encodeSharedInstrumentCellSoftwareAndToHardware(const LowLevelPsgInstrumentCell& cell, bool encodeRatio) noexcept;

    /**
     * Encodes the end, or loop, of an Instrument.
     * @param instrumentIndex the Instrument index. Only to report the possible error.
     * @param loop true if the Instrument loops.
     * @param loopIndex the index of the loop. Only used if there is a loop.
     */
    void encodeInstrumentEnd(int instrumentIndex, bool loop, int loopIndex) noexcept;


    /**
     * Checks the given arpeggio. If not valid, the arpeggio is modified to 0, false is returned and a warning is added to the StatusReport.
     * @param arpeggio the arpeggio. If not valid, it is modified to 0.
     */
    void checkArpeggioLimitAndCorrect(int& arpeggio) noexcept;

    /**
     * Checks the given pitch. If not valid, the pitch is modified to 0, false is returned and a warning is added to the StatusReport.
     * @param pitch the pitch. If not valid, it is modified to 0.
     */
    void checkPitchLimitAndCorrect(int& pitch) noexcept;

    /**
     * Checks that the software period is auto (0), the only supported mode in this format.
     * @param softwarePeriod the software period.
     */
    void checkSoftwarePeriod(int softwarePeriod) noexcept;

    /**
     * @return the label of an Instrument, with a loop prefix.
     * @param instrumentIndex the Instrument index.
     */
    juce::String getInstrumentLabelWithLoop(int instrumentIndex) noexcept;

    const Song& song;
    SourceGenerator& sourceGenerator;
    SongExportResult& songExportResult;
    bool encodeInvertedRatio;                                                   // True to encode the ratio inverted(7 - ratio).
    std::function<juce::String(int)> getInstrumentLabel;
};


}   // namespace arkostracker

