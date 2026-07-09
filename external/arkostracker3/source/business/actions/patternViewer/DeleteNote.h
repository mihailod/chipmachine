#pragma once

#include "CellsModifier.h"

namespace arkostracker
{

/** Transpose only notes. */
class DeleteNote final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param selectedData info about the data to process.
     * @param instrumentIndexes if empty, nothing is done. RST can be deleted.
     */
    DeleteNote(SongController& songController, SelectedData selectedData, std::unordered_set<int> instrumentIndexes = { }) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;

private:
    std::unordered_set<int> instrumentIndexesToDelete;           // Or empty.
};

} // namespace arkostracker
