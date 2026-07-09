#pragma once

namespace arkostracker 
{

/** Constants for the AKY. */
class AkyConstants
{
public:
    static constexpr auto registerBlockMaximumSize = 255;                            // The maximum size of a RegisterBlock.

    /** The loop byte (b3 = loop?). Must be followed by a DW to where to loop (a non-initial state only!). */
    static constexpr auto loopByte = 0b00001000;

    /** SID NIS byte. */
    static constexpr auto sidNisByte = 0b00010000;
};

}   // namespace arkostracker
