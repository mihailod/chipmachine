#include "EditorWithBarsViewImpl.h"

#include "../../../business/model/Speed.h"
#include "../../keyboard/CommandIds.h"
#include "../../utils/KeyboardFocusTraverserCustom.h"
#include "../controller/EditorWithBarsController.h"

namespace arkostracker 
{

EditorWithBarsViewImpl::EditorWithBarsViewImpl(EditorWithBarsController& pController, const int pXZoomRate) noexcept :
        barEditorCommands(),
        controller(pController),
        header(pController),
        allBarsArea(*this, controller, pXZoomRate),
        displayTopHeaderData(0, 0, false, false, 0, 0)               // Dummy data.
{
    // See resized() on how the initial focus in performed.
}

void EditorWithBarsViewImpl::initializeViews() noexcept
{
    header.addViewToParent(*this);
    allBarsArea.initViews(*this);
}

EditorWithBarsController& EditorWithBarsViewImpl::getController() const noexcept
{
    return controller;
}


// Component method implementations.
// ====================================

void EditorWithBarsViewImpl::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto margins = LookAndFeelConstants::margins;
    const auto x = margins;
    auto y = margins;

    // The header at the top, the rest being the bars.
    header.setBounds(x, y, width);
    const auto headerHeight = header.getHeight();       // The view manages its own size.

    y += headerHeight + (margins / 2);
    const auto allBarsAreaHeight = height - y;
    allBarsArea.locateViews(x, y, width, allBarsAreaHeight);

    // A bit hackish, but couldn't find a way to focus on the bars on init.
    // → REMOVED because it would gain focus whenever going from instr0 to X while browsing in the instrument list.
    // Probably not needed anymore!?
    //if (isShowing() || isOnDesktop()) {     // To prevent JUCE assertion.
        // allBarsArea.getFocusableComponent()->grabKeyboardFocus();
    //}
}

std::unique_ptr<juce::ComponentTraverser> EditorWithBarsViewImpl::createKeyboardFocusTraverser()
{
    // Make sure the Bars always get the focus.
    auto* component = allBarsArea.getFocusableComponent();
    if (component == nullptr) {
        jassertfalse;       // Nothing to focus on?
        return nullptr;
    }
    return std::make_unique<KeyboardFocusTraverserCustom>(*component);
}


// ApplicationCommandTarget method implementations.
// ==================================================

juce::ApplicationCommandTarget* EditorWithBarsViewImpl::getNextCommandTarget()
{
    return findFirstTargetParentComponent();        // Command not found here, goes up the UI hierarchy.
}

void EditorWithBarsViewImpl::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    barEditorCommands.getAllCommands(commands);
}

void EditorWithBarsViewImpl::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    barEditorCommands.getCommandInfo(commandId, result);
}

bool EditorWithBarsViewImpl::perform(const InvocationInfo& info)
{
    auto success = true;
    switch (info.commandID) {
        case barEditorInsert:
            controller.onUserWantsToInsert();
            break;
        case barEditorDelete:
            controller.onUserWantsToDelete();
            break;

        case barEditorCursorRight:
            controller.onUserWantsToMoveCursorInX(false, Speed::normal);
            break;
        case barEditorCursorRightFast:
            controller.onUserWantsToMoveCursorInX(false, Speed::fast);
            break;
        case barEditorCursorRightLimit:
            controller.onUserWantsToMoveCursorInX(false, Speed::limit);
            break;
        case barEditorCursorLeft:
            controller.onUserWantsToMoveCursorInX(true, Speed::normal);
            break;
        case barEditorCursorLeftFast:
            controller.onUserWantsToMoveCursorInX(true, Speed::fast);
            break;
        case barEditorCursorLeftLimit:
            controller.onUserWantsToMoveCursorInX(true, Speed::limit);
            break;
        case barEditorCursorUp:
            controller.onUserWantsToMoveCursorInY(true, Speed::normal);
            break;
        case barEditorCursorUpFast:
            controller.onUserWantsToMoveCursorInY(true, Speed::fast);
            break;
        case barEditorCursorUpLimit:
            controller.onUserWantsToMoveCursorInY(true, Speed::limit);
            break;
        case barEditorCursorDown:
            controller.onUserWantsToMoveCursorInY(false, Speed::normal);
            break;
        case barEditorCursorDownFast:
            controller.onUserWantsToMoveCursorInY(false, Speed::fast);
            break;
        case barEditorCursorDownLimit:
            controller.onUserWantsToMoveCursorInY(false, Speed::limit);
            break;
        case barEditorCursorDuplicateColumn:
            controller.onUserWantsToDuplicateColumnWhereCursorIs();
            break;
        case barEditorCursorDuplicateValue:
            controller.onUserWantsToDuplicateValueWhereCursorIs();
            break;

        case barEditorIncreaseValue:
            controller.onUserWantsToChangeValue(true, Speed::normal);
            break;
        case barEditorIncreaseValueFast:
            controller.onUserWantsToChangeValue(true, Speed::fast);
            break;
        case barEditorIncreaseValueFastest:
            controller.onUserWantsToChangeValue(true, Speed::fastest);
            break;
        case barEditorDecreaseValue:
            controller.onUserWantsToChangeValue(false, Speed::normal);
            break;
        case barEditorDecreaseValueFast:
            controller.onUserWantsToChangeValue(false, Speed::fast);
            break;
        case barEditorDecreaseValueFastest:
            controller.onUserWantsToChangeValue(false, Speed::fastest);
            break;
        case barEditorResetValue:
            controller.onUserWantsToResetValue();
            break;

        case barEditorCursorEdit:
            controller.onUserWantsToEditValue();
            break;

        case barEditorCursorSetMainLoopStart:
            controller.onUserWantsToSetLoopStartFromCursor(true);
            break;
        case barEditorCursorSetMainLoopEnd:
            controller.onUserWantsToSetEndFromCursor(true);
            break;
        case barEditorToggleMainLoop:
            controller.onUserWantsToToggleLoopFromCursor(true);
            break;
        case barEditorIncreaseSpeed:
            controller.onUserWantsToIncreaseSpeed(1);
            break;
        case barEditorDecreaseSpeed:
            controller.onUserWantsToIncreaseSpeed(-1);
            break;

        case barEditorCursorSetAutoSpreadLoopStart:
            controller.onUserWantsToSetLoopStartFromCursor(false);
            break;
        case barEditorCursorSetAutoSpreadLoopEnd:
            controller.onUserWantsToSetEndFromCursor(false);
            break;
        case barEditorToggleAutoSpreadLoop:
            controller.onUserWantsToToggleLoopFromCursor(false);
            break;

        case barEditorHideUnusedRows:
            controller.onUserWantsToShrinkRows(false);
            break;
        case barEditorHideUnusedRowsFull:
            controller.onUserWantsToShrinkRows(true);
            break;
        case barEditorEnlargeCurrentRow:
            controller.onUserWantsToEnlargeArea();
            break;
        case barEditorShrinkCurrentRow:
            controller.onUserWantsToShrinkArea();
        break;

        case barEditorToggleHideInRowAboveCursor:
            controller.onUserWantsToToggleHideFromCursor(true);
            break;
        case barEditorToggleHideInRowBelowCursor:
            controller.onUserWantsToToggleHideFromCursor(false);
            break;

        default:
            success = false;
            break;
    }
    return success;
}


// EditorWithBarsView method implementations.
// =============================================

void EditorWithBarsViewImpl::setDisplayedBarCount(const int newBarCount) noexcept
{
    allBarsArea.setDisplayedBarCount(newBarCount);
}

void EditorWithBarsViewImpl::showBarData(const AreaType areaType, const int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) noexcept
{
    allBarsArea.showBarData(areaType, barIndex, barData, captionData);
}

void EditorWithBarsViewImpl::setMetadataDisplayData(const std::unordered_map<AreaType, BarAreaSize> newAreaTypeToSize,
                                                    const std::unordered_map<AreaType, LeftHeaderDisplayData> newAreaTypeToLeftHeaderDisplayData,
                                                    const std::unordered_map<AreaType, Loop> newAreaTypeToAutoSpreadData) noexcept
{
    jassert(newAreaTypeToSize.size() == newAreaTypeToLeftHeaderDisplayData.size());
    jassert(newAreaTypeToAutoSpreadData.size() == newAreaTypeToLeftHeaderDisplayData.size());

    // Any change in the size/spread bars?
    const auto hasAreaSizesChanged = allBarsArea.setAreaSizesData(newAreaTypeToSize);
    const auto hasAreaAutoSpreadChanged = allBarsArea.setAreaAutoSpreadData(newAreaTypeToAutoSpreadData);
    const auto hasLeftHeaderDisplayDataChanged = allBarsArea.setLeftHeaderDisplayData(newAreaTypeToLeftHeaderDisplayData);
    const auto refreshMainViewPort = hasAreaSizesChanged || hasAreaAutoSpreadChanged;

    if (hasAreaAutoSpreadChanged) {
        allBarsArea.refreshAutoSpreadViewsFromInternalData();
    }

    if (refreshMainViewPort || hasLeftHeaderDisplayDataChanged) {
        // Locates the Views.
        allBarsArea.locateViews();
    }

    // Any change in the left headers?
    if (hasLeftHeaderDisplayDataChanged) {
        // Refreshes the UI.
        allBarsArea.refreshLeftHeaders();
    }
}

void EditorWithBarsViewImpl::setDisplayTopHeaderData(const DisplayTopHeaderData& newDisplayTopHeaderData) noexcept
{
    // Any change?
    if (displayTopHeaderData == newDisplayTopHeaderData) {
        return;
    }
    displayTopHeaderData = newDisplayTopHeaderData;

    const auto loopStart = newDisplayTopHeaderData.getLoopStart();
    const auto end = newDisplayTopHeaderData.getEnd();
    const auto isLooping = newDisplayTopHeaderData.isLoop();

    // Updates the header button.
    header.setDisplayedData(loopStart, end, isLooping, newDisplayTopHeaderData.isRetrig(), newDisplayTopHeaderData.getSpeed(), newDisplayTopHeaderData.getShift());

    // Updates the Loop Bar.
    const LoopBar::DisplayData loopBarDisplayData(isLooping, loopStart, end);
    allBarsArea.setLoopBarDisplayData(loopBarDisplayData);
}

void EditorWithBarsViewImpl::setXZoomRate(const int xZoomRate) noexcept
{
    allBarsArea.setXZoomRate(xZoomRate);
}

int EditorWithBarsViewImpl::getValue(const AreaType areaType, const int barIndex) noexcept
{
    return allBarsArea.getValue(areaType, barIndex);
}

AreaType EditorWithBarsViewImpl::getShiftedAreaType(const AreaType areaType, const int offset) const noexcept
{
    return allBarsArea.getShiftedAreaType(areaType, offset);
}

void EditorWithBarsViewImpl::ensureVisible(const AreaType areaType, const int barIndex) noexcept
{
    allBarsArea.ensureVisible(areaType, barIndex);
}

void EditorWithBarsViewImpl::highlightBarIndexes(const std::vector<int>& indexes) noexcept
{
    allBarsArea.highlightBarIndexes(indexes);
}

}   // namespace arkostracker
