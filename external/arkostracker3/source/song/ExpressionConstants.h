#pragma once

namespace arkostracker 
{

/** Constants about the Expressions. */
class ExpressionConstants
{
public:
    /** Prevents instantiation.*/
    ExpressionConstants() = delete;

    static constexpr auto maximumArpeggioOctave = 10;
    static constexpr auto minimumArpeggioOctave = -maximumArpeggioOctave;

    static constexpr auto maximumArpeggio = maximumArpeggioOctave * 12 - 1;          // Maximum value of an Arpeggio value. Only a few value missed...
    static constexpr auto minimumArpeggio = -maximumArpeggio;                        // Minimum value of an Arpeggio value.
    static constexpr auto maximumPitch = 0xfff;                                      // Maximum value of a Pitch value.
    static constexpr auto minimumPitch = -0xfff;                                     // Minimum value of a Pitch value.

    static constexpr auto minimumSpeed = 0;
    static constexpr auto maximumSpeed = 255;

    static constexpr auto minimumShift = 0;
    static constexpr auto maximumShift = 32;                                         // Because has a cost in memory...
};

}   // namespace arkostracker

