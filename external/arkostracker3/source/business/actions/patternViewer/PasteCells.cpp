#include "PasteCells.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

PasteCells::PasteCells(SongController& pSongController, Location pLocation, const CursorLocation& pCursorLocation, PasteData pPasteData,
    const bool pPasteMix, const bool pPasteContinuously) noexcept :
        songController(pSongController),
        location(std::move(pLocation)),
        pasteCursorLocation(pCursorLocation),
        pasteData(std::move(pPasteData)),
        pasteMix(pPasteMix),
        pasteContinuously(pPasteContinuously),
        oldCellsAndLocations(),
        oldSpecialCellsAndLocations()
{
}

bool PasteCells::perform()
{
    jassert(oldCellsAndLocations.empty());
    jassert(oldSpecialCellsAndLocations.empty());
    // Only a security, shouldn't happen.
    oldCellsAndLocations.clear();
    oldSpecialCellsAndLocations.clear();

    auto modified = false;

    const auto& captureFlags = pasteData.captureFlags;
    const auto& trackToCells = pasteData.tracksToCells;
    jassert(captureFlags.size() == trackToCells.size());        // MUST be equal!

    const auto subsongId = location.getSubsongId();
    const auto position = location.getPosition();
    const auto pasteLineIndex = location.getLine();

    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        const auto channelCount = subsong.getChannelCount();

        const auto positionHeight = subsong.getPositionHeight(position);

        // Pastes the possible Track first.
        if (pasteCursorLocation.getTrackType() == TrackType::normal) {
            const auto pasteChannelIndex = pasteCursorLocation.getChannelIndex();
            size_t channelIndexAt0 = 0;

            // Browses the channel where to paste.
            for (auto targetChannelIndex = pasteChannelIndex; (targetChannelIndex < channelCount) && (targetChannelIndex < pasteChannelIndex + static_cast<int>(trackToCells.size()));
                ++targetChannelIndex, ++channelIndexAt0) {

                const auto& cells = trackToCells.at(channelIndexAt0);
                const auto cellCount = cells.size();
                const auto& baseCaptureFlags = captureFlags.at(channelIndexAt0);
                // The first pasted channel can have the cursor not at the beginning of the channel, which may shift the effects or dismiss the note/instrument.
                const auto cursorRank = (channelIndexAt0 == 0) ? pasteCursorLocation.getRankAsCellCursorRank() : CellCursorRank::first;

                // Browses the cells.
                auto targetCellIndex = pasteLineIndex;
                auto mustContinue = true;

                // The paste of the clipboard is played once, or several times if paste continuously.
                while (mustContinue) {
                    const auto pastEndCellIndex = targetCellIndex + pasteData.height;

                    for (size_t cellIndexAt0 = 0;
                         ((targetCellIndex < pastEndCellIndex) && (targetCellIndex < positionHeight) && (cellIndexAt0 < cellCount)); ++targetCellIndex, ++cellIndexAt0) {

                        const auto& originalCopiedCell = cells.at(cellIndexAt0);
                        const auto& originalCellToBeOverwritten = subsong.getCell(position, targetCellIndex, targetChannelIndex);
                        auto cellToBeOverwritten = originalCellToBeOverwritten;
                        auto innerCaptureFlags = baseCaptureFlags;
                        // Shift needed on the cell to paste? The base copied cell is modified.
                        const auto[newCell, newCaptureFlags, newCopiedCell] = manageShiftedCursorOverCell(cellToBeOverwritten, originalCopiedCell, cursorRank, innerCaptureFlags);
                        cellToBeOverwritten = newCell;
                        innerCaptureFlags = newCaptureFlags;

                        // Applies the CaptureFlags on the cell to paste.
                        auto capturedCell = cellToBeOverwritten.with(newCopiedCell, innerCaptureFlags.isNoteCaptured(), innerCaptureFlags.isInstrumentCaptured(),
                                                                     innerCaptureFlags.isEffect1Captured(), innerCaptureFlags.isEffect2Captured(),
                                                                     innerCaptureFlags.isEffect3Captured(), innerCaptureFlags.isEffect4Captured());
                        if (pasteMix) {
                            capturedCell = originalCellToBeOverwritten.mix(capturedCell);
                        }

                        subsong.setCell(position, targetCellIndex, targetChannelIndex, capturedCell);

                        modified = true;

                        // Stores it for undo.
                        const CellLocationInPosition cellLocationInPosition(subsongId, position, targetCellIndex, targetChannelIndex);
                        oldCellsAndLocations.emplace_back(cellLocationInPosition, cellToBeOverwritten);
                    }

                    if (!pasteContinuously) {
                        mustContinue = false;
                    } else {
                        mustContinue = (targetCellIndex < positionHeight);
                    }
                }
            }
        }

        // Pastes the possible Special Tracks.
        modified |= pasteSpecialTrackIfNeeded(true, subsong, pasteData.speedCells, subsongId, position, pasteLineIndex, positionHeight);
        modified |= pasteSpecialTrackIfNeeded(false, subsong, pasteData.eventCells, subsongId, position, pasteLineIndex, positionHeight);
    });

    notifyListeners();

    return modified;
}

bool PasteCells::pasteSpecialTrackIfNeeded(const bool isSpeedTrack, Subsong& subsong, const std::vector<SpecialCell>& specialCells, const Id& subsongId, const int position,
                                           const int pasteLineIndex, const int positionHeight) noexcept
{
    if (specialCells.empty()) {
        return false;
    }

    auto modified = false;

    const auto specialCellCount = specialCells.size();
    auto targetCellIndex = pasteLineIndex;
    auto mustContinue = true;

    // The paste of the clipboard is played once, or several times if paste continuously.
    while (mustContinue) {
        const auto pastEndCellIndex = targetCellIndex + pasteData.height;

        for (size_t specialCellIndexAt0 = 0;
            ((targetCellIndex < pastEndCellIndex) && (targetCellIndex < positionHeight) && (specialCellIndexAt0 < specialCellCount));
            ++targetCellIndex, ++specialCellIndexAt0) {
            // Stores the cell to be overwritten, for undo.
            const auto specialCellToBeOverwritten = subsong.getSpecialCell(isSpeedTrack, position, targetCellIndex);
            const SpecialCellLocationInPosition specialCellLocation(isSpeedTrack, subsongId, position, targetCellIndex);
            oldSpecialCellsAndLocations.emplace_back(specialCellLocation, specialCellToBeOverwritten);

            // Gets the Special Cell.
            auto sourceSpecialCell = specialCells.at(specialCellIndexAt0);
            if (pasteMix) {
                sourceSpecialCell = specialCellToBeOverwritten.mix(sourceSpecialCell);
            }
            // Writes the Special Cell.
            subsong.setSpecialCell(isSpeedTrack, position, targetCellIndex, sourceSpecialCell);
            modified = true;
        }

        if (!pasteContinuously) {
            mustContinue = false;
        } else {
            mustContinue = (targetCellIndex < positionHeight);
        }
    }

    return modified;
}

bool PasteCells::undo()
{
    // Puts the stored Cells "back".
    const auto subsongId = location.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        for (const auto&[cellLocationInPosition, cell] : oldCellsAndLocations) {
            jassert(cellLocationInPosition.getSubsongId() == subsongId);      // Terribly abnormal!
            subsong.setCell(cellLocationInPosition.getPositionIndex(), cellLocationInPosition.getLineIndex(), cellLocationInPosition.getChannelIndex(), cell);
        }

        // The same for the special Tracks.
        for (const auto&[localLocation, specialCell] : oldSpecialCellsAndLocations) {
            jassert(localLocation.getSubsongId() == subsongId);      // Terribly abnormal!
            subsong.setSpecialCell(localLocation.isSpeedTrack(), localLocation.getPositionIndex(), localLocation.getLineIndex(), specialCell);
        }
    });

    oldCellsAndLocations.clear();
    oldSpecialCellsAndLocations.clear();

    notifyListeners();

    return true;
}

void PasteCells::notifyListeners() const noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(location.getSubsongId());
    });
}

std::tuple<Cell, CaptureFlags, Cell> PasteCells::manageShiftedCursorOverCell(const Cell& baseTargetCell, const Cell& copiedCell, const CellCursorRank cursorRank,
                                                                      const CaptureFlags& baseCaptureFlags) noexcept
{
    auto newCaptureFlags = baseCaptureFlags;
    auto newCell = baseTargetCell;
    auto newCopiedCell = copiedCell;
    // A tricky case: if all or none effects are selected, we don't need to meddle with them.
    // However, as soon as one is not selected, the source effect becomes the first captured effect.
    // This is to allow pasting effects into another columns.
    auto firstSelectedEffectIndex = 0;
    const auto isEffect1Captured = baseCaptureFlags.isEffect1Captured();
    const auto isEffect2Captured = baseCaptureFlags.isEffect2Captured();
    const auto isEffect3Captured = baseCaptureFlags.isEffect3Captured();
    const auto isEffect4Captured = baseCaptureFlags.isEffect4Captured();

    if ((isEffect1Captured && isEffect2Captured && isEffect3Captured && isEffect4Captured)
        ||
        (!isEffect1Captured && !isEffect2Captured && !isEffect3Captured && !isEffect4Captured)) {
        firstSelectedEffectIndex = 0;
    } else {
        // Special case.
        firstSelectedEffectIndex = 0;
        if (!isEffect1Captured) {
            ++firstSelectedEffectIndex;
            if (!isEffect2Captured) {
                ++firstSelectedEffectIndex;
                if (!isEffect3Captured) {
                    ++firstSelectedEffectIndex;
                }
            }
        }
    }
    // How many effects to copy, for the shift? IMPORTANT: we consider there can be no "holes" in the selection.
    auto effectToCopyCount = 0;
    if (isEffect1Captured) {
        ++effectToCopyCount;
    }
    if (isEffect2Captured) {
        ++effectToCopyCount;
    }
    if (isEffect3Captured) {
        ++effectToCopyCount;
    }
    if (isEffect4Captured) {
        ++effectToCopyCount;
    }

    OptionalInt targetEffectIndex;

    switch (cursorRank) {
        case CellCursorRank::cursorOnInstrumentDigit1:
        case CellCursorRank::cursorOnInstrumentDigit2:
            // Must discard the note.
            newCaptureFlags = newCaptureFlags.withNoteCaptured(false);
            break;
        case CellCursorRank::cursorOnEffect1Number:
        case CellCursorRank::cursorOnEffect1Digit1:
        case CellCursorRank::cursorOnEffect1Digit2:
        case CellCursorRank::cursorOnEffect1Digit3:
            targetEffectIndex = 0;
            break;
        case CellCursorRank::cursorOnEffect2Number:
        case CellCursorRank::cursorOnEffect2Digit1:
        case CellCursorRank::cursorOnEffect2Digit2:
        case CellCursorRank::cursorOnEffect2Digit3:
            targetEffectIndex = 1;
            break;
        case CellCursorRank::cursorOnEffect3Number:
        case CellCursorRank::cursorOnEffect3Digit1:
        case CellCursorRank::cursorOnEffect3Digit2:
        case CellCursorRank::cursorOnEffect3Digit3:
            targetEffectIndex = 2;
            break;
        case CellCursorRank::cursorOnEffect4Number:
        case CellCursorRank::cursorOnEffect4Digit1:
        case CellCursorRank::cursorOnEffect4Digit2:
        case CellCursorRank::cursorOnEffect4Digit3:
            targetEffectIndex = 3;
            break;
        default:
        case CellCursorRank::cursorOnNote:
        case CellCursorRank::notPresent:
            // Nothing to do.
            break;
    }

    // Shift effects?
    if (targetEffectIndex.isPresent()) {
        const auto shift = targetEffectIndex.getValue() - firstSelectedEffectIndex;
        if (shift != 0) {
            // Shifts the effect in the source effect.
            newCopiedCell.shiftEffects(shift);
        }
        // Also shifts the capture flags, even if shift 0 (useful when pasting on effect but capturing the note and instr, they must not be pasted).
        newCaptureFlags = newCaptureFlags.withEffectShift(shift)
                .withNoteAndInstrumentCaptured(false, false);   // This could have been done above, but here it is shared all the code.
    }

    return { newCell, newCaptureFlags, newCopiedCell };
}

}   // namespace arkostracker
