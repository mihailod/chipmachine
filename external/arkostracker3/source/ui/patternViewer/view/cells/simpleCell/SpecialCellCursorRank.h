#pragma once

namespace arkostracker 
{

/** Where the cursor can be in a "special" Cell. */
enum class SpecialCellCursorRank
{
    first = 0,

    cursorOnDigit2 = first,                   // Must be 0, else the CellView won't display the cursor correctly.
    cursorOnDigit1,

    count,
    notPresent = count,
    last = count - 1
};

}   // namespace arkostracker

