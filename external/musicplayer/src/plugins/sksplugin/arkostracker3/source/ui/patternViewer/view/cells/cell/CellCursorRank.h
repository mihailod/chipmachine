#pragma once

namespace arkostracker 
{

/** Where the cursor can be in a "normal" Cell. */
enum class CellCursorRank
{
    first = 0,
    cursorOnNote = first,                   // Must be 0, else the CellView won't display the cursor correctly.
    cursorOnInstrumentDigit2,
    cursorOnInstrumentDigit1,

    // MUST be contiguous!
    cursorOnEffect1Number,
    cursorOnEffect1Digit3,
    cursorOnEffect1Digit2,
    cursorOnEffect1Digit1,

    cursorOnEffect2Number,
    cursorOnEffect2Digit3,
    cursorOnEffect2Digit2,
    cursorOnEffect2Digit1,

    cursorOnEffect3Number,
    cursorOnEffect3Digit3,
    cursorOnEffect3Digit2,
    cursorOnEffect3Digit1,

    cursorOnEffect4Number,
    cursorOnEffect4Digit3,
    cursorOnEffect4Digit2,
    cursorOnEffect4Digit1,

    count,
    notPresent = count,
    last = count - 1
};

}   // namespace arkostracker

