#include "Cell.h"

#include "../../utils/NumberUtil.h"
#include "CellConstants.h"

namespace arkostracker 
{

Cell::Cell() noexcept :
        note(),
        instrument(),
        effects()
{
}

Cell::Cell(const OptionalValue<Note>& pNote, const OptionalInt& pInstrument) noexcept :
        note(pNote),
        instrument(pInstrument),
        effects()
{
}

Cell::Cell(const OptionalValue<Note>& pNote, const OptionalInt& pInstrument, CellEffects pEffects) noexcept :
        note(pNote),
        instrument(pInstrument),
        effects(std::move(pEffects))
{
}

Cell::Cell(CellEffects pEffects) noexcept :
        note(),
        instrument(),
        effects(std::move(pEffects))
{
}

Cell Cell::build(const int note, int instrument) noexcept
{
    return { Note::buildNote(note), instrument };
}

Cell Cell::build(const int note, int instrument, const CellEffects& effects) noexcept
{
    return { Note::buildNote(note), instrument, effects };
}

Cell Cell::buildRst() noexcept
{
    return { Note::buildRst(), CellConstants::rstInstrument };
}

const OptionalValue<Note>& Cell::getNote() const noexcept
{
    return note;
}

void Cell::setNote(const OptionalValue<Note> newNote) noexcept
{
    note = newNote;
}

const OptionalInt& Cell::getInstrument() const noexcept
{
    return instrument;
}

void Cell::setInstrument(const OptionalInt newInstrumentIndex) noexcept
{
    instrument = newInstrumentIndex;
}

const CellEffects& Cell::getEffects() const noexcept
{
    return effects;
}

void Cell::setEffects(const CellEffects& newCellEffects) noexcept
{
    effects = newCellEffects;
}

void Cell::setEffectDigit(const int effectIndex, const int digitIndex, const int value) noexcept
{
    effects.setEffectDigit(effectIndex, digitIndex, value);
}

void Cell::setEffectNumber(const int effectIndex, const Effect effect) noexcept
{
    effects.setEffectNumber(effectIndex, effect);
}

bool Cell::isEffectPresent(const int effectIndex) const noexcept
{
    return effects.getEffect(effectIndex).isPresent();
}

Cell Cell::mix(const Cell& cellWithPriority) const noexcept
{
    auto newCell = *this;

    if (cellWithPriority.isNote()) {
        newCell.setNote(cellWithPriority.note);
    }
    if (cellWithPriority.isNoteAndInstrument()) {       // Instruments are not shown if there is no note.
        newCell.setInstrument(cellWithPriority.instrument);
    }
    for (const auto&[effectIndex, cellEffect] : cellWithPriority.getEffects().getExistingEffects()) {
        newCell.setEffect(effectIndex, cellEffect);
    }

    return newCell;
}

bool Cell::isLegato() const noexcept
{
    return (note.isPresent() && instrument.isAbsent() && effects.find(Effect::pitchGlide).isAbsent());
}

bool Cell::isEmpty() const noexcept
{
    return note.isAbsent() && instrument.isAbsent() && effects.isEmpty();
}

bool Cell::isClean() const noexcept
{
    return isEmpty() && effects.isClean();
}

bool Cell::isNote() const noexcept
{
    return note.isPresent();
}

bool Cell::isInstrument() const noexcept
{
    return instrument.isPresent();
}

bool Cell::isNoteAndInstrument() const noexcept
{
    return isNote() && isInstrument();
}

bool Cell::isRst() const noexcept
{
    return isNote() && isInstrument() && (instrument.getValueRef() == CellConstants::rstInstrument);
}

int Cell::correctNote(const int note) noexcept
{
    return NumberUtil::restrictWithLoop(note, CellConstants::maximumNote);
}

bool Cell::hasEffects() const noexcept
{
    return !effects.isEmpty();
}

void Cell::clearNote() noexcept
{
    note = OptionalValue<Note>();
}

void Cell::clearInstrument() noexcept
{
    instrument = OptionalInt();
}

void Cell::clear() noexcept
{
    clearNote();
    clearInstrument();
    effects.clear();
}

bool Cell::addEffect(const Effect effect, const int logicalValue) noexcept
{
    return effects.addEffect(effect, logicalValue);
}

bool Cell::setEffect(const int effectIndex, const CellEffect& cellEffect) noexcept
{
    return effects.setEffect(effectIndex, cellEffect);
}

OptionalValue<CellEffect> Cell::find(const Effect effect) const noexcept
{
    return effects.find(effect);
}

std::pair<int, OptionalValue<CellEffect>> Cell::findWithIndex(const Effect effect) const noexcept
{
    return effects.findWithIndex(effect);
}

void Cell::performOnEffects(const std::function<void(std::pair<int, CellEffect&>)>& operationToApply) noexcept
{
    effects.performOnEffects(operationToApply);
}

void Cell::removeEffectFromIndex(const int effectIndex) noexcept
{
    effects.clearEffect(effectIndex);
}

bool Cell::removeEffect(const Effect effect) noexcept
{
    return effects.removeEffect(effect);
}

void Cell::setEffectValue(const int effectIndex, const int rawValue) noexcept
{
    effects.setEffectValue(effectIndex, rawValue);
}

void Cell::shiftEffects(const int shift) noexcept
{
    effects.shiftEffects(shift);
}

bool Cell::operator==(const Cell& rhs) const noexcept
{
    return note == rhs.note &&
           instrument == rhs.instrument &&
           effects == rhs.effects;
}

Cell Cell::with(const Cell& additionalCell, const bool overwriteNote, const bool overwriteInstrument, const bool overwriteEffect1, const bool overwriteEffect2,
    const bool overwriteEffect3, const bool overwriteEffect4) const noexcept
{
    // Nothing to be overwritten? Then returns the same cell.
    if (!overwriteNote && !overwriteInstrument && !overwriteEffect1 && !overwriteEffect2 && !overwriteEffect3 && !overwriteEffect4) {
        jassertfalse;       // Strange, nothing to modify? Can actually happen if copying note+effect (but no effect), and pasting on effect.
        return *this;
    }

    auto newCell = *this;

    // Writes each part that needs to be.
    if (overwriteNote) {
        newCell.setNote(additionalCell.note);
    }
    if (overwriteInstrument) {
        newCell.setInstrument(additionalCell.instrument);
    }
    const auto& lEffects = additionalCell.getEffects();
    if (overwriteEffect1) {
        newCell.setEffect(0, lEffects.getEffect(0));
    }
    if (overwriteEffect2) {
        newCell.setEffect(1, lEffects.getEffect(1));
    }
    if (overwriteEffect3) {
        newCell.setEffect(2, lEffects.getEffect(2));
    }
    if (overwriteEffect4) {
        newCell.setEffect(3, lEffects.getEffect(3));
    }

    return newCell;
}

Cell Cell::withInstrument(const int instrumentIndex) const noexcept
{
    auto newCell = *this;
    newCell.setInstrument(instrumentIndex);
    return newCell;
}

bool Cell::operator!=(const Cell& rhs) const noexcept
{
    return !(rhs == *this);
}

bool Cell::areEffectsPresentAmong(const std::set<Effect>& pEffects) const noexcept
{
    return effects.areEffectsPresentAmong(pEffects);
}


}   // namespace arkostracker

