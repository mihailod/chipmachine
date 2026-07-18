#pragma once

#include "../../../utils/OptionalValue.h"
#include "../../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

class SongController;

/**
 * Abstract Controller for the Linker. It also creates the LinkerView, when onPanelCreated is called.
 * Important, when the Panel is destroyed, it must notify the Controller to delete the UI via the onPanelDeleted.
 */
class LinkerController : public ParentViewLifeCycleAware
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     */
    explicit LinkerController(SongController& songController) noexcept;

    /**
     * The user wants to set the loop start and/or end. This performs the action, if possible.
     * @param newLoopStartPosition the new loop start position, if given.
     * @param newEndPosition the new end position, if given.
     */
    virtual void onWantToSetLoopStartOrEnd(OptionalInt newLoopStartPosition, OptionalInt newEndPosition) noexcept = 0;

    /**
     * Called when the user clicked on a Pattern item.
     * @param position the position the item is linked to.
     * @param down true if the button is down, false if up.
     * @param shift true if Shift if pressed.
     * @param control true if Control if pressed.
     */
    virtual void onPatternItemClicked(int position, bool down, bool shift, bool control) noexcept = 0;

    /**
     * @return true if there is one item selected.
     * @param onlyOneAuthorized true to restrict to only one selected item. False if several items can be selected.
     */
    virtual bool isOneItemSelected(bool onlyOneAuthorized) const noexcept = 0;

    /**
     * Called when the user double-clicked on a Pattern item.
     * @param position the position the item is linked to.
     */
    virtual void onPatternItemDoubleClicked(int position) noexcept = 0;

    /** Called when the user want to open the pattern viewer. */
    virtual void onWantToOpenPatternViewer() noexcept = 0;

    /**
     * Called when the user wants to edit a position.
     * @param positionIndex the position the item is linked to.
     */
    virtual void onWantToEditPosition(int positionIndex) = 0;

    /**
     * Called when the user wants to edit a position.
     * @param positionIndexes the position indexes to edit.
     */
    virtual void onWantToEditPositions(const std::set<int>& positionIndexes) = 0;

    /**
     * Called when the use right-clicked on a Pattern item.
     * @param position the position the item is linked to.
     */
    virtual void onPatternItemRightClicked(int position) noexcept = 0;

    /**
     * Called when the Add New icon is clicked.
     * @param position the index of the selected position.
     */
    virtual void onAddNewPositionClicked(int position) noexcept = 0;

    /**
     * Called when the Clone icon is clicked.
     * @param position the index of the selected position.
     */
    virtual void onClonePositionClicked(int position) noexcept = 0;

    /**
     * Called when the Repeat icon is clicked.
     * @param position the index of the selected position.
     */
    virtual void onRepeatPositionClicked(int position) noexcept = 0;
    /**
     * Called when the user made a drag'n'drop. Only one Position is given (which SHOULD be selected),
     * but the whole selection will be used. The original position is actually only used to correct the offset
     * with the destination Position.
     * @param originalPosition the position of the original item.
     * @param destinationPosition the position of the pointed-to item. Note that, if this is superior to the original,
     * it should be considered "after" this destination position.
     * @param duplicate true to duplicate, false to move.
     */
    virtual void onWantToMoveOrDuplicateSelectedPositions(int originalPosition, int destinationPosition, bool duplicate) = 0;

    /**
     * Called when the user wants to increase/decrease the Pattern value. It may be accepted or not (out of bounds for example).
     * @param position the Position where to change the Pattern.
     * @param offset may be positive or negative.
     */
    virtual void onWantToIncDecPatternItemValue(int position, int offset) = 0;

    /** Duplicates the selected positions. */
    virtual void duplicateSelectedPositions() noexcept = 0;
    /** Clones the selected positions. */
    virtual void cloneSelectedPositions() noexcept = 0;
    /** Deletes the selected positions. */
    virtual void deleteSelectedPositions() noexcept = 0;
    /** Creates a new position and pattern. */
    virtual void createNew() noexcept = 0;

    /** Edits the selected Position. If there are several, the most "logical" is edited. */
    virtual void editSelectedPosition() noexcept = 0;
    /**
     * Edits one or more Positions.
     * @param positionIndexes the positions to edit.
     */
    virtual void editPositions(const std::set<int>& positionIndexes) noexcept = 0;

    /**
     * Tries to move the selection by one item.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToMoveSelection(bool forward) noexcept = 0;
    /**
     * Tries to move the selection by several items.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToMoveSelectionFast(bool forward) noexcept = 0;
    /**
     * Tries to move the selection to the limit.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToMoveSelectionToLimit(bool forward) noexcept = 0;

    /** Marks the selection as the loop start/end. Nothing happens if there is no selection. */
    virtual void markSelectionAsLoopStartAndEnd() noexcept = 0;
    /** Marks the selection as loop start. Nothing happens if there is no selection. */
    virtual void markLoopStart() noexcept = 0;
    /** Marks the selection as end. Nothing happens if there is no selection. */
    virtual void markEnd() noexcept = 0;

    /**
     * The user wants to increase the pattern index.
     * @param step how much to increase. May be negative.
     */
    virtual void onWantToIncreasePatternIndex(int step) noexcept = 0;

    /**
     * Tries to grow the selection by one item.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToGrowSelection(bool forward) noexcept = 0;
    /**
     * Tries to grow the selection by several items.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToGrowSelectionFast(bool forward) noexcept = 0;
    /**
     * Tries to grow the selection to the limit.
     * @param forward true to move forward, false for backward.
     */
    virtual void onWantToGrowSelectionToLimit(bool forward) noexcept = 0;

    /** Copies the selection to clipboard. If no selection, nothing happens. */
    virtual void copySelectionToClipboard() noexcept = 0;
    /** Cuts the selection to clipboard. If no selection, nothing happens. */
    virtual void cutSelectionToClipboard() noexcept = 0;
    /** Pastes the clipboard to where the cursor is. If the clipboard cannot be deserialized, silently fails. */
    virtual void pasteClipboard() noexcept = 0;

    /** @return true if the Cut operation on selection can be performed. */
    virtual bool canCutSelection() const noexcept = 0;
    /** @return true if the Delete operation on selection can be performed. */
    virtual bool canDeleteSelection() const noexcept = 0;

    /** Grabs the keyboard focus. */
    virtual void getKeyboardFocus() noexcept = 0;

protected:
    SongController& songController;    // NOLINT(*-non-private-member-variables-in-classes)
};

}   // namespace arkostracker
