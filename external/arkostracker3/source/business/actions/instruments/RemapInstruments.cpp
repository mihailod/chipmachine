#include "RemapInstruments.h"

#include "../../../utils/NumberUtil.h"

namespace arkostracker
{

RemapInstruments::RemapInstruments(SongController& pSongController, SelectedData pSelectedData, std::unordered_set<int> pSourceInstrumentIndexes,
    const int pTargetInstrumentIndex) noexcept :
        CellsModifier(pSongController, std::move(pSelectedData)),
        sourceInstrumentIndexes(std::move(pSourceInstrumentIndexes)),
        targetInstrumentIndex(pTargetInstrumentIndex)
{
}

std::unique_ptr<Cell> RemapInstruments::performOnCell(const Cell& inputCell, const CaptureFlags& /*captureFlags*/) const noexcept
{
    // The instrument must be present.
    if (!inputCell.isNoteAndInstrument()) {
        return nullptr;
    }

    // The instrument matches any source instrument?
    const auto readInstrumentIndex = inputCell.getInstrument().getValue();
    if (std::find(sourceInstrumentIndexes.cbegin(), sourceInstrumentIndexes.cend(), readInstrumentIndex) == sourceInstrumentIndexes.cend()) {
        return nullptr;
    }

    // Creates a new Cell.
    auto newCell = inputCell;
    newCell.setInstrument(targetInstrumentIndex);

    return std::make_unique<Cell>(newCell);
}

std::unique_ptr<SpecialCell> RemapInstruments::performOnSpecialCell(const bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return nullptr;
}

}   // namespace arkostracker
