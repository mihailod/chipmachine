#pragma once

#include "browser/CellAndLocation.h"

namespace arkostracker 
{

class Song;

/** Utility class to change the ordering of the Expressions in a Song. The new mapping must be given to this class. */
class ChangeExpressionsOrdering             // TODO TU this.
{
public:
    /** Prevents instantiation. */
    ChangeExpressionsOrdering() = delete;

    /**
     * Changes the Expressions in the Song Tracks, changing them or removing them.
     * @param isArpeggio true for Arpeggio, false for Pitches.
     * @param song the Song. It will be modified
     * @param mapping a mapping of indexes. If not found, it means the expression is deleted.
     * @return the modified Cells, and their location (useful for Undo).
     */
    static std::vector<CellAndLocation> changeExpressionsInSong(bool isArpeggio, Song& song, const std::unordered_map<int, OptionalInt>& mapping) noexcept;
};



}   // namespace arkostracker

