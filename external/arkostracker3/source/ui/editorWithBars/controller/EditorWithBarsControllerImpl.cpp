#include "EditorWithBarsControllerImpl.h"

#include "../../../business/song/validation/CheckLoopStartEnd.h"
#include "../../../controllers/MainController.h"
#include "../../../utils/CollectionUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../arpeggioTableEditor/controller/ArpeggioTableEditorControllerSpecificImpl.h"
#include "../view/EditorWithBarsViewNoOp.h"

namespace arkostracker 
{

const int EditorWithBarsControllerImpl::smallXZoomRate = 0;
const int EditorWithBarsControllerImpl::normalXZoomRate = 3;

const int EditorWithBarsControllerImpl::aheadColumnCount = 3;

EditorWithBarsControllerImpl::EditorWithBarsControllerImpl(MainController& pMainController, EditorWithBarsControllerSpecific::Factory& pFactory,
                                                           const AreaType pDefaultCursorAreaType) noexcept :
        parentView(),
        specificController(pFactory.buildSpecificController(pMainController, *this)),
        shownItemIdOptional(specificController->getCurrentItemId()),
        xZoom(ZoomX::normal),
        editorView(std::make_unique<EditorWithBarsViewNoOp>()),
        areaTypeToLeftHeaderDisplayData(),
        areaTypeToAutoSpreadData(),
        currentlyHoveredBarIndex(),
        hoveredBarIndexWhereContextualMenuOpened(),
        isContextualMenuOpened(false),
        currentBarEditorCursorLocation(0, pDefaultCursorAreaType),
        visibilityHandler(*specificController)
{
    // Asks the specific controller to give the sizes each area has and can have.
    const auto areaTypes = specificController->getAreaTypes();

    // Fills the left header with raw data, to beginning with.
    for (const auto areaType : areaTypes) {
        areaTypeToLeftHeaderDisplayData.insert(std::make_pair(areaType, LeftHeaderDisplayData(false, false, OptionalBool(), false)));
        areaTypeToAutoSpreadData.insert(std::make_pair(areaType, Loop(0, 0, false)));
    }
    // The tables must have all their entries.
    jassert(areaTypeToLeftHeaderDisplayData.size() == areaTypes.size());
    jassert(areaTypeToAutoSpreadData.size() == areaTypes.size());

    // Now we construct the left header right data.
    updateLeftHeadersAndAutoSpreadData();
}


// ParentViewLifeCycleAware method implementations.
// ===================================================

void EditorWithBarsControllerImpl::onParentViewCreated(BoundedComponent& newParentView)
{
    parentView = &newParentView;
    instantiateViewAndRefreshUi();
}

void EditorWithBarsControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    editorView->setBounds(startX, startY, newWidth, newHeight);
}

void EditorWithBarsControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
    // Nothing to do.
}

void EditorWithBarsControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to a no-op View.
    editorView = std::make_unique<EditorWithBarsViewNoOp>();
    parentView = nullptr;
}

void EditorWithBarsControllerImpl::onNewItemSelected(const Id& newItemId, const bool force) noexcept
{
    // May not be called according to the implementation (expressions register their own listener, so never reach this).

    if (!force && (OptionalId(newItemId) == shownItemIdOptional)) {
        return;
    }

    onSelectedItemChangedMustRefreshUi(newItemId);
}

void EditorWithBarsControllerImpl::getKeyboardFocus() noexcept
{
    editorView->grabKeyboardFocus();
}


// HeaderController method implementations.
// ===========================================

void EditorWithBarsControllerImpl::onUserWantsToSetStartInHeader(const int newValue) noexcept
{
    onUserWantsToSetLoopBarStart({ }, newValue);
}

void EditorWithBarsControllerImpl::onUserWantsToSetLoopEndInHeader(const int newValue) noexcept
{
    onUserWantsToSetLoopBarEnd({ }, newValue);
}

void EditorWithBarsControllerImpl::onUserWantsToSetSpeedInHeader(const int newValue) noexcept
{
    onUserWantsToSetSpeed(newValue);
}

void EditorWithBarsControllerImpl::onUserWantsToSetShiftInHeader(const int newValue) noexcept
{
    onUserWantsToSetShift(newValue);
}

void EditorWithBarsControllerImpl::onUserWantsToSwitchZoomXInHeader() noexcept
{
    onUserWantsToSwitchXZoom();
}

void EditorWithBarsControllerImpl::onUserWantsToToggleLoopInHeader(const OptionalValue<AreaType> areaType) noexcept
{
    onUserWantsToToggleLoop(areaType);
}

void EditorWithBarsControllerImpl::onUserWantsToToggleRetrigInHeader() noexcept
{
    onUserWantsToToggleInstrumentRetrig();
}

void EditorWithBarsControllerImpl::onUserWantsToShrinkRows(const bool forceShrinkFull) noexcept
{
    visibilityHandler.hideRows(getCurrentlyShownItemId(), false, forceShrinkFull);
    refreshCompleteUi(true, false);     // Don't shrink, we already did (and maybe with a "force shrink").
}

bool EditorWithBarsControllerImpl::showShrinkRows() const noexcept
{
    return canHideRows();
}


// BarAreaController method implementations.
// ============================================

void EditorWithBarsControllerImpl::onUserClickOnBar(const AreaType areaType, const int barIndex, const int value, const juce::MouseEvent& event, const bool isDrag) noexcept
{
    const auto& mods = event.mods;

    // Right-click down?
    if (mods.isRightButtonDown() && !isDrag) {
        const auto& itemId = getCurrentlyShownItemId();
        jassert(itemId.isPresent());        // Else, abnormal!
        // Hack: Stores the hovered item, else it disappears when contextual menu opens, and the action won't have
        // the item anymore when going back. This index will be dismissed when the menu closes.
        hoveredBarIndexWhereContextualMenuOpened = barIndex;
        isContextualMenuOpened = true;
        specificController->openContextualMenu(itemId.getValueRef(), areaType, barIndex, [this] { onContextualMenuItemClicked(); });
        return;
    }

    // The left should be clicked. Security.
    if (!mods.isLeftButtonDown()) {
        return;
    }

    // Double click?
    if (event.getNumberOfClicks() == 2) {
        const auto& itemId = getCurrentlyShownItemId();
        jassert(itemId.isPresent());        // Else, abnormal!
        specificController->onUserWantsToEditValue(itemId.getValueRef(), areaType, barIndex);
        return;
    }

    if (!mods.isAnyModifierKeyDown() && !isDrag) {
        // Simple click = moves cursor.
        moveCursorLocationTo(areaType, barIndex);
        return;
    }

    // Left button, maybe drag.

    // Ctrl = write.
    if (mods.isCtrlDown()) {
        // Changes the data, if needed. It will notify for a change, updating the UI.
        checkValueAndChangeItem(areaType, barIndex, value);
    }
}

void EditorWithBarsControllerImpl::onMouseMoveOverBar(AreaType /*areaType*/, const int barIndex) noexcept
{
    // Ignores the mouse exit if the context menu is present.
    if (isContextualMenuOpened) {
        return;
    }

    // New hovered bar?
    if (currentlyHoveredBarIndex == barIndex) {
        return;
    }

    currentlyHoveredBarIndex = barIndex;

    refreshUiColumn(barIndex);
    removeContextualMenuHoveredBarIfPresent(barIndex);
}

void EditorWithBarsControllerImpl::onMouseExit() noexcept
{
    // Ignores the mouse exit if the context menu is present.
    if (isContextualMenuOpened) {
        return;
    }

    const auto localHoveredIndex = currentlyHoveredBarIndex;

    // Un-hover the hovered bar, if present.
    currentlyHoveredBarIndex = { };

    if (localHoveredIndex.isPresent()) {
        refreshUiColumn(localHoveredIndex.getValue());

        removeContextualMenuHoveredBarIfPresent(localHoveredIndex.getValue());
    }
}

void EditorWithBarsControllerImpl::removeContextualMenuHoveredBarIfPresent(const int hoveredBarIndex) noexcept
{
    if (hoveredBarIndexWhereContextualMenuOpened.isPresent() && (hoveredBarIndexWhereContextualMenuOpened.getValue() != hoveredBarIndex)) {
        refreshUiColumn(hoveredBarIndexWhereContextualMenuOpened.getValue());
        hoveredBarIndexWhereContextualMenuOpened = { };
    }
}

void EditorWithBarsControllerImpl::onContextualMenuItemClicked() noexcept
{
    isContextualMenuOpened = false;
}

void EditorWithBarsControllerImpl::onUserWantsToSlideValue(const AreaType areaType, const int barIndex, int value, const Speed modifierSpeed, const bool increase) noexcept
{
    unsigned int modifierSpeedInt;           // NOLINT(*-init-variables)
    constexpr auto maxSpeed = 2U;
    switch (modifierSpeed) {
        case Speed::normal:
            modifierSpeedInt = 0U;
            break;
        case Speed::fast:
            modifierSpeedInt = 1U;
            break;
        case Speed::fastest: [[fallthrough]];
        case Speed::limit: [[fallthrough]];
        default:
            modifierSpeedInt = maxSpeed;
            break;
    }
    jassert(modifierSpeedInt <= maxSpeed);

    // Non-default speed values.
    const auto& areaTypeToSpeeds = specificController->getSpecificAreaTypeToSlideSpeed();
    static const std::vector defaultAreaTypeToSpeeds = {
            { 1, 2, 4 }
    };
    jassert(defaultAreaTypeToSpeeds.size() == (maxSpeed + 1U));           // Missing size?

    // What speed? If not found in the map, uses a default value.
    const auto* speeds = &defaultAreaTypeToSpeeds;
    const auto iterator = areaTypeToSpeeds.find(areaType);
    if (iterator != areaTypeToSpeeds.cend()) {
        speeds = &iterator->second;
    }
    jassert(speeds->size() == (maxSpeed + 1U));                           // Missing size?

    auto speed = speeds->at(modifierSpeedInt);
    if (!increase) {
        speed = -speed;
    }

    value += speed;

    // Changes the item, if needed. It will notify for a change, updating the UI.
    checkValueAndChangeItem(areaType, barIndex, value);
}

void EditorWithBarsControllerImpl::onUserWantsToEnlargeArea(const AreaType areaType) noexcept
{
    // Allowed?
    if (!isEnlargeAreaAllowed(areaType)) {
        return;
    }

    changeAreaSizeAndUpdate(areaType, true);
}

void EditorWithBarsControllerImpl::onUserWantsToEnlargeArea() noexcept
{
    const auto currentArea = getBarEditorCursorLocation().getAreaType();
    onUserWantsToEnlargeArea(currentArea);
}

void EditorWithBarsControllerImpl::onUserWantsToShrinkArea() noexcept
{
    const auto currentArea = getBarEditorCursorLocation().getAreaType();
    onUserWantsToShrinkArea(currentArea);
}

void EditorWithBarsControllerImpl::onUserWantsToShrinkArea(const AreaType areaType) noexcept
{
    // Allowed?
    if (!isShrinkAreaAllowed(areaType)) {
        return;
    }

    changeAreaSizeAndUpdate(areaType, false);
}

void EditorWithBarsControllerImpl::onUserWantsToToggleHideFromCursor(const bool above) noexcept
{
    // Gets the area to target.
    const auto currentArea = getBarEditorCursorLocation().getAreaType();
    const auto newAreaType = editorView->getShiftedAreaType(currentArea, above ? -1 : 1);
    if (currentArea != newAreaType) {
        toggleHiddenArea(newAreaType);
    }
}

void EditorWithBarsControllerImpl::onUserWantsToSetLoopBarStart(const OptionalValue<AreaType> areaType, const int newStart) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    if (areaType.isPresent()) {
        // Concerns the auto-spread.
        specificController->onUserWantsToSetAutoSpreadStartAndEnd(shownItemId, areaType.getValue(), newStart, OptionalInt());
        return;
    }

    onUserWantsToSetLoopStartAndEnd(newStart, OptionalInt());
}

void EditorWithBarsControllerImpl::onUserWantsToSetLoopBarEnd(const OptionalValue<AreaType> areaType, const int newEnd) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    if (areaType.isPresent()) {
        // Concerns the auto-spread.
        specificController->onUserWantsToSetAutoSpreadStartAndEnd(shownItemId, areaType.getValue(), OptionalInt(), newEnd);
        return;
    }

    onUserWantsToSetLoopStartAndEnd(OptionalInt(), newEnd);
}

void EditorWithBarsControllerImpl::onUserWantsToToggleLoop(const OptionalValue<AreaType> areaType) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    specificController->onUserWantsToToggleLoop(shownItemId, areaType);
}

void EditorWithBarsControllerImpl::onUserWantsToToggleLoopFromCursor(const bool isMainLoop) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    specificController->onUserWantsToToggleLoop(shownItemId, isMainLoop ? OptionalValue<AreaType>() : currentBarEditorCursorLocation.getAreaType());
}

void EditorWithBarsControllerImpl::onUserWantsToSetLoopStartFromCursor(const bool isMainLoop) noexcept
{
    const auto newStart = currentBarEditorCursorLocation.getX();
    onUserWantsToSetLoopBarStart(isMainLoop ? OptionalValue<AreaType>() : currentBarEditorCursorLocation.getAreaType(), newStart);
}

void EditorWithBarsControllerImpl::onUserWantsToSetEndFromCursor(const bool isMainLoop) noexcept
{
    const auto newEnd = currentBarEditorCursorLocation.getX();
    onUserWantsToSetLoopBarEnd(isMainLoop ? OptionalValue<AreaType>() : currentBarEditorCursorLocation.getAreaType(), newEnd);
}

void EditorWithBarsControllerImpl::onUserWantsToIncreaseSpeed(const int step) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    specificController->onUserWantsToIncreaseSpeed(shownItemId, step);
}

void EditorWithBarsControllerImpl::onLeftHeaderHideClicked(const AreaType areaType) noexcept
{
    toggleHiddenArea(areaType);
}

void EditorWithBarsControllerImpl::toggleHiddenArea(const AreaType areaType) noexcept
{
    // Toggles.
    const auto toggled = toggleStoredHiddenAreaType(areaType);
    if (!toggled) {
        jassertfalse;
        return;
    }

    updateLeftHeaderAndAutoSpreadData(areaType);
    refreshUiAreasMetadataFromInternalData();               // This refreshes ALL the areas. Diffs will be made, so it is ok.
    // Makes sure that the cursor is not on a hidden area type.
    moveCursorToNonHiddenAreaType();
}

void EditorWithBarsControllerImpl::onUserWantsToSetSpeed(const int originalNewSpeed) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    const auto currentSpeed = specificController->getItemSpeed(shownItemId);
    const auto newSpeed = specificController->correctItemSpeed(originalNewSpeed);

    // Any change?
    if (currentSpeed == newSpeed) {
        return;
    }

    specificController->setItemMetadata(shownItemId, OptionalInt(), OptionalInt(), newSpeed, OptionalInt(),
                                        OptionalBool(), OptionalBool(), { });
}

void EditorWithBarsControllerImpl::onUserWantsToSetShift(const int originalNewShift) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    const auto currentShift = specificController->getItemShift(shownItemId);
    const auto newShift = specificController->correctItemShift(originalNewShift);

    // Any change?
    if (currentShift == newShift) {
        return;
    }

    specificController->setItemMetadata(shownItemId, OptionalInt(), OptionalInt(), OptionalInt(), newShift,
                                        OptionalBool(), OptionalBool(), { });
}

void EditorWithBarsControllerImpl::onUserWantsToSwitchXZoom() noexcept
{
    ZoomX newXZoom;         // NOLINT(*-init-variables)
    switch (xZoom) {
        default:
            jassertfalse;       // Shouldn't happen!
        case ZoomX::normal:
            newXZoom = ZoomX::small;
            break;
        case ZoomX::small:
            newXZoom = ZoomX::normal;
            break;
    }

    // Any change?
    if (xZoom == newXZoom) {
        return;
    }
    xZoom = newXZoom;

    // Updates the UI.
    const auto newZoomValue = getXZoomValueFromZoomX(xZoom);
    editorView->setXZoomRate(newZoomValue);
    updateTopHeaderView();          // Updates the header, it shows the X zoom.
}

void EditorWithBarsControllerImpl::onUserWantsToToggleInstrumentRetrig() noexcept
{
    if (showIsRetrig()) {
        if (shownItemIdOptional.isAbsent()) {
            jassertfalse;       // Nothing shown, shouldn't be called.
            return;
        }
        const auto& shownItemId = shownItemIdOptional.getValueRef();

        specificController->onUserWantsToToggleInstrumentRetrig(shownItemId);
    } else {
        jassertfalse; // Not supposed to be called!
    }
}

int EditorWithBarsControllerImpl::getXZoomValueFromZoomX(const ZoomX zoomX) noexcept
{
    switch (zoomX) {
        case ZoomX::small:
            return smallXZoomRate;
        default:
            jassertfalse;
        case ZoomX::normal:
            return normalXZoomRate;
    }
}

bool EditorWithBarsControllerImpl::isAreaHidden(const AreaType areaType) const noexcept
{
    return visibilityHandler.isAreaHidden(shownItemIdOptional, areaType);
}

// ============================================

void EditorWithBarsControllerImpl::refreshCompleteUi(const bool pEnsureCursorVisible, const bool shrinkRows) noexcept
{
    // On arriving at the item, tries to shrink the rows, but only if it has never been done before.
    if (shrinkRows) {
        visibilityHandler.hideRows(shownItemIdOptional, true);
    }

    updateTopHeaderView();
    updateLeftHeadersAndAutoSpreadUi();     // Must be done before the Bars, in order to set the sizes of the bars.
    correctCursorXInternally(false);        // On new instrument, looks better if the cursor remains inside the "real" sound.
    updateBars();

    if (pEnsureCursorVisible) {
        ensureCursorVisible();
    }
}

void EditorWithBarsControllerImpl::updateBars(const int updateFromIndex) const noexcept
{
    // First, creates the bar we need (or removing some), with default data.
    const auto barCount = getDisplayedCellIndexCount();
    editorView->setDisplayedBarCount(barCount);

    // Updates the cells.
    for (auto cellIndex = updateFromIndex; cellIndex < barCount; ++cellIndex) {
        refreshUiColumn(cellIndex);
    }
}

void EditorWithBarsControllerImpl::refreshUiColumn(const int cellIndex) const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    // Gets the data but all the Bars for this index.
    const auto areaTypeToData = specificController->createBarsData(shownItemId, cellIndex);

    // Refreshes the UI for each bar of the index.
    for (const auto& [areaType, data] : areaTypeToData) {
        editorView->showBarData(areaType, cellIndex, data.getBarData(), data.getCaptionData());
    }
}

void EditorWithBarsControllerImpl::updateTopHeaderView() const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    const auto displayedTopHeaderData = specificController->getDisplayedTopHeaderData(shownItemId);

    editorView->setDisplayTopHeaderData(displayedTopHeaderData);
}

int EditorWithBarsControllerImpl::getDisplayedCellIndexCount(const bool withAheadColumns) const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        return 0;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    // Gets the length, but adds more column, which is handy to the user.
    return specificController->getItemLength(shownItemId) + (withAheadColumns ? aheadColumnCount : 0);
}

void EditorWithBarsControllerImpl::checkValueAndChangeItem(const AreaType areaType, const int barIndex, const int value) const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    specificController->checkValueAndChangeItem(shownItemId, areaType, barIndex, value);
}

bool EditorWithBarsControllerImpl::isShrinkAreaAllowed(const AreaType areaType) const noexcept
{
    // Minus Button always available, unless already at 0.
    const auto currentSize = getAreaSize(areaType);
    return (currentSize > BarAreaSize::first);
}

bool EditorWithBarsControllerImpl::isEnlargeAreaAllowed(const AreaType areaType) const noexcept
{
    return visibilityHandler.isEnlargeAreaAllowed(shownItemIdOptional, areaType);
}

void EditorWithBarsControllerImpl::changeAreaSizeAndUpdate(const AreaType areaType, const bool enlarge) noexcept
{
    // What is the current size?
    auto currentSize = getAreaSize(areaType);
    const auto direction = enlarge ? 1 : -1;
    // New size. The possibility of doing so must have been checked before.
    currentSize = static_cast<BarAreaSize>(static_cast<int>(currentSize) + direction);

    // Modifies the internal data.
    visibilityHandler.setAreaSize(shownItemIdOptional, areaType, currentSize);

    updateLeftHeaderAndAutoSpreadData(areaType);

    refreshUiAreasMetadataFromInternalData();               // This refreshes ALL the areas. Diffs will be made, so it is ok.

    // The area bars must also be refreshed, because Bars can be out of bounds.
    refreshAreaBars(areaType);
}

void EditorWithBarsControllerImpl::updateLeftHeadersAndAutoSpreadUi() noexcept
{
    updateLeftHeadersAndAutoSpreadData();
    refreshUiAreasMetadataFromInternalData();
}

void EditorWithBarsControllerImpl::updateLeftHeadersAndAutoSpreadData() noexcept
{
    for (const auto& [areaType, _] : areaTypeToLeftHeaderDisplayData) {
        updateLeftHeaderAndAutoSpreadData(areaType);
    }
}

void EditorWithBarsControllerImpl::updateLeftHeaderAndAutoSpreadData(const AreaType areaType) noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    const auto isAutoSpreadPossible = specificController->isAutoSpreadAllowed();

    OptionalBool autoSpreadOn;
    Loop loop;
    if (isAutoSpreadPossible) {
        loop = specificController->getAutoSpreadLoop(shownItemId, areaType);
        autoSpreadOn = loop.isLooping();
    }

    const auto minusEnabled = isShrinkAreaAllowed(areaType);
    const auto plusEnabled = isEnlargeAreaAllowed(areaType);

    // We can update the data.
    const auto newData = LeftHeaderDisplayData(plusEnabled, minusEnabled, autoSpreadOn, isAreaHidden(areaType));
    areaTypeToLeftHeaderDisplayData[areaType] = newData;
    areaTypeToAutoSpreadData[areaType] = loop;
}

bool EditorWithBarsControllerImpl::toggleStoredHiddenAreaType(const AreaType areaType) noexcept
{
    return visibilityHandler.toggleHiddenAreaType(shownItemIdOptional, areaType);
}

void EditorWithBarsControllerImpl::refreshUiAreasMetadataFromInternalData() const noexcept
{
    editorView->setMetadataDisplayData(visibilityHandler.getCurrentSizes(shownItemIdOptional),
        areaTypeToLeftHeaderDisplayData, areaTypeToAutoSpreadData);
}

void EditorWithBarsControllerImpl::refreshAreaBars(const AreaType areaType) const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    for (auto cellIndex = 0, cellCount = getDisplayedCellIndexCount(); cellIndex < cellCount; ++cellIndex) {
        // This creates the data of the whole column, but only a subset is kept. Not slower, because the whole Cell must be read anyway.
        const auto areaTypeToData = specificController->createBarsData(shownItemId, cellIndex);          // This COULD be optimized because only one Area is used.

        const auto& data = areaTypeToData.find(areaType)->second;
        editorView->showBarData(areaType, cellIndex, data.getBarData(), data.getCaptionData());
    }
}

void EditorWithBarsControllerImpl::onUserWantsToSetLoopStartAndEnd(const OptionalInt originalNewStart, const OptionalInt originalNewEnd) const noexcept
{
    if (shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Nothing shown, shouldn't be called.
        return;
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();

    if (originalNewStart.isAbsent() && originalNewEnd.isAbsent()) {
        jassertfalse;           // No change? Abnormal.
        return;
    }

    auto newStart = originalNewStart;
    auto newEnd = originalNewEnd;

    // Gets the length and the missing start or end, to make sure they are consistent.
    const auto [length, originalLoop] = specificController->getItemLengthAndLoop(shownItemId);

    if (newStart.isAbsent()) {
        newStart = originalLoop.getStartIndex();
    } else if (newEnd.isAbsent()) {
        newEnd = originalLoop.getEndIndex();
    }

    // Corrects the values. We allow the start to be higher than the end, more user friendly.
    auto newStartAndEnd = CheckLoopStartEnd::checkLoopStartEnd(newStart.getValue(), newEnd.getValue(), length, originalNewStart, originalNewEnd);
    newStart = newStartAndEnd.first;
    newEnd = newStartAndEnd.second;

    // No need to do anything if the loop is the same.
    if ((newStart == originalLoop.getStartIndex()) && (newEnd == originalLoop.getEndIndex())) {
        return;
    }

    specificController->setItemMetadata(shownItemId, newStart, newEnd, OptionalInt(), OptionalInt(),
                                        OptionalBool(), OptionalBool(), { });
}


// ArpeggioTableEditorController method implementations.
// ========================================================

void EditorWithBarsControllerImpl::onItemCellChanged(const Id& newItemId, const int cellIndex, const bool mustAlsoRefreshPastIndex) noexcept
{
    // We only care about the shown item.
    if (OptionalId(newItemId) != shownItemIdOptional) {
        return;
    }

    // Only one column to refresh, or >= cellIndex?
    if (mustAlsoRefreshPastIndex) {
        // It is possible that cells are added (or removed!). Before updating, update their number.
        updateBars(cellIndex);
    } else {
        refreshUiColumn(cellIndex);
    }
}

void EditorWithBarsControllerImpl::onItemMetadataChanged(const Id& newItemId, unsigned int whatChanged) noexcept
{
    // Is the change on the shown item? Else, don't do anything.
    if (OptionalId(newItemId) != shownItemIdOptional) {
        return;
    }

    // Any change that interests us?
    if (!specificController->isItemMetadataChangeFlagsInteresting(whatChanged)) {
        return;
    }

    // The whole UI should be updated, because a loop change changes the out-of-bounds.
    refreshCompleteUi(false);
}

void EditorWithBarsControllerImpl::onSelectedItemChangedMustRefreshUi(const Id& newSelectedItemId) noexcept
{
    const auto initialShownItemIdOptional = shownItemIdOptional;    // Keep the previous value for the method below.
    shownItemIdOptional = newSelectedItemId;

    // The view may have been destroyed, yet the controller is still notified.
    if (parentView == nullptr) {
        return;
    }

    // Should the View be recreated? For example, when moving for "none selected" to something else.
    const auto mustRecreate = specificController->shouldViewInstanceRecreated(initialShownItemIdOptional, newSelectedItemId);

    if (mustRecreate) {
        instantiateViewAndRefreshUi();
    } else {
        refreshCompleteUi(true);        // True to skip hidden rows if switching from an instrument to another.
    }
}

OptionalId EditorWithBarsControllerImpl::getCurrentlyShownItemId() const noexcept
{
    return shownItemIdOptional;
}

BarAreaSize EditorWithBarsControllerImpl::getAreaSize(const AreaType areaType) const noexcept
{
    return visibilityHandler.getAreaSize(shownItemIdOptional, areaType);
}

void EditorWithBarsControllerImpl::instantiateViewAndRefreshUi() noexcept
{
    if (parentView == nullptr) {
        jassertfalse;           // No parent view, should never happen!
        return;
    }

    // We can now create a "real" View.
    editorView = specificController->createView(*this, getXZoomValueFromZoomX(xZoom), shownItemIdOptional);
    parentView->addAndMakeVisible(*editorView);
    onParentViewResized(parentView->getXInsideComponent(), parentView->getYInsideComponent(), parentView->getAvailableWidthInComponent(),
                        parentView->getAvailableHeightInComponent());

    refreshCompleteUi(true);
}

OptionalInt EditorWithBarsControllerImpl::getHoveredBarIndex() const noexcept
{
    return currentlyHoveredBarIndex;
}

BarEditorCursorLocation EditorWithBarsControllerImpl::getBarEditorCursorLocation() const noexcept
{
    return currentBarEditorCursorLocation;
}

void EditorWithBarsControllerImpl::onUserWantsToInsert() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->duplicateColumn(itemId.getValueRef(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::onUserWantsToDelete() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    const auto performed = specificController->deleteColumn(itemId.getValueRef(), currentBarEditorCursorLocation.getX());
    if (performed) {
        correctCursorIfNeededAndRefreshUi();
    }
}

void EditorWithBarsControllerImpl::onUserWantsToToggleRetrig() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->toggleRetrig(itemId.getValueRef(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::onUserWantsToGenerateIncreasingVolume() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->generateIncreasingVolume(itemId.getValueRef(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::onUserWantsToGenerateDecreasingVolume() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->generateDecreasingVolume(itemId.getValueRef(), currentBarEditorCursorLocation.getX());
}

juce::Component* EditorWithBarsControllerImpl::getComponentWithTargetCommands() noexcept
{
    return editorView.get();
}

std::pair<OptionalInt, Id> EditorWithBarsControllerImpl::getSafeHoveredBarIndex() const noexcept
{
    if (currentlyHoveredBarIndex.isAbsent() || shownItemIdOptional.isAbsent()) {
        jassertfalse;       // Performs on nothing? Or nothing shown, shouldn't be called.
        return { {}, {} };
    }
    const auto& shownItemId = shownItemIdOptional.getValueRef();
    const auto barIndex = currentlyHoveredBarIndex.getValue();

    return { barIndex, shownItemId };
}

void EditorWithBarsControllerImpl::onUserWantsToMoveCursorInX(const bool left, const Speed speed) noexcept
{
    const auto currentX = currentBarEditorCursorLocation.getX();

    int speedInt;     // NOLINT(*-init-variables)
    switch (speed) {
        case Speed::normal: [[fallthrough]];
        default:
            speedInt = 1;
            break;
        case Speed::fast: [[fallthrough]];
        case Speed::fastest:
            speedInt = 4;
            break;
        case Speed::limit:
            // This is more tricky, because we want to reach the last column. Calculates the difference to reach it.
            static_assert(aheadColumnCount > 0);
            if (left) {
                speedInt = 999999;      // Any large value will do. Will be inverted below.
            } else {
                const auto length = getDisplayedCellIndexCount(false);
                speedInt = length - currentX - 1;
            }
            break;
    }
    if (left) {
        speedInt = -speedInt;
    }

    const auto newX = currentX + speedInt;
    moveCursorLocationTo(currentBarEditorCursorLocation.getAreaType(), newX);
}

void EditorWithBarsControllerImpl::onUserWantsToMoveCursorInY(const bool up, const Speed speed) noexcept
{
    int speedInt;     // NOLINT(*-init-variables)
    switch (speed) {
        case Speed::normal: [[fallthrough]];
        default:
            speedInt = 1;
            break;
        case Speed::fast: [[fallthrough]];
        case Speed::fastest:
            speedInt = 2;
            break;
        case Speed::limit:
            speedInt = 999999;      // Any large value will do...
            break;
    }
    if (up) {
        speedInt = -speedInt;
    }

    const auto currentX = currentBarEditorCursorLocation.getX();
    const auto currentAreaType = currentBarEditorCursorLocation.getAreaType();
    auto newAreaType = editorView->getShiftedAreaType(currentAreaType, speedInt);

    // If hidden, goes to the next/previous.
    newAreaType = findNonHiddenAreaType(newAreaType, (speedInt >= 0));

    moveCursorLocationTo(newAreaType, currentX);
}

AreaType EditorWithBarsControllerImpl::findNonHiddenAreaType(const AreaType originalAreaType, bool down) const noexcept
{
    auto areaType = originalAreaType;
    auto attempts = 30;             // Arbitrary, and should never be useful, but I wouldn't want an infinite loop...
    while (isAreaHidden(areaType) && (--attempts > 0)) {
        // The current area is hidden. Tries to go up or down.
        const auto nextAreaType = editorView->getShiftedAreaType(areaType, down ? 1 : -1);
        if (areaType == nextAreaType) {
            // Couldn't find a next area. Changes the direction.
            down = !down;
        }
        areaType = nextAreaType;
    }

    if (attempts <= 0) {
        jassertfalse;           // Should never happen. No area type is shown??
        return originalAreaType;
    }

    return areaType;
}

void EditorWithBarsControllerImpl::moveCursorToNonHiddenAreaType() noexcept
{
    const auto areaType = currentBarEditorCursorLocation.getAreaType();
    const auto newAreaType = findNonHiddenAreaType(areaType, true);     // Down is arbitrary.
    if (areaType != newAreaType) {
        moveCursorLocationTo(newAreaType, currentBarEditorCursorLocation.getX());
    }
}

void EditorWithBarsControllerImpl::onUserWantsToChangeValue(const bool increase, const Speed speed) noexcept
{
    const auto cursorLocation = getBarEditorCursorLocation();
    const auto barIndex = cursorLocation.getX();
    const auto areaType = cursorLocation.getAreaType();
    const auto value = editorView->getValue(areaType, barIndex);

    onUserWantsToSlideValue(areaType, barIndex, value, speed, increase);
}

void EditorWithBarsControllerImpl::onUserWantsToResetValue() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->onUserWantsToResetValue(itemId.getValueRef(), currentBarEditorCursorLocation.getAreaType(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::moveCursorLocationTo(const AreaType areaType, const int barIndex) noexcept
{
    moveCursorTo(areaType, barIndex, true);     // Coming from click, allows reaching the outer bounds columns.
}

void EditorWithBarsControllerImpl::correctCursorIfNeededAndRefreshUi() noexcept
{
    moveCursorTo(currentBarEditorCursorLocation.getAreaType(), currentBarEditorCursorLocation.getX(), false);
}

void EditorWithBarsControllerImpl::moveCursorTo(const AreaType areaType, const int barIndex, const bool allowAheadColumn) noexcept
{
    const auto currentCursorLocation = currentBarEditorCursorLocation;
    // Changes the X and Y, then corrects the X.
    currentBarEditorCursorLocation = BarEditorCursorLocation(barIndex, areaType);
    correctCursorXInternally(allowAheadColumn);

    // No change? Then don't do anything.
    if (currentCursorLocation == currentBarEditorCursorLocation) {
        return;
    }

    // Refreshes the old and new column. Only them are needed to be refreshed.
    refreshUiColumn(currentCursorLocation.getX());
    refreshUiColumn(currentBarEditorCursorLocation.getX());
    ensureCursorVisible();
}

bool EditorWithBarsControllerImpl::correctCursorXInternally(const bool allowAheadColumn) noexcept
{
    // Corrects the X.
    const auto currentX = currentBarEditorCursorLocation.getX();
    const auto newX  = NumberUtil::correctNumber(currentX, 0, getDisplayedCellIndexCount(allowAheadColumn) - 1);
    if (currentX == newX) {
        return false;
    }

    currentBarEditorCursorLocation = BarEditorCursorLocation(newX, currentBarEditorCursorLocation.getAreaType());

    return true;
}

void EditorWithBarsControllerImpl::onUserWantsToEditValue() noexcept
{
    const auto itemId = getCurrentlyShownItemId();
    jassert(itemId.isPresent());        // Else, abnormal!
    specificController->onUserWantsToEditValue(itemId.getValueRef(), currentBarEditorCursorLocation.getAreaType(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::ensureCursorVisible() noexcept
{
    moveCursorToNonHiddenAreaType();
    editorView->ensureVisible(currentBarEditorCursorLocation.getAreaType(), currentBarEditorCursorLocation.getX());
}

void EditorWithBarsControllerImpl::onUserWantsToDuplicateColumnWhereCursorIs() noexcept
{
    jassert(shownItemIdOptional.isPresent());
    const auto performed = specificController->duplicateColumn(shownItemIdOptional.getValueRef(), currentBarEditorCursorLocation.getX());
    // Moves forward if the action could be performed.
    if (performed) {
        onUserWantsToMoveCursorInX(false, Speed::normal);
    }
}

void EditorWithBarsControllerImpl::onUserWantsToDuplicateValueWhereCursorIs() noexcept
{
    jassert(shownItemIdOptional.isPresent());
    const auto performed = specificController->duplicateValue(shownItemIdOptional.getValueRef(), currentBarEditorCursorLocation.getX(),
        currentBarEditorCursorLocation.getAreaType());
    // Moves forward if the action could be performed.
    if (performed) {
        onUserWantsToMoveCursorInX(false, Speed::normal);
    }
}

void EditorWithBarsControllerImpl::highlightBarIndexes(const std::vector<int>& indexes) noexcept
{
    editorView->highlightBarIndexes(indexes);
}

}   // namespace arkostracker
