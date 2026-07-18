#pragma once

#include "../../../../song/cells/Cell.h"
#include "../../../../utils/Id.h"
#include "CellAndLocation.h"
#include "../../../../song/CellLocationInPosition.h"

namespace arkostracker 
{

class Song;

/**
 * Browses the cells of a Song or Subsongs, and allow to modify them.
 * To only browse and thus using a const Song, use the static method browseConstSongCells/browseConstSubsongCells.
 * Also nice are various the Get Tracks method.
 */
class CellBrowser
{
public:
    /**
     * Constructor.
     * @param song the Song to browse.
     */
    explicit CellBrowser(Song& song) noexcept;

    /**
     * Browses the Cells of ALL the Tracks of ALL the Subsongs, and allows to modify them.
     * A lock will be performed on each Track, but not during the whole operation, as it could take too much time.
     * @param returnModifiedCells if true, the modified Cells are returned.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     * @return the modified Cells, or empty if returnModifiedCells is false.
     */
    std::vector<CellAndLocation> browseCells(bool returnModifiedCells, const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept;

    /**
     * Browses the Cells of ALL the Tracks of one Subsong, and allows to modify them.
     * A lock will be performed on each Track, but not during the whole operation, as it could take too much time.
     * @param returnModifiedCells if true, the modified Cells are returned.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     * @return the modified Cells, or empty if returnModifiedCells is false.
     */
    std::vector<CellAndLocation> browseCells(bool returnModifiedCells, const Id& subsongId, const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept;

    /**
     * Browses the Cells of the used Tracks of one Subsong, as they appear in the Linker, and allows to modify them.
     * A lock will be performed on each Track, but not during the whole operation, as it could take too much time.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     */
    void browseSubsongCells(bool stopAtLoopEnd, const Id& subsongId, const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept;

    /**
     * Browses the Cells of a Tracks of one Subsong, and allows to modify them.
     * A lock will be used during the operation.
     * @param returnModifiedCells if true, the modified Cells are returned.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param trackIndex the index of the Track. Must be valid.
     * @param operationOnCell the operation to apply on the Cell. If returns nullptr, the Cell is not modified.
     * @return the modified Cells, or empty if returnModifiedCells is false.
     */
    std::vector<CellAndLocation> browseCells(bool returnModifiedCells, const Id& subsongId, int trackIndex,
                                      const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept;

    /**
     * Writes the Cells in the Song. A lock will be used during the operation, one Subsong at a time.
     * @param cellsAndLocations the cells to write.
     */
    void writeCells(const std::vector<CellAndLocation>& cellsAndLocations) const noexcept;

    /**
     * Browses the Cells of the used Tracks of a Song, as they appear in the Linker.
     * A lock will be performed on each Track, but not during the whole operation, as it could take too much time.
     * @param song the Song.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param onCellRead function called when a Cell is read. Return true to stop, false to continue.
     */
    static void browseConstSongCells(const Song& song, bool stopAtLoopEnd, const std::function<bool(const Cell&)>& onCellRead) noexcept;

    /**
     * Browses the Cells of the used Tracks of one Subsong, as they appear in the Linker.
     * A lock will be performed on each Track, but not during the whole operation, as it could take too much time.
     * @param song the Song.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param onCellRead function called when a Cell is read. Return true to stop, false to continue.
     * @return true if has been stopped.
     */
    static bool browseConstSubsongCells(const Song& song, const Id& subsongId, bool stopAtLoopEnd, const std::function<bool(const Cell&)>& onCellRead) noexcept;

    /**
     * Browses the cells from all the positions of one Subsong. Only the cells from the position heights are browsed. Tracks may be browsed more than once.
     * A lock will be performed on each Position, but not during the whole operation, as it could take too much time.
     * @param song the Song.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     * @param onCellRead function called when a Cell is read, along where is was found. Return true to store this Cell, false to ignore it.
     * @return the stored Cells.
     */
    static std::vector<CellLocationInPosition> browseCellsFromPositions(const Song& song, const Id& subsongId, bool stopAtLoopEnd,
        const std::function<bool(const CellLocationInPosition& cellLocationInPosition, const Cell&)>& onCellRead) noexcept;

private:
    Song& song;
};

}   // namespace arkostracker
