#pragma once

#include "EditorWithBarsController.h"

namespace arkostracker 
{

/** Controller that does nothing. */
class EditorWithBarsControllerNoOp final : public EditorWithBarsController
{
public:
    // EditorWithBarsController method implementations.
    // ======================================================
    void onItemCellChanged(const Id& newItemId, int cellIndex, bool mustAlsoRefreshPastIndex) noexcept override;
    void onItemMetadataChanged(const Id& newItemId, unsigned int whatChanged) noexcept override;
    void onSelectedItemChangedMustRefreshUi(const Id& newSelectedItemId) noexcept override;
    OptionalId getCurrentlyShownItemId() const noexcept override;
    BarAreaSize getAreaSize(AreaType areaType) const noexcept override;
    OptionalInt getHoveredBarIndex() const noexcept override;
    BarEditorCursorLocation getBarEditorCursorLocation() const noexcept override;
    void onUserWantsToInsert() noexcept override;
    void onUserWantsToDelete() noexcept override;
    void onUserWantsToToggleRetrig() noexcept override;
    void onUserWantsToGenerateIncreasingVolume() noexcept override;
    void onUserWantsToGenerateDecreasingVolume() noexcept override;
    void onUserWantsToMoveCursorInX(bool left, Speed speed) noexcept override;
    void onUserWantsToMoveCursorInY(bool up, Speed speed) noexcept override;
    void onUserWantsToChangeValue(bool increase, Speed speed) noexcept override;
    void onUserWantsToResetValue() noexcept override;
    void moveCursorLocationTo(AreaType areaType, int barIndex) noexcept override;
    void onUserWantsToEditValue() noexcept override;
    void onUserWantsToDuplicateColumnWhereCursorIs() noexcept override;
    void onUserWantsToDuplicateValueWhereCursorIs() noexcept override;

    void onNewItemSelected(const Id& itemId, bool force) noexcept override;
    void onUserWantsToToggleLoopInHeader(OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToToggleRetrigInHeader() noexcept override;
    void onUserWantsToSetStartInHeader(int newValue) noexcept override;
    void onUserWantsToSetLoopEndInHeader(int newValue) noexcept override;
    void onUserWantsToSetSpeedInHeader(int newValue) noexcept override;
    void onUserWantsToSetShiftInHeader(int newValue) noexcept override;
    void onUserWantsToSwitchZoomXInHeader() noexcept override;
    void onUserWantsToShrinkRows(bool forceShrinkFull) noexcept override;
    bool showShift() const noexcept override;
    bool showIsLooping() const noexcept override;
    bool showIsRetrig() const noexcept override;
    bool showShrinkRows() const noexcept override;

    void onUserClickOnBar(AreaType areaType, int barIndex, int value, const juce::MouseEvent& event, bool isDrag) noexcept override;
    void onUserWantsToSlideValue(AreaType areaType, int barIndex, int value, Speed speed, bool increase) noexcept override;
    void onMouseMoveOverBar(AreaType areaType, int barIndex) noexcept override;
    void onMouseExit() noexcept override;
    void onUserWantsToEnlargeArea(AreaType areaType) noexcept override;
    void onUserWantsToShrinkArea(AreaType areaType) noexcept override;
    void onUserWantsToEnlargeArea() noexcept override;
    void onUserWantsToShrinkArea() noexcept override;
    void onUserWantsToToggleHideFromCursor(bool above) noexcept override;
    void onUserWantsToSetLoopBarStart(OptionalValue<AreaType> areaType, int newStart) noexcept override;
    void onUserWantsToSetLoopBarEnd(OptionalValue<AreaType> areaType, int newEnd) noexcept override;
    void onUserWantsToToggleLoop(OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToToggleLoopFromCursor(bool isMainLoop) noexcept override;
    void onLeftHeaderHideClicked(AreaType areaType) noexcept override;
    void onUserWantsToSetSpeed(int newSpeed) noexcept override;
    void onUserWantsToSetShift(int newShift) noexcept override;
    void onUserWantsToToggleInstrumentRetrig() noexcept override;
    void onUserWantsToSwitchXZoom() noexcept override;

    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;

    void getKeyboardFocus() noexcept override;

    juce::Component* getComponentWithTargetCommands() noexcept override;

    void onUserWantsToSetLoopStartFromCursor(bool isMainLoop) noexcept override;
    void onUserWantsToSetEndFromCursor(bool isMainLoop) noexcept override;
    void onUserWantsToIncreaseSpeed(int step) noexcept override;

    bool canHideRows() const noexcept override;
    bool canHideRow(AreaType areaType) const noexcept override;
    bool isAreaHidden(AreaType areaType) const noexcept override;

    void highlightBarIndexes(const std::vector<int>& indexes) noexcept override;
};

}   // namespace arkostracker

