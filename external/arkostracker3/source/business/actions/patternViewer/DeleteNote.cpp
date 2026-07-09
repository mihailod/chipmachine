#include "DeleteNote.h"

#include "../../../utils/NumberUtil.h"

namespace arkostracker
{

DeleteNote::DeleteNote(SongController& pSongController, SelectedData pSelectedData, std::unordered_set<int> pInstrumentIndexes) noexcept :
        CellsModifier(pSongController, std::move(pSelectedData)),
        instrumentIndexesToDelete(std::move(pInstrumentIndexes))
{
}

std::unique_ptr<Cell> DeleteNote::performOnCell(const Cell& inputCell, const CaptureFlags& /*captureFlags*/) const noexcept
{
    // Both note and instrument must be present.
    if (!inputCell.isNoteAndInstrument()) {
        return nullptr;
    }

    // Is the instrument in the list?
    if (const auto instrumentIndex = inputCell.getInstrument();
        instrumentIndex.isPresent() && instrumentIndexesToDelete.find(instrumentIndex.getValue()) == instrumentIndexesToDelete.cend()) {
        return nullptr;
    }

    // Creates a new Cell.
    auto newCell = inputCell;
    newCell.setNote({ });
    newCell.setInstrument({ });

    return std::make_unique<Cell>(newCell);
}

std::unique_ptr<SpecialCell> DeleteNote::performOnSpecialCell(const bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return nullptr;         // Nothing to delete.
}

}   // namespace arkostracker
