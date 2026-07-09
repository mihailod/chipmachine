#include "ClearCells.h"

namespace arkostracker 
{

ClearCells::ClearCells(SongController& pSongController, const SelectedData& pSelectedData) noexcept :
        CellsModifier(pSongController, pSelectedData)
{
}

std::unique_ptr<Cell> ClearCells::performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept
{
    auto newCell = std::make_unique<Cell>(inputCell);

    // Removes everything that IS captured.
    if (captureFlags.isNoteCaptured()) {
        newCell->clearNote();
    }
    if (captureFlags.isInstrumentCaptured()) {
        newCell->clearInstrument();
    }

    if (captureFlags.isEffect1Captured()) {
        newCell->removeEffectFromIndex(0);
    }
    if (captureFlags.isEffect2Captured()) {
        newCell->removeEffectFromIndex(1);
    }
    if (captureFlags.isEffect3Captured()) {
        newCell->removeEffectFromIndex(2);
    }
    if (captureFlags.isEffect4Captured()) {
        newCell->removeEffectFromIndex(3);
    }

    return newCell;
}

std::unique_ptr<SpecialCell> ClearCells::performOnSpecialCell(bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return std::make_unique<SpecialCell>();           // Returns an empty Special Cell whatever happens.
}

}   // namespace arkostracker
