#pragma once

#include "../../../controllers/SongControllerNoOp.h"
#include "PatternViewerController.h"
#include "toolbox/ToolboxControllerNoOp.h"

namespace arkostracker
{

enum class TransposeRate : unsigned char;

/** A controller implementation that does nothing. */
class PatternViewerControllerNoOp final : public PatternViewerController
{
public:
    SongController& getSongController() override;

    void onParentViewCreated(BoundedComponent& parentView) override;

    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;

    void onParentViewFirstResize(juce::Component& parentView) override;

    void onParentViewDeleted() noexcept override;

    void onUserLeftClickedOnChannelNumber(int channelIndex) override;

    void onUserRightClickedOnChannelNumber(int channelIndex) override;

    void onUserWantsToScrollVerticallyWithMouseWheel(bool scrollToBottom) override;

    void onUserWantsToMoveVertically(bool goDown, bool fast, bool enlargeSelection) override;

    void onUserWantsToMoveVerticallyTo(int cellIndex, bool enlargeSelection) override;

    void onUserWantsToMoveHorizontally(bool goToRight, bool enlargeSelection) override;

    void onUserWantsToMoveToNextChannel(bool next) override;

    void onUserWantsToMoveToNextPosition(bool next) override;

    void onUserClickedOnRankOfNormalTrack(int channelIndex, int rankFromCenterAsOrigin, CellCursorRank cursorRank, bool leftButton, bool shift) override;

    void onUserClickIsUpFromNormalTrack(bool leftButton) override;

    void onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event) override;

    void onUserClickedOnRankOfSpecialTrack(bool isSpeedTrack, int rankFromCenterAsOrigin, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) override;

    void onUserClickIsUpFromSpecialTrack(bool leftButton) override;

    void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) override;

    void onUserClickedOnRankOfLineTrack(int rankFromCenterAsOrigin, bool leftButton) override;

    void onUserClickIsUpFromLineTrack(bool leftButton) override;

    void onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event) override;

    void onUserWantsToToggleMinimize() override;

    void onUserWantsToToggleRecord() override;

    void onUserWantsToToggleFollow() override;

    void onUserWantsToToggleWriteInstrument() override;

    void onUserWantsToToggleOverwriteDefaultEffect() override;

    void onUserWantsToDelete(bool wholeCell) override;

    void onUserWantsToModifyTrackHeight(int newHeight) override;

    void onUserWantsToModifyTrackHeightWithOffset(int offset) override;

    void onUserWantsToModifyTransposition(int channelIndex, int newTransposition) override;

    void onUserWantsToSetTrackNameAndColor(int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) override;

    void onUserWantsToRenameSpecialTrack(bool isSpeedTrack, const juce::String& newName) override;

    void onKeyboardNote(int baseNote) override;
    void onNote(int note) override;

    bool onKeyPressed(const juce::KeyPress& key) override;

    void onUserWantsToPlayLine() override;

    void onUserWantsToTogglePlayPatternFromCursorOrBlock(bool forceSetBlock) override;

    void onUserWantsToTogglePlayPatternFromStart() override;

    void onUserWantsToCopy() override;

    void onUserWantsToCut() override;

    void onUserWantsToPaste(bool moveBelowAfter, bool pasteContinuously) override;

    void onUserWantsToPasteMix() override;

    void onUserWantsToClearSelection() override;

    void onUserWantsToTransposeSelection(TransposeRate transposeRate) override;

    void onUserWantsToToggleReadOnly() override;

    void onUserWantsToGenerateArpeggio() override;
    void onUserWantsToGenerateInstrument() override;

    void onUserWantsToInsert() override;

    void onUserWantsToRemove() override;

    void onUserWantsToSetEditStep(int stepToValidate) override;

    void onUserWantsToIncreaseEditStep(int offset) override;

    void onUserSelectedLockedEffect(Effect effect) override;

    void onUserWantsToToggleSoloTrack() override;

    void onUserWantsToToggleMuteTrack() override;

    void onUserWantsToSelectTrack() override;

    void onUserWantsToSelectAllTracks() override;

    void onUserWantsToCapture() override;

    void onUserWantsToWriteRst() override;

    bool isRecording() override;

    void onUserWantsToCreateLink(int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) override;
    void onUserWantsToUnlink(int positionIndexToUnlink, int channelIndexToUnlink) noexcept override;
    void onUserWantsToCreateSpecialLink(bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) override;
    void onUserWantsToUnlinkSpecial(bool isSpeedTrack, int positionIndexToUnlink) noexcept override;

    void onUserWantsToGoTo(int positionIndexToGoTo, int channelIndexToGoTo) noexcept override;
    void onUserWantsToGoToSpecialTrack(bool isSpeedTrack, int positionIndexToGoTo) noexcept override;
    void onUserWantsToGoToChannel(int channelIndexToGoTo) noexcept override;

    void onUserWantsToSetTrackNameAndColor() override;
    void onUserWantsToLinkTrack() override;

    void getKeyboardFocus() noexcept override;

    bool isReadOnlyTrack(int channelIndex) noexcept override;

    bool canGenerateArpeggioFromSelection() noexcept override;
    bool canGenerateInstrumentFromSelection() noexcept override;

    bool canSetNameAndColorToTrack() noexcept override;
    bool canLinkTrack() noexcept override;

    int getTrackIndex(int channelIndex) noexcept override;

    int getPositionIndex() noexcept override;

    Id getSubsongId() noexcept override;

    Effect getDefaultEffect() const noexcept override;
    const std::vector<Effect>& getDefaultEffects() const noexcept override;
    void onUserWantsToChangeDefaultEffect(bool next) override;

    void showOrHideToolbox(bool open, juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void locateToolbox(int toolBoxButtonX, int toolBoxButtonY) noexcept override;

    void showWarningBecauseRecordingOff() noexcept override;

private:
    SongControllerNoOp songController;
    ToolboxControllerNoOp toolboxController;
};

}   // namespace arkostracker
