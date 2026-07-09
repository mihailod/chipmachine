#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../song/Location.h"
#include "../../../../song/cells/Cell.h"
#include "../../../../song/cells/SpecialCell.h"
#include "../../../../ui/patternViewer/CursorLocation.h"

namespace arkostracker 
{

class SongController;
class Subsong;

/** Generic class to remove/insert a Cell or Special Cell, shifting the rest. */
class InsertOrRemoveCellAt : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to access the Song.
     * @param location the location where to insert.
     * @param cursorLocation where the cursor is.
     */
    InsertOrRemoveCellAt(SongController& songController, Location location, const CursorLocation& cursorLocation) noexcept;

    bool perform() override;
    bool undo() override;

protected:

    /**
     * Performs the action on a normal Track.
     * @param subsong the Subsong.
     * @param trackIndex the index of the track to modify.
     * @param cellIndex the cell index where the action is performed.
     * @return the Cell that is getting lost.
     */
    virtual Cell performActionOnTrack(Subsong& subsong, int trackIndex, int cellIndex) noexcept = 0;

    /**
     * Performs the action on a Special Track.
     * @param subsong the Subsong.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the index of the track to modify.
     * @param cellIndex the cell index where the action is performed.
     * @return the Cell that is getting lost.
     */
    virtual SpecialCell performActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex) noexcept = 0;

    /**
     * Performs the action for the Undo, on a normal Track.
     * @param subsong the Subsong.
     * @param trackIndex the index of the track to modify.
     * @param cellIndex the cell index where the action is performed.
     * @param cellToInsert the Cell that was lost and must be shown again.
     */
    virtual void undoActionOnNormalTrack(Subsong& subsong, int trackIndex, int cellIndex, const Cell& cellToInsert) noexcept = 0;

    /**
     * Performs the action for the Undo, on a Special Track.
     * @param subsong the Subsong.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the index of the track to modify.
     * @param cellIndex the cell index where the action is performed.
     * @param specialCellToInsert the Special Cell that was lost and must be shown again.
     */
    virtual void undoActionOnSpecialTrack(Subsong& subsong, bool isSpeedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCellToInsert) = 0;

private:
    /** Performs the Do on a normal track type. */
    void performOnNormalTrackType() noexcept;

    /**
     * Performs the Do on a Special Track type.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     */
    void performOnSpecialTrackType(bool isSpeedTrack) noexcept;

    /** Performs the Undo on a normal track type. */
    void undoOnNormalTrackType() noexcept;

    /**
     * Performs the Undo on a Special Track type.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     */
    void undoOnSpecialTrackType(bool isSpeedTrack) noexcept;

    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;
    const Location location;
    const CursorLocation cursorLocation;

    Cell removedCell;
    SpecialCell removedSpecialCell;
    int trackIndexToPerformOn;          // For both normal and Special Tracks.
};

}   // namespace arkostracker
