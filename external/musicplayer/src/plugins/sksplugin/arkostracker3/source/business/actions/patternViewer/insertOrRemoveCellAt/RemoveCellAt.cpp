#include "RemoveCellAt.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker 
{

RemoveCellAt::RemoveCellAt(SongController& pSongController, const Location& pLocation, const CursorLocation& pCursorLocation) noexcept :
        InsertOrRemoveCellAt(pSongController, pLocation, pCursorLocation)
{
}

Cell RemoveCellAt::performActionOnTrack(Subsong& subsong, int trackIndex, int cellIndex) noexcept
{
    return subsong.removeCellAt(trackIndex, cellIndex, Cell());
}

SpecialCell RemoveCellAt::performActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex) noexcept
{
    return subsong.removeSpecialCellAt(isSpeedTrack, specialTrackIndex, cellIndex, SpecialCell());
}

void RemoveCellAt::undoActionOnNormalTrack(Subsong& subsong, int trackIndex, int cellIndex, const Cell& cellToInsert) noexcept
{
    subsong.insertCellAt(trackIndex, cellIndex, cellToInsert);
}

void RemoveCellAt::undoActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCellToInsert)
{
    subsong.insertSpecialCellAt(isSpeedTrack, specialTrackIndex, cellIndex, specialCellToInsert);
}


}   // namespace arkostracker

