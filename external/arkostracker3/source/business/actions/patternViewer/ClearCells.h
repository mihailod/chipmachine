#pragma once

#include "CellsModifier.h"

namespace arkostracker 
{

/** Clears the given Cells. */
class ClearCells final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param selectedData the data to modify.
     */
    ClearCells(SongController& songController, const SelectedData& selectedData) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;
};

}   // namespace arkostracker
