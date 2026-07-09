#pragma once

namespace arkostracker 
{

/** Constants about the Cells. */
class CellConstants
{
public:
    /** Prevents instantiation.*/
    CellConstants() = delete;

    static constexpr auto noteCountInOctave = 12;                                // How many notes in an octave.
    static constexpr auto lastNoteInOctave = noteCountInOctave - 1;              // The last note in an octave.
    static constexpr auto firstNoteInOctave = 0;                                 // Only to have a "last" counterpart.

    static constexpr auto effectCount = 4;                                       // How many effects there are.
    static constexpr auto noEffectNumber = 0;                                    // Effect number which means "no effect".
    static constexpr auto maximumEffectValue = 0xfff;                            // The maximum effect value.
    static constexpr auto minimumNote = 0;                                       // The minimum note.
    static constexpr auto maximumNote = 127;                                     // The maximum note. 127 to fill all the 8 bits, which is the player's limit.
    static constexpr auto rstNote = 4 * 12;                                   // Only a convention, must not be used in comparisons.
    static constexpr auto rstInstrument = 0;                                     // A RST has this instrument.

    static constexpr auto minimumInstrumentExceptRst = rstInstrument + 1;
    static constexpr auto maximumInstrument = 0xff;
};

}   // namespace arkostracker
