#pragma once

#include <vector>

#include "../CursorLocation.h"

namespace arkostracker 
{

/** Represents a selection in the PatternViewer. It is immutable. */
class Selection     // TODO TU this (already started).
{
public:
    /** Builds an empty selection. */
    Selection() noexcept;

    /**
     * Creates a Selection with only a start. The end is the same.
     * @param line the line.
     * @param xLocation the X location.
     * @param createdByMouse true if the selection was created via mouse, false if via keyboard.
     */
    Selection(int line, const CursorLocation& xLocation, bool createdByMouse = false) noexcept;

    /**
     * Creates a Selection with only both start and end.
     * @param startLine the start line.
     * @param startXLocation the start X location.
     * @param endLine the end line. May be inferior or superior to the start line.
     * @param endXLocation the end X location.
     * @param createdByMouse true if the selection was created via mouse, false if via keyboard.
     * @param grown true if the selection has been grown. Cannot go back to false once it has been true.
     */
    Selection(int startLine, const CursorLocation& startXLocation, int endLine, const CursorLocation& endXLocation, bool createdByMouse, bool grown) noexcept;

    /**
     * @return a copy of the Selection with a new end. If was empty, sets both start and end to the given end (abnormal, so asserts).
     * @param endLine the end line.
     * @param endXLocation the end X location.
     */
    Selection withEnd(int endLine, const CursorLocation& endXLocation) const noexcept;

    /**
     * @return a copy of the Selection with a new creator.
     * @param createdByMouse true if the selection was created via mouse, false if via keyboard.
     */
    Selection withCreator(bool createdByMouse) const noexcept;

    /** @return true if there is a selection. */
    bool isPresent() const noexcept;
    /** @return true if there is no selection. */
    bool isEmpty() const noexcept;

    /**
     * @return true if the selection contains the line.
     * @param lineIndex the line index.
     */
    bool containsLine(int lineIndex) const noexcept;

    /** @return the top line (so, inferior to bottom line), or 0 if empty, which should have been tested earlier. */
    int getTopLine() const noexcept;
    /** @return the bottom line (so, superior to the top line), or 0 if empty, which should have been tested earlier. */
    int getBottomLine() const noexcept;
    /** @return the start line (no ordering), or 0 if empty, which should have been tested earlier. */
    //int getStartLine() const noexcept;
    /** @return the end line (no ordering), or 0 if empty, which should have been tested earlier. */
    int getEndLine() const noexcept;

    /** @return the left rank. We consider it is a CellCursor rank, but it must have been tested earlier! May be "not present". */
    CellCursorRank getLeftRankAsCellCursorRank() const noexcept;
    /** @return the right rank. We consider it is a CellCursor rank, but it must have been tested earlier! May be "not present". */
    CellCursorRank getRightRankAsCellCursorRank() const noexcept;

    /** @return the start location. */
    const CursorLocation& getStartLocation() const noexcept;
    /** @return the end location. */
    const CursorLocation& getEndLocation() const noexcept;

    /**
     * @return the selection for a specific track. Empty if there is none.
     * For example, the current selection may go from channel 0 to 1, so channel 2 must show an empty selection.
     * On the opposite, a selection going from 0 to 2, would return a whole selection of the rank of the channel 1.
     * Note that the original start/end ordering is lost in the built selection.
     * @param channelIndex the channel index.
     */
    Selection extractForTrack(int channelIndex) const noexcept;

    /**
     * @return the selection for a special track. Empty if there is none.
     * For example, the current selection may go from channel 0 to event track, both speed and event track are selected.
     * On the opposite, a selection going from 0 to speed track, would return empty if asking for the event track.
     * @param isSpeedTrack true if speed track, false if event track.
     */
    Selection extractForSpecialTrack(bool isSpeedTrack) const noexcept;

    /**
     * @return an instance of a Selection, on a Line Track.
     * @param startLine the start line.
     * @param endLine the end line. May be inferior to the start line.
     */
    static Selection buildForLineTrack(int startLine, int endLine) noexcept;

    /** @return an instance of a Selection for the whole pattern. */
    static Selection buildForWholePattern() noexcept;

    /** @return true if the selection is only one line and one rank. Empty selection is not considered single. */
    bool isSingle() const noexcept;

    /** @return true if the selection was created via mouse, false if via keyboard. */
    bool isCreatedByMouse() const noexcept;
    /** @return true if the selection has already been grown. */
    bool isGrown() const noexcept;

    /** @return the height of the selection. 0 if empty. */
    int getHeight() const noexcept;

    /**
     * @return a Selection which may be enlarged if to the start/end if it grows beyond one track. This sets the "grow" flag to on.
     * If not, returns an unmodified copy of the input selection.
     */
    Selection enlargeSelectionIfNeeded() const noexcept;

    /** @return a Selection with start/end are enlarged to the start of the left track, and end to the right track. This sets the "grow" flag to on. */
    Selection growSelectionHorizontally() const noexcept;
    /** @return a Selection with the height to the full pattern. This sets the "grow" flag to on. */
    Selection growHeight() const noexcept;

    bool operator==(const Selection& rhs) const;
    bool operator!=(const Selection& rhs) const;

    /**
     * @return the channel indexes related to this selection. There may be none!
     * @param channelCount how many channel there are (>0). Mandatory, because this information is not known to the selection.
     */
    std::vector<int> getChannelIndexes(int channelCount) const noexcept;

    /**
     * @return true if the selection encompasses a specific Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     */
    bool containsSpecialTrack(bool isSpeedTrack) const noexcept;

    /** @return true if the selection encompasses any Special Track. */
    bool containsSpecialTrack() const noexcept;

    /** @return true if the selection has exactly one channel selected. It does not test the nature of the channel (normal, special). Empty return false. */
    bool isSingleChannel() const noexcept;

    /** @return the channel index "to the left", if the start track is a Track, else absent. */
    OptionalInt getLeftStartChannelIndexIfOnTrack() const noexcept;

    /** @return true if the Selection is within the given height (top and bottom). Empty returns true. */
    bool isValid(int height) const noexcept;

private:
    /** @return the cursor location that in on "the left". This shouldn't be called if the selection is empty! */
    const CursorLocation& getLeftCursorLocation() const noexcept;
    /** @return the cursor location that in on "the left", but can be modified. This shouldn't be called if the selection is empty! */
    CursorLocation& getLeftCursorLocationNotConst() noexcept;
    /** @return the cursor location that in on "the right". This shouldn't be called if the selection is empty! */
    const CursorLocation& getRightCursorLocation() const noexcept;
    /** @return the cursor location that in on "the right", but can be modified. This shouldn't be called if the selection is empty! */
    CursorLocation& getRightCursorLocationNotConst() noexcept;

    /**
     * @return a cursor location, which has been put either the far left or far right of its channel.
     * @param inputCursorLocation the cursor location.
     * @param growToLeft true to put the cursor to the left, false to the right.
     */
    static CursorLocation growCursorLocation(const CursorLocation& inputCursorLocation, bool growToLeft) noexcept;

    /**
     * If needed, changes the internal locations to select a bit more: whole effect instead of a single column, for example.
     * It does NOT meddle with the "grow" state.
     */
    void correctOnAreas() noexcept;

    bool empty;                                         // True is there is no selection. Only true at the beginning.
    int startLine;                                      // The start line. May be inferior or superior to the end line.
    int endLine;                                        // The end line. May be inferior or superior to the start line.
    CursorLocation startCursorLocation;                 // The start X location.
    CursorLocation endCursorLocation;                   // The end X location.
    bool grown;                                         // True once more than one track is selected. Once it is grown, it never goes back.
    bool createdByMouse;                                // True if the selection was created via mouse, false if via keyboard.
};

}   // namespace arkostracker
