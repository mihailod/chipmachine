#include "CellOperations.h"

#include "../../../song/cells/CellConstants.h"
#include "../cells/CellEffectsChecker.h"

namespace arkostracker 
{

std::unique_ptr<Cell> LegatoToEffect::operator()(const Cell& cell) const noexcept
{
    if (cell.isNote() && !cell.isInstrument() && cell.find(Effect::pitchGlide).isAbsent()) {
        // There is a note, but no Instrument. Legato!
        const auto note = cell.getNote().getValueRef();

        // Removes the note.
        auto newCell = std::make_unique<Cell>(cell);
        newCell->clearNote();

        // Adds the Legato effect.
        const auto success = newCell->addEffect(Effect::legato, note.getNote());
        jassert(success);       // No more room for Legato!
        (void)success;

        return newCell;
    }

    return nullptr;
}


// ========================================================================

std::unique_ptr<Cell> NormalizeCell::operator()(const Cell& cell) const noexcept
{
    auto newCell = std::make_unique<Cell>(cell);

    // If RST, the note must be an "RST note" by convention.
    if (newCell->isRst()) {
        if (newCell->getNote().getValue().getNote() != CellConstants::rstNote) {
            newCell->setNote(Note::buildRst());
        }
    }

    // Instrument without Note? If yes, removes the Instrument.
    if (newCell->isInstrument() && !newCell->isNote()) {
        newCell->setInstrument({});
    }

    // Normalizes the Effects.
    const auto normalizedCellEffects = CellEffectsChecker::normalize(*newCell);
    newCell->setEffects(normalizedCellEffects);

    const auto changed = (cell != *newCell);
    return changed ? std::move(newCell) : nullptr;
}


// ========================================================================

IllegalInstrumentToRstInCell::IllegalInstrumentToRstInCell(const int pInstrumentCount) noexcept :
        instrumentCount(pInstrumentCount)
{
    jassert(instrumentCount > 0);
}

std::unique_ptr<Cell> IllegalInstrumentToRstInCell::operator()(const Cell& cell) const noexcept
{
    if (cell.isNoteAndInstrument() && (cell.getInstrument().getValue() >= instrumentCount)) {
        // The instrument is out of bounds. Encodes an RST.
        auto newCell = std::make_unique<Cell>(cell);
        newCell->setNote(Note::buildRst());
        newCell->setInstrument(CellConstants::rstInstrument);

        return newCell;
    }

    return nullptr;
}


// ========================================================================


IllegalExpressionTo0InCell::IllegalExpressionTo0InCell(const bool pIsArpeggio, const int pExpressionCount) noexcept :
        isArpeggio(pIsArpeggio),
        expressionCount(pExpressionCount)
{
    jassert(pExpressionCount > 0);
}

std::unique_ptr<Cell> IllegalExpressionTo0InCell::operator()(const Cell& cell) const noexcept
{
    if (!cell.hasEffects()) {
        return nullptr;
    }

    const auto targetEffect = isArpeggio ? Effect::arpeggioTable : Effect::pitchTable;

    auto newCell = std::make_unique<Cell>(cell);

    auto changed = false;
    newCell->performOnEffects([&](const std::pair<int, CellEffect&>& indexAndCellEffect) {
        const auto& cellEffect = indexAndCellEffect.second;
        if ((cellEffect.getEffect() == targetEffect) && (cellEffect.getEffectLogicalValue() >= expressionCount)) {
            // Illegal value. Forces it to 0.
            newCell->setEffectValue(indexAndCellEffect.first, 0);
            changed = true;
        }
    });

    return changed ? std::move(newCell) : nullptr;
}

}   // namespace arkostracker
