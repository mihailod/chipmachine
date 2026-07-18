#include "InsertCellAt.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker 
{

InsertCellAt::InsertCellAt(SongController& pSongController, const Location& pLocation, const CursorLocation& pCursorLocation) noexcept :
        InsertOrRemoveCellAt(pSongController, pLocation, pCursorLocation)
{
}

Cell InsertCellAt::performActionOnTrack(Subsong& subsong, int trackIndex, int cellIndex) noexcept
{
    return subsong.insertCellAt(trackIndex, cellIndex, Cell());
}

SpecialCell InsertCellAt::performActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex) noexcept
{
    return subsong.insertSpecialCellAt(isSpeedTrack, specialTrackIndex, cellIndex, SpecialCell());
}

void InsertCellAt::undoActionOnNormalTrack(Subsong& subsong, int trackIndex, int cellIndex, const Cell& cellToInsert) noexcept
{
    subsong.removeCellAt(trackIndex, cellIndex, cellToInsert);
}

void InsertCellAt::undoActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCellToInsert)
{
    subsong.removeSpecialCellAt(isSpeedTrack, specialTrackIndex, cellIndex, specialCellToInsert);
}


}   // namespace arkostracker

