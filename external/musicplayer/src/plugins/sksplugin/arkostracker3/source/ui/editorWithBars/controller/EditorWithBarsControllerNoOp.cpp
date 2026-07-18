#include "EditorWithBarsControllerNoOp.h"

namespace arkostracker 
{

void EditorWithBarsControllerNoOp::onItemCellChanged(const Id& /*newItemId*/, int /*cellIndex*/, bool /*mustAlsoRefreshPastIndex*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onItemMetadataChanged(const Id& /*newItemId*/, const unsigned int /*whatChanged*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onSelectedItemChangedMustRefreshUi(const Id& /*newSelectedItemId*/) noexcept
{
}

OptionalId EditorWithBarsControllerNoOp::getCurrentlyShownItemId() const noexcept
{
    return { };
}

BarAreaSize EditorWithBarsControllerNoOp::getAreaSize(AreaType /*areaType*/) const noexcept
{
    return BarAreaSize::normal;
}

OptionalInt EditorWithBarsControllerNoOp::getHoveredBarIndex() const noexcept
{
    return { };
}

BarEditorCursorLocation EditorWithBarsControllerNoOp::getBarEditorCursorLocation() const noexcept
{
    return { 0, AreaType::soundType };
}

void EditorWithBarsControllerNoOp::onUserWantsToInsert() noexcept
{
}

void EditorWithBarsControllerNoOp::onNewItemSelected(const Id& /*itemId*/, bool /*force*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleLoopInHeader(OptionalValue<AreaType> /*areaType*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleRetrigInHeader() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetStartInHeader(int /*newValue*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetLoopEndInHeader(int /*newValue*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetSpeedInHeader(int /*newValue*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetShiftInHeader(int /*newValue*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSwitchZoomXInHeader() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToShrinkRows(bool /*forceShrinkFull*/) noexcept
{
}

bool EditorWithBarsControllerNoOp::showShift() const noexcept
{
    return false;
}

bool EditorWithBarsControllerNoOp::showIsLooping() const noexcept
{
    return false;
}

bool EditorWithBarsControllerNoOp::showIsRetrig() const noexcept
{
    return false;
}

bool EditorWithBarsControllerNoOp::showShrinkRows() const noexcept
{
    return false;
}

void EditorWithBarsControllerNoOp::onUserClickOnBar(AreaType /*areaType*/, int /*barIndex*/, int /*value*/, const juce::MouseEvent& /*event*/, bool /*isDrag*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSlideValue(AreaType /*areaType*/, int /*barIndex*/, int /*value*/, Speed /*speed*/, bool /*p*/) noexcept
{

}

void EditorWithBarsControllerNoOp::onMouseMoveOverBar(AreaType /*p*/, int /*p*/) noexcept
{

}

void EditorWithBarsControllerNoOp::onMouseExit() noexcept
{

}

void EditorWithBarsControllerNoOp::onUserWantsToEnlargeArea(AreaType /*p*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToShrinkArea(AreaType /*p*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToEnlargeArea() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToShrinkArea() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleHideFromCursor(bool /*above*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetLoopBarStart(OptionalValue<AreaType> /*p*/, int /*p*/) noexcept
{

}

void EditorWithBarsControllerNoOp::onUserWantsToSetLoopBarEnd(OptionalValue<AreaType> /*p*/, int /*p*/) noexcept
{

}

void EditorWithBarsControllerNoOp::onUserWantsToToggleLoop(OptionalValue<AreaType> /*p*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleLoopFromCursor(bool /*isMainLoop*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onLeftHeaderHideClicked(AreaType /*areaType*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetSpeed(int /*p*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetShift(int /*p*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleInstrumentRetrig() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSwitchXZoom() noexcept
{
}

void EditorWithBarsControllerNoOp::onParentViewCreated(BoundedComponent& /*parent*/)
{
}

void EditorWithBarsControllerNoOp::onParentViewResized(int /*p*/, int /*p*/, int /*p*/, int /*p*/)
{
}

void EditorWithBarsControllerNoOp::onParentViewFirstResize(juce::Component& /*parent*/)
{
}

void EditorWithBarsControllerNoOp::onParentViewDeleted() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToDelete() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToToggleRetrig() noexcept
{
}

void EditorWithBarsControllerNoOp::getKeyboardFocus() noexcept
{
}

juce::Component* EditorWithBarsControllerNoOp::getComponentWithTargetCommands() noexcept
{
    return nullptr;
}

void EditorWithBarsControllerNoOp::moveCursorLocationTo(AreaType /*areaType*/, int /*barIndex*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToGenerateIncreasingVolume() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToGenerateDecreasingVolume() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToMoveCursorInX(bool /*left*/, Speed /*speed*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToMoveCursorInY(bool /*up*/, Speed /*speed*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToDuplicateColumnWhereCursorIs() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToDuplicateValueWhereCursorIs() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToChangeValue(bool /*increase*/, Speed /*speed*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToResetValue() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToEditValue() noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetLoopStartFromCursor(bool /*isMainLoop*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToSetEndFromCursor(bool /*isMainLoop*/) noexcept
{
}

void EditorWithBarsControllerNoOp::onUserWantsToIncreaseSpeed(int /*step*/) noexcept
{
}

bool EditorWithBarsControllerNoOp::canHideRows() const noexcept
{
    return false;
}

bool EditorWithBarsControllerNoOp::canHideRow(AreaType /*areaType*/) const noexcept
{
    return false;
}

bool EditorWithBarsControllerNoOp::isAreaHidden(AreaType) const noexcept
{
    return true;
}

void EditorWithBarsControllerNoOp::highlightBarIndexes(const std::vector<int>& /*indexes*/) noexcept
{
}

}   // namespace arkostracker
