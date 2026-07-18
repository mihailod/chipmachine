#pragma once

#include "InsertOrRemoveCellAt.h"

namespace arkostracker 
{

class SongController;

/** Insert an empty cell or Special Cell. */
class InsertCellAt final : public InsertOrRemoveCellAt
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to access the Song.
     * @param location the location where to insert.
     * @param cursorLocation where the cursor is.
     */
    InsertCellAt(SongController& songController, const Location& location, const CursorLocation& cursorLocation) noexcept;

protected:
    Cell performActionOnTrack(Subsong& subsong, int trackIndex, int cellIndex) noexcept override;
    SpecialCell performActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex) noexcept override;
    void undoActionOnNormalTrack(Subsong& subsong, int trackIndex, int cellIndex, const Cell& cellToInsert) noexcept override;
    void undoActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCellToInsert) override;
};


}   // namespace arkostracker

