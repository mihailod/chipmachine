#include "Transpose.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../song/cells/SpecialCellConstants.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

Transpose::Transpose(SongController& pSongController, const TransposeRate pTransposeRate, SelectedData pSelectedData) noexcept :
        CellsModifier(pSongController, std::move(pSelectedData)),
        transposeRate(pTransposeRate),
        mode(Mode::note)        // Set below.
{
    // What can be transposed? It depends on what has been selected.
    const auto selectedColumns = getSelectedData().getSelectedColumns();
    const auto hasSeveralCaptures = selectedColumns.hasSeveralCaptures();
    if (selectedColumns.isNoteCaptured()) {         // Note has priority.
        mode = Mode::note;
    } else if (!hasSeveralCaptures && selectedColumns.isInstrumentCaptured()) {
        mode = Mode::instrument;
    } else if (!hasSeveralCaptures && selectedColumns.isEffectCaptured()) {
        mode = Mode::effect;
    } else if (!hasSeveralCaptures && selectedColumns.isSpeedTrackCaptured()) {
        mode = Mode::speedTrack;
    } else if (!hasSeveralCaptures && selectedColumns.isEventTrackCaptured()) {
        mode = Mode::eventTrack;
    }
}

std::unique_ptr<Cell> Transpose::performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept
{
    if (mode == Mode::note) {
        return performOnNotes(inputCell, captureFlags);
    }
    if (mode == Mode::instrument) {
        return performOnInstruments(inputCell);
    }
    if (mode == Mode::effect) {
        return performOnEffects(inputCell, captureFlags);
    }
    if ((mode == Mode::speedTrack) || (mode == Mode::eventTrack)) {
        jassertfalse;       // Shouldn't be called in this method.
        return nullptr;
    }

    jassertfalse;
    return nullptr;
}

std::unique_ptr<SpecialCell> Transpose::performOnSpecialCell(const bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept
{
    // Only one Special Track can be used at the same time.
    if ((isSpeedTrack && (mode != Mode::speedTrack))
        || (!isSpeedTrack && (mode != Mode::eventTrack))) {
        return nullptr;
    }

    // Don't transpose empty values.
    if (inputSpecialCell.isEmpty()) {
        return nullptr;
    }

    // How much to change?
    const auto transposeRateValue = getTransposeAmount(transposeRate);

    const auto baseValue = inputSpecialCell.getValue();
    const auto newValue = NumberUtil::correctNumber(baseValue + transposeRateValue, SpecialCellConstants::minimumValueExceptEmpty,
        SpecialCellConstants::maximumValue);
    if (baseValue == newValue) {
        return nullptr;         // Happens when the limit is already reached.
    }

    // Creates a new Cell.
    return std::make_unique<SpecialCell>(SpecialCell::buildSpecialCell(newValue));
}

std::unique_ptr<Cell> Transpose::performOnNotes(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept
{
    // Any note? If RST, don't transpose. The note must be captured.
    if (!captureFlags.isNoteCaptured() || !inputCell.isNote() || inputCell.isRst()) {
        return nullptr;
    }

    // How much to transpose?
    const auto transposeRateSemiTones = getTransposeAmount(transposeRate, CellConstants::noteCountInOctave);

    // Transposes.
    jassert(inputCell.getNote().isPresent());
    const auto baseNote = inputCell.getNote().getValue().getNote();
    const auto newNote = NumberUtil::correctNumber(baseNote + transposeRateSemiTones, CellConstants::minimumNote, CellConstants::maximumNote);
    if (baseNote == newNote) {
        return nullptr;         // Happens when the limit is already reached.
    }

    // Creates a new Cell.
    auto newCell = inputCell;
    newCell.setNote(Note::buildNote(newNote));

    return std::make_unique<Cell>(newCell);
}

std::unique_ptr<Cell> Transpose::performOnInstruments(const Cell& inputCell) const noexcept
{
    // A note must be present for the instrument to be relevant.
    // RST cannot be transposed.
    if (!inputCell.isNoteAndInstrument() || inputCell.isRst()) {
        return nullptr;
    }

    // How much to change?
    const auto transposeRateValue = getTransposeAmount(transposeRate);

    const auto baseInstrument = inputCell.getInstrument().getValue();
    // Cannot switch to RST.
    const auto newInstrument = NumberUtil::correctNumber(baseInstrument + transposeRateValue, CellConstants::minimumInstrumentExceptRst,
        CellConstants::maximumInstrument);
    if (baseInstrument == newInstrument) {
        return nullptr;         // Happens when the limit is already reached.
    }

    // Creates a new Cell.
    return std::make_unique<Cell>(inputCell.withInstrument(newInstrument));
}

std::unique_ptr<Cell> Transpose::performOnEffects(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept
{
    const auto transposeRateValue = getTransposeAmount(transposeRate, 4);
    auto hasChanged = false;

    auto newEffects = inputCell.getEffects();

    managePerformOnEffect(captureFlags.isEffect1Captured(), newEffects, 0, hasChanged, transposeRateValue);
    managePerformOnEffect(captureFlags.isEffect2Captured(), newEffects, 1, hasChanged, transposeRateValue);
    managePerformOnEffect(captureFlags.isEffect3Captured(), newEffects, 2, hasChanged, transposeRateValue);
    managePerformOnEffect(captureFlags.isEffect4Captured(), newEffects, 3, hasChanged, transposeRateValue);

    if (hasChanged) {
        auto newCell = std::make_unique<Cell>(inputCell);
        newCell->setEffects(newEffects);
        return newCell;
    }
    return nullptr;
}

int Transpose::getTransposeAmount(const TransposeRate transposeRate, const int fastValue) noexcept
{
    switch (transposeRate) {
        case TransposeRate::upNormal:
            return 1;
        case TransposeRate::upFast:
            return fastValue;
        case TransposeRate::downNormal:
            return -1;
        case TransposeRate::downFast:
            return -fastValue;
        default:
            jassertfalse;
            return 0;
    }
}

void Transpose::managePerformOnEffect(const bool isEffectCaptured, CellEffects& cellEffects, const int effectIndex, bool& hasChangedOut, const int transposeRateValue) noexcept
{
    if (isEffectCaptured && cellEffects.hasEffect(effectIndex)) {
        const auto cellEffect = cellEffects.getEffect(effectIndex);
        const auto effect = cellEffect.getEffect();
        const auto originalEffectLogicalValue = cellEffect.getEffectLogicalValue();
        // Checks the limits.
        const auto [minimumValue, maximumValue] = EffectUtil::getMinMaxValues(effect);
        const auto newEffectLogicalValue = NumberUtil::correctNumber(originalEffectLogicalValue + transposeRateValue, minimumValue, maximumValue);
        const auto changed = (originalEffectLogicalValue != newEffectLogicalValue);

        if (changed) {
            const auto newCellEffect = CellEffect::buildFromLogicalValue(effect, newEffectLogicalValue);
            cellEffects.setEffect(effectIndex, newCellEffect);

            hasChangedOut = true;
        }
    }
}

}   // namespace arkostracker
