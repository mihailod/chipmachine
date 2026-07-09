#include "PatternViewerControllerNoOp.h"

namespace arkostracker
{

SongController& PatternViewerControllerNoOp::getSongController()
{
    return songController;
}

void PatternViewerControllerNoOp::onParentViewCreated(BoundedComponent& /*parentView*/)
{
}

void PatternViewerControllerNoOp::onParentViewResized(int /*startX*/, int /*startY*/, int /*newWidth*/, int /*newHeight*/)
{
}

void PatternViewerControllerNoOp::onParentViewFirstResize(juce::Component& /*parentView*/)
{
}

void PatternViewerControllerNoOp::onParentViewDeleted() noexcept
{
}

void PatternViewerControllerNoOp::onUserLeftClickedOnChannelNumber(int /*channelIndex*/)
{
}

void PatternViewerControllerNoOp::onUserRightClickedOnChannelNumber(int /*channelIndex*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToScrollVerticallyWithMouseWheel(bool /*scrollToBottom*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToMoveVertically(bool /*goDown*/, bool /*fast*/, bool /*enlargeSelection*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToMoveVerticallyTo(int /*cellIndex*/, bool /*enlargeSelection*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToMoveHorizontally(bool /*goToRight*/, bool /*enlargeSelection*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToMoveToNextChannel(bool /*next*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToMoveToNextPosition(bool /*next*/)
{
}

void PatternViewerControllerNoOp::onUserClickedOnRankOfNormalTrack(int /*channelIndex*/, int /*rankFromCenterAsOrigin*/, CellCursorRank /*cursorRank*/, bool /*leftButton*/, bool /*shift*/)
{
}

void PatternViewerControllerNoOp::onUserClickIsUpFromNormalTrack(bool /*leftButton*/)
{
}

void PatternViewerControllerNoOp::onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& /*event*/)
{
}

void PatternViewerControllerNoOp::onUserClickedOnRankOfSpecialTrack(bool /*isSpeedTrack*/, int /*rankFromCenterAsOrigin*/, SpecialCellCursorRank /*cursorRank*/, bool /*leftButton*/, bool /*shift*/)
{
}

void PatternViewerControllerNoOp::onUserClickIsUpFromSpecialTrack(bool /*leftButton*/)
{
}

void PatternViewerControllerNoOp::onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& /*event*/)
{
}

void PatternViewerControllerNoOp::onUserClickedOnRankOfLineTrack(int /*rankFromCenterAsOrigin*/, bool /*leftButton*/)
{
}

void PatternViewerControllerNoOp::onUserClickIsUpFromLineTrack(bool /*leftButton*/)
{
}

void PatternViewerControllerNoOp::onUserDraggedCursorFromLineTrack(const juce::MouseEvent& /*event*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleMinimize()
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleRecord()
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleFollow()
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleWriteInstrument()
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleOverwriteDefaultEffect()
{
}

void PatternViewerControllerNoOp::onUserWantsToChangeDefaultEffect(bool /*next*/)
{
}

bool PatternViewerControllerNoOp::isRecording()
{
    return false;
}

void PatternViewerControllerNoOp::onUserWantsToDelete(bool /*wholeCell*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToModifyTrackHeight(int /*newHeight*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToModifyTrackHeightWithOffset(int /*offset*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToModifyTransposition(int /*channelIndex*/, int /*newTransposition*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToSetTrackNameAndColor(int /*channelIndex*/, const juce::String& /*newName*/, const OptionalValue<juce::Colour>& /*newColor*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToRenameSpecialTrack(bool /*isSpeedTrack*/, const juce::String& /*newName*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToCreateLink(int /*sourcePositionIndex*/, int /*sourceChannelIndex*/, int /*targetPositionIndex*/, int /*targetChannelIndex*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToUnlink(int /*positionIndexToUnlink*/, int /*channelIndexToUnlink*/) noexcept
{
}

void PatternViewerControllerNoOp::onUserWantsToCreateSpecialLink(bool /*isSpeedTrack*/, int /*sourcePositionIndex*/, int /*targetPositionIndex*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToUnlinkSpecial(bool /*isSpeedTrack*/, int /*positionIndexToUnlink*/) noexcept
{
}

void PatternViewerControllerNoOp::onUserWantsToGoTo(int /*positionIndexToGoTo*/, int /*channelIndexToGoTo*/) noexcept
{
}

void PatternViewerControllerNoOp::onUserWantsToGoToChannel(int /*channelIndexToGoTo*/) noexcept
{
}

void PatternViewerControllerNoOp::onUserWantsToGoToSpecialTrack(bool /*isSpeedTrack*/, int /*positionIndexToGoTo*/) noexcept
{
}

void PatternViewerControllerNoOp::onKeyboardNote(int /*baseNote*/)
{
}

void PatternViewerControllerNoOp::onNote(int /*note*/)
{
}

bool PatternViewerControllerNoOp::onKeyPressed(const juce::KeyPress& /*key*/)
{
    return false;
}

void PatternViewerControllerNoOp::onUserWantsToPlayLine()
{
}

void PatternViewerControllerNoOp::onUserWantsToTogglePlayPatternFromCursorOrBlock(bool /*forceSetBlock*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToTogglePlayPatternFromStart()
{
}

void PatternViewerControllerNoOp::onUserWantsToCopy()
{
}

void PatternViewerControllerNoOp::onUserWantsToCut()
{
}

void PatternViewerControllerNoOp::onUserWantsToPaste(bool /*moveBelowAfter*/, bool /*pasteContinuously*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToPasteMix()
{
}

void PatternViewerControllerNoOp::onUserWantsToClearSelection()
{
}

void PatternViewerControllerNoOp::onUserWantsToTransposeSelection(TransposeRate /*transposeRate*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleReadOnly()
{
}

void PatternViewerControllerNoOp::onUserWantsToGenerateArpeggio()
{
}

void PatternViewerControllerNoOp::onUserWantsToGenerateInstrument()
{
}

void PatternViewerControllerNoOp::onUserWantsToSetTrackNameAndColor()
{
}

void PatternViewerControllerNoOp::onUserWantsToLinkTrack()
{
}

void PatternViewerControllerNoOp::onUserWantsToInsert()
{
}

void PatternViewerControllerNoOp::onUserWantsToRemove()
{
}

void PatternViewerControllerNoOp::onUserWantsToSetEditStep(int /*stepToValidate*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToIncreaseEditStep(int /*offset*/)
{
}

void PatternViewerControllerNoOp::onUserSelectedLockedEffect(Effect /*effect*/)
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleSoloTrack()
{
}

void PatternViewerControllerNoOp::onUserWantsToToggleMuteTrack()
{
}

void PatternViewerControllerNoOp::onUserWantsToSelectTrack()
{
}

void PatternViewerControllerNoOp::onUserWantsToSelectAllTracks()
{
}

void PatternViewerControllerNoOp::onUserWantsToCapture()
{
}

void PatternViewerControllerNoOp::onUserWantsToWriteRst()
{
}

void PatternViewerControllerNoOp::getKeyboardFocus() noexcept
{
}

bool PatternViewerControllerNoOp::isReadOnlyTrack(int /*channelIndex*/) noexcept
{
    return false;
}

bool PatternViewerControllerNoOp::canGenerateArpeggioFromSelection() noexcept
{
    return false;
}

bool PatternViewerControllerNoOp::canGenerateInstrumentFromSelection() noexcept
{
    return false;
}

int PatternViewerControllerNoOp::getTrackIndex(int /*channelIndex*/) noexcept
{
    return 0;
}

const std::vector<Effect>& PatternViewerControllerNoOp::getDefaultEffects() const noexcept
{
    static const std::vector<Effect> effects;
    return effects;
}

int PatternViewerControllerNoOp::getPositionIndex() noexcept
{
    return 0;
}

Id PatternViewerControllerNoOp::getSubsongId() noexcept
{
    return { };
}

bool PatternViewerControllerNoOp::canSetNameAndColorToTrack() noexcept
{
    return false;
}

bool PatternViewerControllerNoOp::canLinkTrack() noexcept
{
    return false;
}

Effect PatternViewerControllerNoOp::getDefaultEffect() const noexcept
{
    return Effect::volume;
}

void PatternViewerControllerNoOp::showOrHideToolbox(bool /*open*/, juce::Component& /*parent*/, int /*toolBoxButtonX*/, int /*toolBoxButtonY*/) noexcept
{
}

void PatternViewerControllerNoOp::locateToolbox(int /*toolBoxButtonX*/, int /*toolBoxButtonY*/) noexcept
{
}

void PatternViewerControllerNoOp::showWarningBecauseRecordingOff() noexcept
{
}

}   // namespace arkostracker
