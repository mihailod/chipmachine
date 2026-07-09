#pragma once

#include <vector>

#include "browser/CellAndLocation.h"
#include "browser/SpecialCellAndLocation.h"

namespace arkostracker 
{

class Song;

/** Utility class to change the ordering of the Instruments in a Song. The new mapping must be given to this class. */
class ChangeInstrumentsOrdering
{
public:
    /** Prevents instantiation. */
    ChangeInstrumentsOrdering() = delete;

    /**
     * Changes the Instruments in the Song Tracks, changing them or removing them.
     * @param song the Song. It will be modified
     * @param mapping a mapping of indexes (from old to new). Absent means the instrument is deleted.
     * @param oldSampleIndexesForSpecialEvent the indexes of the sample to remap in the Event Tracks. If empty, they won't be modified.
     * The indexes present here will be mapped according to the mapping.
     * @return the modified Cells, and their location (useful for Undo).
     */
    static std::pair<std::vector<CellAndLocation>, std::vector<SpecialCellAndLocation>> changeInstrumentsInSong(
            Song& song, const std::unordered_map<int, OptionalInt>& mapping,
            const std::unordered_set<int>& oldSampleIndexesForSpecialEvent) noexcept;
};

}   // namespace arkostracker
