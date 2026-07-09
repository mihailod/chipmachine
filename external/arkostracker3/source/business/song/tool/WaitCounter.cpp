#include "WaitCounter.h"

#include "../../../song/Song.h"
#include "browser/TrackBrowser.h"

namespace arkostracker 
{

OccurrenceMap<int> WaitCounter::countWait(const Song& song, const Id& subsongId) noexcept
{
    OccurrenceMap<int> occurrenceMap;

    // Browses each Track. Note that the CellBrowser "browse" methods can not be used, because we need to know what the Track is over,
    // because we don't want to encode the end wait (useless).
    for (const auto trackIndex : TrackBrowser::getAllUsedTrackIndexes(song, subsongId, true)) {
        Track track;
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            track = subsong.getTrackRefFromIndex(trackIndex);
        });

        // Browses each Cell of the Track up to the last Cell, so that the last "space area" is skipped.
        const auto height = track.findMinimumHeight();
        auto accumulatedEmptyCells = 0;
        for (auto cellIndex = 0; cellIndex < height; ++cellIndex) {
            const auto& cell = track.getCellRefConst(cellIndex);
            if (!cell.isEmpty()) {
                // The Cell is not empty, so the accumulated empty cells are stored and reset.
                occurrenceMap.addItem(accumulatedEmptyCells);
                accumulatedEmptyCells = 0;
            } else {
                ++accumulatedEmptyCells;
            }
        }
    }

    return occurrenceMap;
}

}   // namespace arkostracker
