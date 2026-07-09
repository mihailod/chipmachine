#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/CellLocationInPosition.h"
#include "../../../song/SpecialCellLocationInPosition.h"
#include "../../../song/cells/Cell.h"
#include "../../../song/cells/SpecialCell.h"
#include "../../../ui/patternViewer/controller/CaptureFlags.h"
#include "SelectedData.h"

namespace arkostracker 
{

class SongController;
class Subsong;

/**
 * An abstract action to modify cells of a position, and next ones if wanted, from a selection if wanted.
 * Subclasses can define if a cell must be modified or not.
 */
class CellsModifier : public juce::UndoableAction
{
public:

    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param selectedData represents what is selected.
     */
    CellsModifier(SongController& songController, SelectedData selectedData) noexcept;

    // UndoableAction method implementations.
    // =========================================
    bool perform() override;
    bool undo() override;

protected:
    /**
     * Subclass must indicate whether the given cell should be modified or not.
     * @param inputCell the input cell.
     * @param captureFlags the capture flags. It is up to the client to take it in account or ignore it.
     * @return the output cell, or nullptr if no change needs to be performed.
     */
    virtual std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept = 0;

    /**
     * Subclass must indicate whether the given Special Cell should be modified or not. The default implementation does nothing, since there are not many
     * operations on them.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param inputSpecialCell the input Special Cell.
     * @return the output Special Cell, or nullptr if no change needs to be performed.
     */
    virtual std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept;

    /** @return the selected data. */
    const SelectedData& getSelectedData() const noexcept;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    /**
     * Browsing the selection on the Special Tracks. This is called by the main browsing code, so this is done in a transaction!
     * @param subsong the Subsong.
     * @param alreadyBrowsedSpecialTrackIndexesToFill contains the already browsed special track indexes. Will be filled if necessary.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsongId the Subsong index.
     * @param positionIndex the position index.
     * @param startLine the start line.
     * @param endLine the end line.
     * @return true if a Special Cell has been modified.
     */
    bool browseSelectionOnSpecialTrack(Subsong& subsong, std::unordered_set<int>& alreadyBrowsedSpecialTrackIndexesToFill, bool isSpeedTrack, const Id& subsongId,
        int positionIndex, int startLine, int endLine) noexcept;

    SongController& songController;
    const SelectedData selectedData;

    std::vector<std::pair<CellLocationInPosition, Cell>> oldCellsAndLocations;                          // Stored for the Undo.
    std::vector<std::pair<SpecialCellLocationInPosition, SpecialCell>> oldSpecialCellsAndLocations;     // Stored for the Undo.
};

}   // namespace arkostracker
