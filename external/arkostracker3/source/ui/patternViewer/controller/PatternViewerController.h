#pragma once

#include "../../../business/actions/patternViewer/TransposeRate.h"
#include "../../../controllers/SongController.h"
#include "../../../song/cells/Effect.h"
#include "../../utils/ParentViewLifeCycleAware.h"
#include "../view/cells/cell/CellCursorRank.h"
#include "../view/cells/simpleCell/SpecialCellCursorRank.h"

namespace arkostracker 
{

/** Abstract Controller of the Pattern Viewer. */
class PatternViewerController : public ParentViewLifeCycleAware
{
public:
    /** @return the Song Controller. */
    virtual SongController& getSongController() = 0;

    /**
     * The user left-clicked on the channel number. It will toggle its mute state.
     * @param channelIndex the channel index (>=0).
     */
    virtual void onUserLeftClickedOnChannelNumber(int channelIndex) = 0;

    /**
     * The user right-clicked on the channel number. It will solo/un-solo it.
     * @param channelIndex the channel index (>=0).
     */
    virtual void onUserRightClickedOnChannelNumber(int channelIndex) = 0;

    /**
     * The user scrolled with the mouse wheel.
     * @param scrollToBottom true to go to the bottom, false to go up.
     */
    virtual void onUserWantsToScrollVerticallyWithMouseWheel(bool scrollToBottom) = 0;

    /**
     * The user wants to move vertically.
     * @param goDown true if down, false if up.
     * @param fast false if normal, true if fast motion.
     * @param enlargeSelection true to enlarge the selection.
     */
    virtual void onUserWantsToMoveVertically(bool goDown, bool fast, bool enlargeSelection) = 0;
    /**
     * The user wants to move vertically.
     * @param cellIndex the cell index. May be out of bounds.
     * @param enlargeSelection true to enlarge the selection.
     */
    virtual void onUserWantsToMoveVerticallyTo(int cellIndex, bool enlargeSelection) = 0;

    /**
     * The user wants to move horizontally.
     * @param goToRight true to go right, false to go left.
     * @param enlargeSelection true to enlarge the selection.
     */
    virtual void onUserWantsToMoveHorizontally(bool goToRight, bool enlargeSelection) = 0;

    /**
     * The user wants to move horizontally.
     * @param next true for next channel, false for previous.
     */
    virtual void onUserWantsToMoveToNextChannel(bool next) = 0;

    /**
     * The user wants to move to a next position.
     * @param next true for next position, false for previous.
     */
    virtual void onUserWantsToMoveToNextPosition(bool next) = 0;

    /**
     * Called when the user clicked on a specific rank of the Cell, for a music track.
     * @param channelIndex the channel of the Track (>=0).
     * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
     * @param cursorRank the rank.
     * @param leftButton true if left Button, false if right.
     * @param shift true if shift is pressed.
     */
    virtual void onUserClickedOnRankOfNormalTrack(int channelIndex, int rankFromCenterAsOrigin, CellCursorRank cursorRank, bool leftButton, bool shift) = 0;

    /**
     * Called when the user click is now up. Only called after a down has been clicked.
     * This is called from the callback of a normal track, but it does NOT mean the cell under the cursor is a normal track! It can be anything!
     * @param leftButton true if left Button, false if right.
     */
    virtual void onUserClickIsUpFromNormalTrack(bool leftButton) = 0;

    /**
     * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
     * It is up to the client to find it.
     * This is called from the callback of a normal track, but it does NOT mean the cell under the cursor is a normal track! It can be anything!
     * @param event the mouse event.
     */
    virtual void onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event) = 0;

    /**
     * Called when the user clicked on a specific rank of the Cell, for a special track.
     * @param isSpeedTrack true is Speed Track, false if Event Track.
     * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
     * @param cursorRank the rank.
     * @param leftButton true if left Button, false if right.
     * @param shift true if shift is pressed.
     */
    virtual void onUserClickedOnRankOfSpecialTrack(bool isSpeedTrack, int rankFromCenterAsOrigin, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) = 0;

    /**
     * Called when the user click is now up. Only called after a down has been clicked.
     * @param leftButton true if left Button, false if right.
     */
    virtual void onUserClickIsUpFromSpecialTrack(bool leftButton) = 0;

    /**
     * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
     * It is up to the client to find it.
     * @param event the mouse event.
     */
    virtual void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) = 0;

    /**
     * Called when the user clicked on a specific rank of the Cell.
     * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
     * @param leftButton true if left Button, false if right.
     */
    virtual void onUserClickedOnRankOfLineTrack(int rankFromCenterAsOrigin, bool leftButton) = 0;

    /**
     * Called when the user click is now up. Only called after a down has been clicked.
     * @param leftButton true if left Button, false if right.
     */
    virtual void onUserClickIsUpFromLineTrack(bool leftButton) = 0;

    /**
     * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
     * It is up to the client to find it.
     * @param event the mouse event.
     */
    virtual void onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event) = 0;

    /** The user wants to toggle the minimize state of the selected Track (where the cursor is, or selection). */
    virtual void onUserWantsToToggleMinimize() = 0;
    /** The user wants to toggle the Record state. */
    virtual void onUserWantsToToggleRecord() = 0;
    /** The user wants to toggle the Follow mode. */
    virtual void onUserWantsToToggleFollow() = 0;
    /** The user wants to toggle the Write Instrument state. */
    virtual void onUserWantsToToggleWriteInstrument() = 0;
    /** The user wants to toggle the Overwrite Default Effect state. */
    virtual void onUserWantsToToggleOverwriteDefaultEffect() = 0;
    /** The user wants to go to the next/previous default effect. */
    virtual void onUserWantsToChangeDefaultEffect(bool next) = 0;

    /** @return true if Recording is on. */
    virtual bool isRecording() = 0;

    /**
     * The user wants to delete where the cursor is.
     * @param wholeCell true to delete the whole cell.
     */
    virtual void onUserWantsToDelete(bool wholeCell) = 0;

    /**
     * The user wants to modify the Track height.
     * @param newHeight the new height.
     */
    virtual void onUserWantsToModifyTrackHeight(int newHeight) = 0;
    /** The user wants to modify the Track height, with the given offset. */
    virtual void onUserWantsToModifyTrackHeightWithOffset(int offset) = 0;

    /**
     * The user wants to modify a transposition.
     * @param channelIndex the channel index.
     * @param newTransposition the new transposition.
     */
    virtual void onUserWantsToModifyTransposition(int channelIndex, int newTransposition) = 0;

    /**
     * The user wants to rename a Track.
     * @param channelIndex the channel index.
     * @param newName the new name.
     * @param newColor the new possible color.
     */
    virtual void onUserWantsToSetTrackNameAndColor(int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) = 0;

    /**
     * The user wants to rename a Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param newName the new name.
     */
    virtual void onUserWantsToRenameSpecialTrack(bool isSpeedTrack, const juce::String& newName) = 0;

    /**
     * Creates a Track Link.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param sourceChannelIndex the channel index from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     * @param targetChannelIndex the channel index to link to.
     */
    virtual void onUserWantsToCreateLink(int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) = 0;

    /**
     * Unlinks a linked Track.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     * @param channelIndexToUnlink the channel index to remove the link from.
     */
    virtual void onUserWantsToUnlink(int positionIndexToUnlink, int channelIndexToUnlink) noexcept = 0;

    /**
     * Creates a Special Track Link.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     */
    virtual void onUserWantsToCreateSpecialLink(bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) = 0;

    /**
     * Unlinks a linked Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     */
    virtual void onUserWantsToUnlinkSpecial(bool isSpeedTrack, int positionIndexToUnlink) noexcept = 0;

    /**
     * Goes to a Track.
     * @param positionIndexToGoTo the index of the position to go to.
     * @param channelIndexToGoTo the channel index to go to.
     */
    virtual void onUserWantsToGoTo(int positionIndexToGoTo, int channelIndexToGoTo) noexcept = 0;

    /**
     * Goes to a channel. Nothing else moves.
     * @param channelIndexToGoTo the channel index to go to.
     */
    virtual void onUserWantsToGoToChannel(int channelIndexToGoTo) noexcept = 0;

    /**
     * Goes to a Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param positionIndexToGoTo the index of the position to go to.
     */
    virtual void onUserWantsToGoToSpecialTrack(bool isSpeedTrack, int positionIndexToGoTo) noexcept = 0;

    /**
     * Called when a (PC) keyboard note was detected.
     * @param baseNote the base note (to which the current octave must be added).
     */
    virtual void onKeyboardNote(int baseNote) = 0;

    /**
     * Called when a note was detected (a MIDI note for example).
     * @param note a full note.
     */
    virtual void onNote(int note) = 0;

    /**
     * Called when a key is pressed. This is called BEFORE JUCE manages any command, so it takes precedence over them.
     * @param key the pressed key.
     * @return true if the key is consumed. False to let the parent handle it.
     */
    virtual bool onKeyPressed(const juce::KeyPress& key) = 0;

    /** The user wants to play the current line. Will then move to the next line plus "step". */
    virtual void onUserWantsToPlayLine() = 0;
    /**
     * The user wants to play/stop the current pattern, from either the top, or the selection block.
     * If there is no block and forceSetBlock is false, plays the pattern from start.
     * If there is a block and forceSetBlock is false, plays from the block.
     * @param forceSetBlock if true, sets the block from the cursor even if there is a block already.
     */
    virtual void onUserWantsToTogglePlayPatternFromCursorOrBlock(bool forceSetBlock) = 0;
    /**
     * The user wants to play/stop the current pattern, from start. The possible block is ignored.
     * This is a shortcut to the global Play Pattern, plus a toggle.
     */
    virtual void onUserWantsToTogglePlayPatternFromStart() = 0;

    /** The user wants to copy the selection. */
    virtual void onUserWantsToCopy() = 0;
    /** The user wants to cut the selection. */
    virtual void onUserWantsToCut() = 0;
    /**
     * The user wants to paste the selection.
     * @param moveBelowAfter true to move below the copy after.
     * @param pasteContinuously true to paste up to the position height.
     */
    virtual void onUserWantsToPaste(bool moveBelowAfter, bool pasteContinuously) = 0;
    /** The user wants to paste-mix the selection. */
    virtual void onUserWantsToPasteMix() = 0;
    /** The user wants to delete what is selected. */
    virtual void onUserWantsToClearSelection() = 0;

    /**
     * The user wants to transpose what is selected.
     * @param transposeRate of how many fast to transpose.
     */
    virtual void onUserWantsToTransposeSelection(TransposeRate transposeRate) = 0;

    /** The user wants to toggle the read-only tracks that are selected. */
    virtual void onUserWantsToToggleReadOnly() = 0;

    /** The user wants to generate an arpeggio from the selection. This should be called only if the selection is valid, but a further test is performed. */
    virtual void onUserWantsToGenerateArpeggio() = 0;
    /** The user wants to generate an instrument from the selection. This should be called only if the selection is valid, but a further test is performed. */
    virtual void onUserWantsToGenerateInstrument() = 0;

    /** The user wants to name a Track from the selection. This should be called only if the selection is valid, but a further test is performed. */
    virtual void onUserWantsToSetTrackNameAndColor() = 0;
    /** The user wants to link a Track from the selection. This should be called only if the selection is valid, but a further test is performed. */
    virtual void onUserWantsToLinkTrack() = 0;

    /** The user wants to insert an empty cell where the cursor is. */
    virtual void onUserWantsToInsert() = 0;
    /** The user wants to remove the cell where the cursor is, shifting the cells. */
    virtual void onUserWantsToRemove() = 0;

    /**
     * The user wants to set a new edit step value.
     * @param stepToValidate the step value. May be invalid!
     */
    virtual void onUserWantsToSetEditStep(int stepToValidate) = 0;
    /**
     * The users wants to add a number to the current edit step value.
     * @param offset the offset to add. May be negative.
     */
    virtual void onUserWantsToIncreaseEditStep(int offset) = 0;

    /**
     * The user selected another locked effect.
     * @param effect the effect. Must not be "no effect" or "legato". Only effects that can be written by the user.
     */
    virtual void onUserSelectedLockedEffect(Effect effect) = 0;

    /** The user wants to solo/un-solo the track where the cursor is. */
    virtual void onUserWantsToToggleSoloTrack() = 0;
    /** The user wants to mute/unmute the track where the cursor is. */
    virtual void onUserWantsToToggleMuteTrack() = 0;

    /** The user wants to select the track where the cursor is. It may be several tracks */
    virtual void onUserWantsToSelectTrack() = 0;
    /** The user wants to select all the tracks. */
    virtual void onUserWantsToSelectAllTracks() = 0;

    /** The user wants to capture the effect, expression, or nearest instrument. */
    virtual void onUserWantsToCapture() = 0;

    /** The user wants to write an RST. */
    virtual void onUserWantsToWriteRst() = 0;

    /** Grabs the keyboard focus. */
    virtual void getKeyboardFocus() noexcept = 0;

    /**
     * @return true if the Track at the given channel index is in read-only.
     * @param channelIndex the channel index.
     */
    virtual bool isReadOnlyTrack(int channelIndex) noexcept = 0;

    /** @return true if the current selection allows generating arpeggios. */
    virtual bool canGenerateArpeggioFromSelection() noexcept = 0;
    /** @return true if the current selection allows generating instruments. */
    virtual bool canGenerateInstrumentFromSelection() noexcept = 0;

    /** @return true if the current selection allows naming/coloring the track. */
    virtual bool canSetNameAndColorToTrack() noexcept = 0;
    /** @return true if the current selection allows linking the track. */
    virtual bool canLinkTrack() noexcept = 0;

    /**
     * @return the track index from the given channel index.
     * @param channelIndex the channel index. Must be valid.
     */
    virtual int getTrackIndex(int channelIndex) noexcept = 0;

    /** @return the displayed position index. */
    virtual int getPositionIndex() noexcept = 0;

    /** @return the ID of the displayed Subsong. */
    virtual Id getSubsongId() noexcept = 0;

    /** @return the locked effect (never noEffect or legato). */
    virtual Effect getDefaultEffect() const noexcept = 0;

    /** @return the effects, in the order they are displayed. */
    virtual const std::vector<Effect>& getDefaultEffects() const noexcept = 0;

    /**
     * Opens or closes the toolbox.
     * @param open true to open, false to close.
     * @param parent the parent where to add the view.
     * @param toolBoxButtonX as an indication of where to display the toolbox, the X of the toolbox button.
     * @param toolBoxButtonY as an indication of where to display the toolbox, the Y of the toolbox button.
     */
    virtual void showOrHideToolbox(bool open, juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept = 0;

    /**
     * Locates the toolbox, shown or not. Useful when resizing.
     * @param toolBoxButtonX as an indication of where to display the toolbox, the X of the toolbox button.
     * @param toolBoxButtonY as an indication of where to display the toolbox, the Y of the toolbox button.
     */
    virtual void locateToolbox(int toolBoxButtonX, int toolBoxButtonY) noexcept = 0;

    /** Triggers a warning (animation) because an action is performed whereas the recording is off. */
    virtual void showWarningBecauseRecordingOff() noexcept = 0;
};

}   // namespace arkostracker

