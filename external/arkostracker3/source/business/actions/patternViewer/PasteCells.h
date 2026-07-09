#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/CellLocationInPosition.h"
#include "../../../song/Location.h"
#include "../../../song/SpecialCellLocationInPosition.h"
#include "../../../ui/patternViewer/CursorLocation.h"
#include "../../../ui/patternViewer/controller/PasteData.h"

namespace arkostracker 
{

class SongController;
class Subsong;

/** Action to paste cells into a location given, probably from the PatternViewer. */
class PasteCells final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller, to modify the song.
     * @param location where to paste.
     * @param cursorLocation where to paste occurs.
     * @param pasteData the data to paste.
     * @param pasteMix true to paste-mix (empty parts of the cells are not pasted).
     * @param pasteContinuously true to paste continuously (up to the position height).
     */
    PasteCells(SongController& songController, Location location, const CursorLocation& cursorLocation, PasteData pasteData, bool pasteMix, bool pasteContinuously) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    /**
     * @return a new Cell, which may have its effect shifted and some parts removed, according to the cursor position.
     * Also returns the new capture flags, and a shifted base copied Cell.
     * @param baseTargetCell the base target cell.
     * @param copiedCell the copied cell, in order to get its effects.
     * @param cursorRank where the cursor is.
     * @param captureFlags the base capture flags.
     */
    static std::tuple<Cell, CaptureFlags, Cell> manageShiftedCursorOverCell(const Cell& baseTargetCell, const Cell& copiedCell, CellCursorRank cursorRank,
                                                                     const CaptureFlags& captureFlags) noexcept;

    /**
     * Pastes a Special Track, if needed. Warning, this is called during a Subsong transaction.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsong the Subsong.
     * @param specialCells the Cells to paste.
     * @param subsongId the id of the Subsong.
     * @param position the position.
     * @param pasteLineIndex the line where to paste.
     * @param positionHeight the height of the position.
     * @return true if a modification has been performed.
     */
    bool pasteSpecialTrackIfNeeded(bool isSpeedTrack, Subsong& subsong, const std::vector<SpecialCell>& specialCells, const Id& subsongId,
        int position, int pasteLineIndex, int positionHeight) noexcept;

    SongController& songController;

    const Location location;
    const CursorLocation pasteCursorLocation;
    const PasteData pasteData;
    const bool pasteMix;
    const bool pasteContinuously;

    std::vector<std::pair<CellLocationInPosition, Cell>> oldCellsAndLocations;                          // Stored for the Undo.
    std::vector<std::pair<SpecialCellLocationInPosition, SpecialCell>> oldSpecialCellsAndLocations;     // Stored for the Undo.
};

}   // namespace arkostracker
