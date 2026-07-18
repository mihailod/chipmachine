#include "TransposeNote.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker
{

TransposeNote::TransposeNote(SongController& pSongController, const int pTransposeRate, SelectedData pSelectedData, std::unordered_set<int> pInstrumentIndexes) noexcept :
        CellsModifier(pSongController, std::move(pSelectedData)),
        transposeRate(pTransposeRate),
        instrumentIndexesToTranspose(std::move(pInstrumentIndexes))
{
}

std::unique_ptr<Cell> TransposeNote::performOnCell(const Cell& inputCell, const CaptureFlags& /*captureFlags*/) const noexcept
{
    const auto note = inputCell.getNote();
    if (!note.isPresent() || inputCell.isRst()) {
        return nullptr;
    }

    // Is the instrument in the instrument-to-transpose-list (if any)?
    if (!instrumentIndexesToTranspose.empty()) {
        if (const auto instrumentIndex = inputCell.getInstrument();
            instrumentIndex.isPresent() && instrumentIndexesToTranspose.find(instrumentIndex.getValue()) == instrumentIndexesToTranspose.cend()) {
            return nullptr;
        }
    }

    const auto baseNote = note.getValue().getNote();
    const auto newNote = NumberUtil::correctNumber(baseNote + transposeRate, CellConstants::minimumNote, CellConstants::maximumNote);
    if (baseNote == newNote) {
        return nullptr;         // Happens when the limit is already reached.
    }

    // Creates a new Cell.
    auto newCell = inputCell;
    newCell.setNote(Note::buildNote(newNote));

    return std::make_unique<Cell>(newCell);
}

std::unique_ptr<SpecialCell> TransposeNote::performOnSpecialCell(const bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return nullptr;         // Nothing to transpose.
}

}   // namespace arkostracker
