#pragma once

#include "juce_core/juce_core.h"

namespace arkostracker 
{

class PlayerConfiguration;
class PsgPart;
class PsgInstrumentCell;
class SourceGenerator;
class LowLevelPsgInstrumentCell;

/** Class that encodes a given PSGInstrument Cell. */
class PsgInstrumentCellEncoder
{
public:
    /**
     * Constructor.
     * @param sourceGenerator object inside which the generated source is written.
     */
    explicit PsgInstrumentCellEncoder(SourceGenerator& sourceGenerator) noexcept;

    /**
     * Encodes the given PSG Instrument Cell.
     * @param playerConfiguration the Player Configuration to fill.
     * @param psgInstrumentCell the cell to encode.
     */
    void encodePsgInstrumentCell(PlayerConfiguration& playerConfiguration, const PsgPart& psgPart, int cellIndex) noexcept;

    /**
     * Encodes a loop to the specified label.
     * @param label the Label.
     */
    void encodeLoopToLabel(const juce::String& label) noexcept;

    /** Encodes a loop to the silence. */
    void encodeLoopToSilence() noexcept;

private:
    static const int linkNoSoftNoHardTag = 0b000;             // The tag to encode for the NoSoftNoHard link.
    static const int linkSoftOnlyTag = 0b001;                 // The tag to encode for the SoftOnly link.
    static const int linkSoftToHardTag = 0b010;               // The tag to encode for the SoftToHard link.
    static const int linkHardOnlyTag = 0b011;                 // The tag to encode for the HardOnly link.
    static const int linkHardToSoftTag = 0b100;               // The tag to encode for the HardToSoft link.
    static const int linkSoftAndHardTag = 0b101;              // The tag to encode for the SoftAndHard link.
    static const int linkLoopToSilence = 0b110;		          // The tag to encode for the loop (to silence).
    static const int linkLoop = 0b111;			              // The tag to encode for the loop (to the same Instrument).

    /** Encodes a "no soft no hard" Cell. */
    void encodeNoSoftNoHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;
    /** Encodes a "soft only" Cell. */
    void encodeSoftOnly(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;
    /** Encodes a "hard only" Cell. */
    void encodeHardOnly(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;
    /** Encodes a "soft to hard" Cell. */
    void encodeSoftToHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;
    /** Encodes a "hard to soft" Cell. */
    void encodeHardToSoft(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;
    /** Encodes a "soft to hard" Cell. */
    void encodeSoftAndHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
      * Encodes a "soft or hard" or "hard to soft", as the format is the same.
      * @param isSoftToHard true if soft to hard, false if hard to soft.
      * @param sourcePeriod the source period. May be 0 if forced.
      * @param sourceArpeggio the source arpeggio.
      * @param sourcePitch the source pitch.
      * @param destinationPitchShift the pitch shift of the destination.
      * @param cell the cell, to get the remaining data.
      */
    void encodeSoftToHardOrHardToSoft(PlayerConfiguration& playerConfiguration, bool isSoftToHard, int sourcePeriod, int sourceArpeggio, int sourcePitch,
                                      int destinationPitchShift, const LowLevelPsgInstrumentCell& cell) noexcept;

    /**
     * Encodes the period, if different to 0 (auto), else encodes the arpeggio and/or pitch (in this order).
     * @param period the period (0 for auto).
     * @param arpeggio the arpeggio;
     * @param pitch the pitch;
     */
    void encodePeriodOrPitchAndArpeggio(int period, int arpeggio, int pitch) noexcept;

    SourceGenerator& sourceGenerator;       // Object inside which the generated source is written.
};


}   // namespace arkostracker

