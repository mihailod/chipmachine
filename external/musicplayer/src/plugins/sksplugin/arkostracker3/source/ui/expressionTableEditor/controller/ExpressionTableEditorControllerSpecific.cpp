#include "ExpressionTableEditorControllerSpecific.h"

#include "../../../controllers/MainController.h"
#include "../../../song/ExpressionConstants.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/SetUtil.h"
#include "../../expressionTableEditor/view/ExpressionTableEditorViewNone.h"

namespace arkostracker 
{

ExpressionTableEditorControllerSpecific::ExpressionTableEditorControllerSpecific(const bool pIsArpeggio, MainController& pMainController,
                                                                                 EditorWithBarsController& pEditorController) noexcept :
        isArpeggio(pIsArpeggio),
        mainController(pMainController),
        songController(pMainController.getSongController()),
        contextualMenu(),
        editorController(pEditorController),
        quickEditController(pMainController.getSongController())
{
    songController.getExpressionObservers(isArpeggio).addObserver(this);
    mainController.observers().getSelectedExpressionIndexObservers(isArpeggio).addObserver(this);
}

ExpressionTableEditorControllerSpecific::~ExpressionTableEditorControllerSpecific()
{
    songController.getExpressionObservers(isArpeggio).removeObserver(this);
    mainController.observers().getSelectedExpressionIndexObservers(isArpeggio).removeObserver(this);
}

EditorWithBarsController& ExpressionTableEditorControllerSpecific::getEditorController() const
{
    return editorController;
}


// ArpeggioTableEditorControllerSpecific method implementations.
// ================================================================

std::unique_ptr<EditorWithBarsView> ExpressionTableEditorControllerSpecific::createView(EditorWithBarsController& editorControllerToUse, const int xZoomRate,
                                                                                        const OptionalId& selectedItemId) noexcept
{
    // If selected index 0, shows a special view.
    std::unique_ptr<EditorWithBarsView> createdView;

    if (selectedItemId.isAbsent() || songController.isReadOnlyExpression(isArpeggio, selectedItemId.getValueRef())) {
        createdView = std::make_unique<ExpressionTableEditorViewNone>(isArpeggio);
    } else {
        createdView = createViewInstance(editorControllerToUse, xZoomRate);
    }

    return createdView;
}

bool ExpressionTableEditorControllerSpecific::shouldViewInstanceRecreated(const OptionalId& currentItemId, const Id& newItemId) const noexcept
{
    if (currentItemId == newItemId) {
        return false;
    }

    // Change required when going to 0, or coming from 0.
    if (songController.isReadOnlyExpression(isArpeggio, newItemId)) {
        return true;
    }

    if (currentItemId.isAbsent()) {
        return true;
    }
    return (songController.isReadOnlyExpression(isArpeggio, currentItemId.getValueRef()));
}

EditorWithBarsView::DisplayTopHeaderData ExpressionTableEditorControllerSpecific::getDisplayedTopHeaderData(const Id& expressionId) const noexcept
{
    const auto& expressionHandler = songController.getExpressionHandler(isArpeggio);

    auto loopStart = 0;
    auto end = 0;
    auto speed = 0;
    auto shift = 0;
    expressionHandler.performOnConstExpression(expressionId, [&] (const Expression& expression) {
        loopStart = expression.getLoopStart();
        end = expression.getEnd();
        speed = expression.getSpeed();
        shift = expression.getShift();
    });

    return { loopStart, end, true, false, speed, shift };
}

int ExpressionTableEditorControllerSpecific::getItemLength(const Id& expressionId) const noexcept
{
    const auto& expressionHandler = songController.getExpressionHandler(isArpeggio);

    auto length = 0;
    expressionHandler.performOnConstExpression(expressionId, [&] (const Expression& expression) {
        length = expression.getLength();
    });

    return length;
}

std::unordered_map<AreaType, BarAndCaptionData> ExpressionTableEditorControllerSpecific::createBarsData(const Id& expressionId, const int barIndex) const noexcept
{
    // Creates the bar values.
    // Gets the expression value for this index.
    auto value = 0;
    auto endLoopIndex = 0;
    auto length = 0;
    songController.getExpressionHandler(isArpeggio).performOnConstExpression(expressionId, [&] (const Expression& expression) {
        value = expression.getValue(barIndex);          // Takes care of the out-of-bound indexes.
        endLoopIndex = expression.getEnd();
        length = expression.getLength();
    });

    const auto withinLoop = (barIndex <= endLoopIndex);
    const auto outOfBounds = (barIndex >= length);
    const auto hovered = (editorController.getHoveredBarIndex() == barIndex);

    // Gather cursor information.
    const auto cursorLocation = editorController.getBarEditorCursorLocation();
    const auto cursorLocationAreaType = cursorLocation.getAreaType();
    const auto cursorXOk = (barIndex == cursorLocation.getX());

    std::unordered_map<AreaType, BarAndCaptionData> areaTypeToData;

    // Fills each area for this column with the data to display.
    fillAreaTypeToData(areaTypeToData, value, withinLoop, outOfBounds, hovered, cursorXOk, cursorLocationAreaType);

    return areaTypeToData;
}

void ExpressionTableEditorControllerSpecific::checkValueAndChangeItem(const Id& expressionId, const AreaType areaType, const int barIndex,
                                                                      const int originalValue) noexcept
{
    // Lets the subclass corrected the value and checks whether it is worth being stored.
    const auto newValue = correctAndCheckValueFromUi(songController, expressionId, barIndex, areaType, originalValue);
    if (newValue.isAbsent()) {
        return;
    }

    // Modifies the Expression. The target bar may not exist! The action may create some.
    songController.setExpressionCell(isArpeggio, expressionId, barIndex, newValue.getValue());
}

void ExpressionTableEditorControllerSpecific::onUserWantsToResetValue(const Id& itemId, const AreaType areaType, const int barIndex) noexcept
{
    auto value = 0;

    switch (areaType) {
        case AreaType::primaryArpeggioNoteInOctave:
            value = EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue();
            break;
        case AreaType::primaryArpeggioOctave:
            value = EditorWithBarsController::getDefaultArpeggioOctaveValue();
            break;
        case AreaType::primaryPitch:
            value = EditorWithBarsController::getDefaultPitchValue();
            break;
        case AreaType::soundType: [[fallthrough]];
        case AreaType::envelope: [[fallthrough]];
        case AreaType::noise: [[fallthrough]];
        case AreaType::primaryPeriod: [[fallthrough]];
        case AreaType::secondaryPeriod: [[fallthrough]];
        case AreaType::secondaryArpeggioNoteInOctave: [[fallthrough]];
        case AreaType::secondaryArpeggioOctave: [[fallthrough]];
        case AreaType::secondaryPitch:
        case AreaType::sidIsActivatedAndRatio:
        case AreaType::sidBalancePercent:
        case AreaType::sidLowShelfVolume:
        case AreaType::sidArpeggio:
        case AreaType::sidPitch:
            jassertfalse;       // Shouldn't happen.
            return;
    }

    checkValueAndChangeItem(itemId, areaType, barIndex, value);
}

bool ExpressionTableEditorControllerSpecific::isAutoSpreadAllowed() const noexcept
{
    return false;
}

Loop ExpressionTableEditorControllerSpecific::getAutoSpreadLoop(const Id& /*expressionId*/ , AreaType /*areaType*/) const noexcept
{
    jassertfalse;           // Shouldn't be called, no auto-spread in this implementation!
    return Loop(0, 0, false);
}

std::pair<int, Loop> ExpressionTableEditorControllerSpecific::getItemLengthAndLoop(const Id& expressionId) const noexcept
{
    auto length = 1;     // Initialized in case the item doesn't exist.
    Loop loop;
    songController.getExpressionHandler(isArpeggio).performOnConstExpression(expressionId, [&](const Expression& expression) {
        length = expression.getLength();
        loop = expression.getLoop();
    });

    return { length, loop };
}

bool ExpressionTableEditorControllerSpecific::isItemMetadataChangeFlagsInteresting(const unsigned int whatChanged) noexcept
{
    return NumberUtil::isBitPresent(whatChanged, static_cast<unsigned int>(What::loop))
        || NumberUtil::isBitPresent(whatChanged, static_cast<unsigned int>(What::speed))
        || NumberUtil::isBitPresent(whatChanged, static_cast<unsigned int>(What::shift));
}

void ExpressionTableEditorControllerSpecific::onUserWantsToToggleLoop(const Id& /*itemId*/, OptionalValue<AreaType> /*areaType*/) noexcept
{
    // Nothing to do (call be called from keyboard shortcut).
}

void ExpressionTableEditorControllerSpecific::onUserWantsToToggleInstrumentRetrig(const Id& /*itemId*/) noexcept
{
    jassertfalse;    // Not relevant, shouldn't be called.
}

void ExpressionTableEditorControllerSpecific::onUserWantsToSetAutoSpreadStartAndEnd(const Id& /*itemId*/, AreaType /*areaType*/, OptionalInt /*newStart*/,
                                                                                    OptionalInt /*newEnd*/) noexcept
{
    // Nothing to do (call be called from keyboard shortcut).
}

void ExpressionTableEditorControllerSpecific::setItemMetadata(const Id& expressionId, const OptionalInt newStart, const OptionalInt newEnd, const OptionalInt newSpeed,
                                                              const OptionalInt newShift, OptionalBool /*newIsLoop*/, OptionalBool /*newIsRetrig*/,
                                                              std::unordered_map<PsgSection, Loop> /*modifiedSectionToAutoSpreadLoop*/) noexcept
{
    songController.setExpressionMetadata(isArpeggio, expressionId, newStart, newEnd, newSpeed, newShift);
}

bool ExpressionTableEditorControllerSpecific::duplicateColumn(const Id& itemId, const int cellIndex) noexcept
{
    return songController.duplicateExpressionCell(isArpeggio, itemId, cellIndex);
}

bool ExpressionTableEditorControllerSpecific::deleteColumn(const Id& itemId, const int cellIndex) noexcept
{
    return songController.deleteExpressionCell(isArpeggio, itemId, cellIndex);
}

bool ExpressionTableEditorControllerSpecific::duplicateValue(const Id& itemId, const int cellIndex, AreaType /*areaType*/) noexcept
{
    const auto& expressionHandler = songController.getExpressionHandler(isArpeggio);
    auto value = 0;
    // Takes the current value.
    expressionHandler.performOnConstExpression(itemId, [&](const Expression& expression) {
        value = expression.getValue(cellIndex);
    });

    // Applies it to the next cell.
    songController.setExpressionCell(isArpeggio, itemId, cellIndex + 1, value);

    return true;
}

void ExpressionTableEditorControllerSpecific::onUserWantsToIncreaseSpeed(const Id& itemId, const int step) noexcept
{
    // Gets the current speed.
    const auto speed = NumberUtil::correctNumber(getItemSpeed(itemId) + step, ExpressionConstants::minimumSpeed, ExpressionConstants::maximumSpeed);

    // Sets it.
    songController.setExpressionMetadata(isArpeggio, itemId, { }, { }, speed, { });
}

void ExpressionTableEditorControllerSpecific::toggleRetrig(const Id& /*itemId*/, int /*cellIndex*/) noexcept
{
    jassertfalse;       // Nothing to do here, shouldn't even be called.
}

void ExpressionTableEditorControllerSpecific::generateIncreasingVolume(const Id& /*itemId*/, int /*cellIndex*/) noexcept
{
    jassertfalse;       // Nothing to do here, shouldn't even be called.
}

void ExpressionTableEditorControllerSpecific::generateDecreasingVolume(const Id& /*itemId*/, int /*cellIndex*/) noexcept
{
    jassertfalse;       // Nothing to do here, shouldn't even be called.
}

void ExpressionTableEditorControllerSpecific::openContextualMenu(const Id & /*itemId*/, AreaType /*areaType*/, int /*cellIndex*/,
                                                                 const std::function<void()> onMenuItemClicked) noexcept
{
    auto *parentComponent = editorController.getComponentWithTargetCommands();

    contextualMenu = std::make_unique<ExpressionEditorContextualMenu>(mainController.getCommandManager());
    contextualMenu->openContextualMenu(parentComponent, onMenuItemClicked);
}

void ExpressionTableEditorControllerSpecific::onUserWantsToEditValue(const Id& itemId, const AreaType areaType, const int barIndex) noexcept
{
    quickEditController.edit(itemId, areaType, barIndex);
}

bool ExpressionTableEditorControllerSpecific::doesContainOnlyDefaultData(const Id& /*itemId*/, AreaType /*areaType*/) noexcept
{
    return false;       // Expressions rows cannot be hidden.
}

bool ExpressionTableEditorControllerSpecific::canHideRow(const Id& /*itemId*/, AreaType /*areaType*/) noexcept
{
    return false;       // Expressions rows cannot be hidden.
}

BarAreaSize ExpressionTableEditorControllerSpecific::getMinimumSizeForData(const Id& /*itemId*/, const AreaType areaType, const bool /*forceShrinkFull*/) noexcept
{
    // Returns the init size, enough.
    const auto sizes = getAreaTypesToInitialSize();
    if (const auto it = sizes.find(areaType); it != sizes.cend()) {
        return it->second;
    }

    jassertfalse;       // Not found, abnormal!
    return BarAreaSize::first;
}

int ExpressionTableEditorControllerSpecific::getItemSpeed(const Id& expressionId) noexcept
{
    auto speed = 0;
    songController.getExpressionHandler(isArpeggio).performOnConstExpression(expressionId, [&](const Expression& expression) {
        speed = expression.getSpeed();
    });

    return speed;
}

int ExpressionTableEditorControllerSpecific::getItemShift(const Id& expressionId) noexcept
{
    auto shift = 0;
    songController.getExpressionHandler(isArpeggio).performOnConstExpression(expressionId, [&](const Expression& expression) {
        shift = expression.getShift();
    });

    return shift;
}

int ExpressionTableEditorControllerSpecific::correctItemSpeed(const int speed) const noexcept
{
    return NumberUtil::correctNumber(speed, ExpressionConstants::minimumSpeed, ExpressionConstants::maximumSpeed);
}

int ExpressionTableEditorControllerSpecific::correctItemShift(const int shift) const noexcept
{
    return NumberUtil::correctNumber(shift, ExpressionConstants::minimumShift, ExpressionConstants::maximumShift);
}


// ExpressionChangeObserver method implementations.
// ===================================================

void ExpressionTableEditorControllerSpecific::onExpressionChanged(const Id& expressionId, const unsigned int whatChanged)
{
    editorController.onItemMetadataChanged(expressionId, whatChanged);
}

void ExpressionTableEditorControllerSpecific::onExpressionCellChanged(const Id& expressionId, const int cellIndex, const bool mustAlsoRefreshPastIndex)
{
    editorController.onItemCellChanged(expressionId, cellIndex, mustAlsoRefreshPastIndex);
}

void ExpressionTableEditorControllerSpecific::onExpressionsInvalidated()
{
    // Nothing to do, the selected expression will change, thus provoking a redraw.
}


// SelectedExpressionIndexObserver method implementations.
// ==========================================================

void ExpressionTableEditorControllerSpecific::onSelectedExpressionChanged(const bool newIsArpeggio, const OptionalId& newExpressionId, const bool forceRefresh)
{
    if (isArpeggio != newIsArpeggio) {          // Sanity check, only arpeggios interest us.
        return;
    }
    if (newExpressionId.isAbsent()) {
        // Nothing we can do.
        return;
    }

    const auto shownExpressionId = editorController.getCurrentlyShownItemId();
    // Must be refreshed?
    if (forceRefresh || (shownExpressionId != newExpressionId)) {
        editorController.onSelectedItemChangedMustRefreshUi(newExpressionId.getValueRef());
    }
}

OptionalId ExpressionTableEditorControllerSpecific::getCurrentItemId() const noexcept
{
    return mainController.getSelectedExpressionId(isArpeggio);
}

}   // namespace arkostracker
