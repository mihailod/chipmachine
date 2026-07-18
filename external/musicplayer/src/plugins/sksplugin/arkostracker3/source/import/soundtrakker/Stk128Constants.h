#pragma once

namespace arkostracker 
{

/** Some constants about the .128 format. */
class Stk128Constants
{
public:
    /** Prevents instantiation. */
    Stk128Constants() = delete;

    static constexpr auto instrumentCount = 16;                  // Always 16 instruments.
    static constexpr auto arpeggioCount = 16;                    // Always 16 arpeggios.

    static constexpr auto offsetInstruments = 0x0;               // Offset where the Instruments are stored (not their names).
    static constexpr auto offsetArpeggios = 0x820;               // Offset where the Arpeggios data is stored.
    static constexpr auto offsetPatternsData = 0x0b80;           // Offset of the Patterns.
    static constexpr auto offsetSongName = 0xa90;
    static constexpr auto offsetInstrumentNames = 0x0a98;        // Offset where the Instrument names are stored.

    static constexpr auto instrumentNameSize = 8;

    static constexpr auto instrumentSize = 0x82;
    static constexpr auto offsetRepeatFlagsInInstrument = 0x80;  // Offset of the repeat flags INSIDE one Instrument.
    static constexpr auto offsetRepeatLengthInInstrument = 0x81; // Offset of the repeat length INSIDE one Instrument.
    static constexpr auto offsetVolumeTableInInstrument = 0;     // Offset of the volume table INSIDE one Instrument.
    static constexpr auto offsetNoiseTableInInstrument = 0x20;   // Offset of the noise table INSIDE one Instrument.
    static constexpr auto offsetToneTableInInstrument = 0x40;    // Offset of the tone table INSIDE one Instrument.

    static constexpr auto songNameSize = 8;
    static constexpr auto arpeggioSize = 32;
};

}   // namespace arkostracker
