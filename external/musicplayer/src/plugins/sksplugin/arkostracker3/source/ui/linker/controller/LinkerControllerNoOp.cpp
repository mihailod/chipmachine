#include "LinkerControllerNoOp.h"

namespace arkostracker 
{

LinkerControllerNoOp::LinkerControllerNoOp(SongController& pSongController) :
        LinkerController(pSongController)
{
}

void LinkerControllerNoOp::onParentViewResized(int /*startX*/, int /*startY*/, int /*newWidth*/, int /*newHeight*/) noexcept
{
}

void LinkerControllerNoOp::onWantToSetLoopStartOrEnd(OptionalInt /*newLoopStartPosition*/, OptionalInt /*newEndPosition*/) noexcept
{
}

void LinkerControllerNoOp::onPatternItemClicked(int /*position*/, bool /*down*/, bool /*shift*/, bool /*control*/) noexcept
{
}

void LinkerControllerNoOp::onPatternItemDoubleClicked(int /*position*/) noexcept
{
}

void LinkerControllerNoOp::onWantToOpenPatternViewer() noexcept
{
}

void LinkerControllerNoOp::onWantToEditPosition(int /*position*/)
{
}

void LinkerControllerNoOp::onWantToEditPositions(const std::set<int>& /*positionIndexes*/)
{
}

void LinkerControllerNoOp::onPatternItemRightClicked(int /*position*/) noexcept
{
}

void LinkerControllerNoOp::onAddNewPositionClicked(int /*position*/) noexcept
{
}

void LinkerControllerNoOp::onClonePositionClicked(int /*position*/) noexcept
{
}

void LinkerControllerNoOp::onRepeatPositionClicked(int /*position*/) noexcept
{
}

void LinkerControllerNoOp::onWantToMoveOrDuplicateSelectedPositions(int /*originalPosition*/, int /*sourcePositionIndex*/, bool /*duplicate*/)
{
}

void LinkerControllerNoOp::onWantToIncDecPatternItemValue(int /*position*/, int /*offset*/)
{
}

/*bool LinkerControllerNoOp::isPatternItemSelected(int *//*position*//*) const noexcept
{
    return false;
}*/

void LinkerControllerNoOp::duplicateSelectedPositions() noexcept
{
}

void LinkerControllerNoOp::cloneSelectedPositions() noexcept
{
}

void LinkerControllerNoOp::deleteSelectedPositions() noexcept
{
}

void LinkerControllerNoOp::createNew() noexcept
{
}

void LinkerControllerNoOp::editSelectedPosition() noexcept
{
}

void LinkerControllerNoOp::editPositions(const std::set<int>& /*positionIndexes*/) noexcept
{
}

void LinkerControllerNoOp::onWantToMoveSelection(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::onWantToMoveSelectionFast(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::onWantToMoveSelectionToLimit(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::onWantToGrowSelection(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::onWantToGrowSelectionFast(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::onWantToGrowSelectionToLimit(bool /*forward*/) noexcept
{
}

void LinkerControllerNoOp::copySelectionToClipboard() noexcept
{
}

void LinkerControllerNoOp::cutSelectionToClipboard() noexcept
{
}

void LinkerControllerNoOp::pasteClipboard() noexcept
{
}

void LinkerControllerNoOp::onParentViewCreated(BoundedComponent& /*parentView*/)
{
}

void LinkerControllerNoOp::onParentViewFirstResize(juce::Component& /*parentView*/)
{
}

void LinkerControllerNoOp::onParentViewDeleted() noexcept
{
}

void LinkerControllerNoOp::markSelectionAsLoopStartAndEnd() noexcept
{
}

void LinkerControllerNoOp::markLoopStart() noexcept
{
}

void LinkerControllerNoOp::markEnd() noexcept
{
}

bool LinkerControllerNoOp::canCutSelection() const noexcept
{
    return false;
}

bool LinkerControllerNoOp::canDeleteSelection() const noexcept
{
    return false;
}

void LinkerControllerNoOp::getKeyboardFocus() noexcept
{
}

bool LinkerControllerNoOp::isOneItemSelected(bool /*onlyOneAuthorized*/) const noexcept
{
    return false;
}

void LinkerControllerNoOp::onWantToIncreasePatternIndex(int /*step*/) noexcept
{
}

}   // namespace arkostracker

