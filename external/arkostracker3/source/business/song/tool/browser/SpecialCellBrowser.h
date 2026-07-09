#pragma once

#include "CellAndLocation.h"
#include "SpecialCellAndLocation.h"

#include <functional>
#include <unordered_set>

namespace arkostracker 
{

class SpecialCell;
class Song;
class Id;

class SpecialCellBrowser
{
public:
    /** Prevents instantiation. */
    SpecialCellBrowser() = delete;

    /**
     * Browses the SpecialCells of the used SpecialTracks of a Song, as they appear in the Linker.
     * A lock will be performed on each SpecialTrack, but not during the whole operation, as if could take too much time.
     * @param song the Song.
     * @param speedTrack true if SpeedTrack, false if EventTrack.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param onCellRead function called when a SpecialCell is read.
     */
    static void browseConstSongSpecialCells(const Song& song, bool speedTrack, bool stopAtLoopEnd, const std::function<void(const SpecialCell&)>& onCellRead) noexcept;

    /**
     * Browses the SpecialCells of the used SpecialTracks of one Subsong, as they appear in the Linker.
     * A lock will be performed on each SpecialTrack, but not during the whole operation, as if could take too much time.
     * @param song the Song.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param speedTrack true if SpeedTrack, false if EventTrack.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param onCellRead function called when a Cell is read.
     */
    static void browseConstSubsongSpecialCells(const Song& song, const Id& subsongId, bool speedTrack, bool stopAtLoopEnd,
                                        const std::function<void(const SpecialCell&)>& onCellRead) noexcept;


    /**
     * @return all the used SpecialTracks from a Subsong.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param speedTrack true if SpeedTrack, false if EventTrack.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     */
    static std::unordered_set<int> getAllUsedSpecialTrackIndexes(const Song& song, const Id& subsongId, bool speedTrack, bool stopAtLoopEnd) noexcept;

    /**
     * Browses the Cells of ALL the SpecialTracks of ALL the Subsongs, and allows to modify them.
     * A lock will be performed on each Track, but not during the whole operation, as if could take too much time.
     * @param song the Song. It will be modified.
     * @param speedTrack true if SpeedTrack, false if EventTrack.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     * @return the modified Cells (only).
     */
    static std::vector<SpecialCellAndLocation> browseSpecialCells(Song& song, bool speedTrack, bool stopAtLoopEnd,
                                                    const std::function<std::unique_ptr<SpecialCell>(const SpecialCell&)>& operationOnCell) noexcept;

    /**
     * Browses the Cells of ALL the SpecialTracks of the given Subsongs, and allows to modify them.
     * A lock will be performed on each Track, but not during the whole operation, as if could take too much time.
     * @param song the Song. It will be modified.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param speedTrack true if SpeedTrack, false if EventTrack.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     * @return the modified Cells (only).
     */
    static std::vector<SpecialCellAndLocation> browseSpecialCells(Song& song, const Id& subsongId,
                                                    bool speedTrack, bool stopAtLoopEnd,
                                                    const std::function<std::unique_ptr<SpecialCell>(const SpecialCell&)>& operationOnCell) noexcept;

    /**
     * Writes the given SpecialCells in the Song. A lock will be use during the operation, one Subsong at a time.
     * @param song the Song. It will be modified.
     * @param cellsAndLocations the cells to write.
     */
    static void writeSpecialCells(Song& song, const std::vector<SpecialCellAndLocation>& cellsAndLocations) noexcept;
};

}   // namespace arkostracker
