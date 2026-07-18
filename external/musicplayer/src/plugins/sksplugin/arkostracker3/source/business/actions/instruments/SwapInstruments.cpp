#include "SwapInstruments.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker
{

SwapInstruments::SwapInstruments(SongController& pSongController, SelectedData pSelectedData, const int pFirstInstrumentIndex, const int pSecondInstrumentIndex) noexcept :
        CellsModifier(pSongController, std::move(pSelectedData)),
        firstInstrumentIndex(pFirstInstrumentIndex),
        secondInstrumentIndex(pSecondInstrumentIndex)
{
}

std::unique_ptr<Cell> SwapInstruments::performOnCell(const Cell& inputCell, const CaptureFlags& /*captureFlags*/) const noexcept
{
    if (firstInstrumentIndex == secondInstrumentIndex) {
        jassertfalse;       // Same indexes??
        return nullptr;
    }
    if ((firstInstrumentIndex == CellConstants::rstInstrument) || (secondInstrumentIndex == CellConstants::rstInstrument)) {
        jassertfalse;       // RST not authorized.
        return nullptr;
    }

    // The instrument must be present, and match the source or target instrument.
    if (!inputCell.isNoteAndInstrument()) {
        return nullptr;
    }

    const auto readInstrumentIndex = inputCell.getInstrument().getValue();
    int newTargetInstrument;        // NOLINT(*-init-variables)
    if (readInstrumentIndex == firstInstrumentIndex) {
        newTargetInstrument = secondInstrumentIndex;
    } else if (readInstrumentIndex == secondInstrumentIndex) {
        newTargetInstrument = firstInstrumentIndex;
    } else {
        return nullptr;
    }

    // Creates a new Cell.
    auto newCell = inputCell;
    newCell.setInstrument(newTargetInstrument);

    return std::make_unique<Cell>(newCell);
}

std::unique_ptr<SpecialCell> SwapInstruments::performOnSpecialCell(const bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return nullptr;
}

}   // namespace arkostracker
