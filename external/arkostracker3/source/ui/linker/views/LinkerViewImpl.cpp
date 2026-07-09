#include "LinkerViewImpl.h"

#include "../../keyboard/CommandIds.h"
#include "../../utils/KeyboardFocusTraverserCustom.h"
#include "../../utils/ViewportUtil.h"
#include "../controller/LinkerController.h"

namespace arkostracker 
{

LinkerViewImpl::LinkerViewImpl(LinkerController& pLinkerController) noexcept :
        LinkerView(pLinkerController),
        linkerPanelAreaCommands(pLinkerController),
        itemsViewport(),
        itemsHolder(*this, *this, *this)
{
    itemsViewport.setViewedComponent(&itemsHolder, false);
    itemsViewport.setScrollBarsShown(false, true, false, false);
    addAndMakeVisible(itemsViewport);
}

std::unique_ptr<juce::ComponentTraverser> LinkerViewImpl::createKeyboardFocusTraverser()
{
    // Make sure the ItemsHolders always get the focus. Strangely enough, using the ItemsHolders crashes (infinite loop?).
    return std::make_unique<KeyboardFocusTraverserCustom>(itemsViewport);
}


// ======================================================

void LinkerViewImpl::refreshAllItems(const std::vector<PatternItemView::DisplayedData>& itemsData, const LoopBar::DisplayData& loopData,
                                     const PlayLineView::DisplayedData& playLineData, const std::unordered_map<int, juce::String>& positionToMarker) noexcept
{
    itemsHolder.setDisplayedData(itemsData, loopData, playLineData, positionToMarker);
}

void LinkerViewImpl::scrollToItem(const int position) noexcept
{
    const auto [xStart, width] = itemsHolder.getItemXAndWidth(position);
    //const auto xEnd = xStart + width - 1;

    const auto viewPortX = itemsViewport.getViewPositionX();
    const auto viewPortWidth = itemsViewport.getWidth();

    // If the item is not fully visible, centers on it.
    const auto newViewPortX = ViewportUtil::calculateIfScrollNeeded(xStart, width, viewPortX, viewPortWidth);
    itemsViewport.setViewPosition(newViewPortX, 0);
}

void LinkerViewImpl::editItems(const std::set<int>& positionIndexes) noexcept
{
    linkerController.editPositions(positionIndexes);
}


// ApplicationCommandTarget method implementations.
// ================================================

juce::ApplicationCommandTarget* LinkerViewImpl::getNextCommandTarget()
{
    return findFirstTargetParentComponent();        // Command not found here, goes up the UI hierarchy.
}

void LinkerViewImpl::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    linkerPanelAreaCommands.getAllCommands(commands);
}

void LinkerViewImpl::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    linkerPanelAreaCommands.getCommandInfo(commandId, result);
}

bool LinkerViewImpl::perform(const InvocationInfo& info)
{
    auto success = true;
    switch (info.commandID) {
        case juce::StandardApplicationCommandIDs::cut:
            linkerController.cutSelectionToClipboard();
            break;
        case juce::StandardApplicationCommandIDs::copy:
            linkerController.copySelectionToClipboard();
            break;
        case juce::StandardApplicationCommandIDs::paste:
            linkerController.pasteClipboard();
            break;
        case linkerGotoNext:
            linkerController.onWantToMoveSelection(true);
            break;
        case linkerGotoPrevious:
            linkerController.onWantToMoveSelection(false);
            break;
        case linkerGotoNextPage:
            linkerController.onWantToMoveSelectionFast(true);
            break;
        case linkerGotoPreviousPage:
            linkerController.onWantToMoveSelectionFast(false);
            break;
        case linkerGotoEnd:
            linkerController.onWantToMoveSelectionToLimit(true);
            break;
        case linkerGotoStart:
            linkerController.onWantToMoveSelectionToLimit(false);
            break;

        case linkerGrowNext:
            linkerController.onWantToGrowSelection(true);
            break;
        case linkerGrowPrevious:
            linkerController.onWantToGrowSelection(false);
            break;
        case linkerGrowNextPage:
            linkerController.onWantToGrowSelectionFast(true);
            break;
        case linkerGrowPreviousPage:
            linkerController.onWantToGrowSelectionFast(false);
            break;
        case linkerGrowEnd:
            linkerController.onWantToGrowSelectionToLimit(true);
            break;
        case linkerGrowStart:
            linkerController.onWantToGrowSelectionToLimit(false);
            break;

        case linkerDuplicateSelectedPositions:
            linkerController.duplicateSelectedPositions();
            break;
        case linkerCloneSelectedPositions:
            linkerController.cloneSelectedPositions();
            break;
        case linkerDeleteSelectedPositions:
            linkerController.deleteSelectedPositions();
            break;
        case linkerCreateNew:
            linkerController.createNew();
            break;

        case linkerEditPositions:
            linkerController.editSelectedPosition();
            break;
        case linkerSelectPosition:
            linkerController.onWantToOpenPatternViewer();
            break;
        case linkerMarkAsLoopStartAndEnd:
            linkerController.markSelectionAsLoopStartAndEnd();
            break;
        case linkerMarkLoopStart:
            linkerController.markLoopStart();
            break;
        case linkerMarkEnd:
            linkerController.markEnd();
            break;

        case linkerIncreasePatternIndex:
            linkerController.onWantToIncreasePatternIndex(1);
            break;
        case linkerDecreasePatternIndex:
            linkerController.onWantToIncreasePatternIndex(-1);
            break;

        default:
            success = false;
            jassertfalse;
            break;
    }
    return success;
}


// ============================================================================

void LinkerViewImpl::resized()
{
    const auto margins = LookAndFeelConstants::margins;
    const auto totalWidth = getWidth();
    const auto totalHeight = getHeight();

    const auto itemsViewportY = margins;
    const auto itemsViewportHeight = totalHeight - itemsViewportY;
    itemsViewport.setBounds(0, itemsViewportY, totalWidth, itemsViewportHeight);
    itemsHolder.setSize(itemsHolder.getWidth(), itemsViewportHeight);       // Let the holder sets its own width according to its content.
}


// LoopBar::Listener method implementations.
// ===============================================

void LinkerViewImpl::onUserWantsToSetLoopBarStart(int /*id*/, const int newStart) noexcept
{
    onWantToSetLoopStartOrEnd(newStart, OptionalInt());
}

void LinkerViewImpl::onUserWantsToSetLoopBarEnd(int /*id*/, const int newEnd) noexcept
{
    onWantToSetLoopStartOrEnd(OptionalInt(), newEnd);
}

void LinkerViewImpl::onUserWantsToToggleLoop(int /*id*/) noexcept
{
    // Nothing to do.
}

void LinkerViewImpl::onWantToSetLoopStartOrEnd(const OptionalInt newLoopStartPosition, const OptionalInt newEndPosition) const noexcept
{
    linkerController.onWantToSetLoopStartOrEnd(newLoopStartPosition, newEndPosition);
}


// PatternItemView::Listener method implementations.
// ===============================================

void LinkerViewImpl::onPatternItemClicked(const int position, const bool down, const bool shift, const bool control)
{
    linkerController.onPatternItemClicked(position, down, shift, control);
}

void LinkerViewImpl::onPatternItemDoubleClicked(const int position)
{
    linkerController.onPatternItemDoubleClicked(position);
}

void LinkerViewImpl::onUserWantsToEditPosition(const int position)
{
    linkerController.onWantToEditPosition(position);
}

void LinkerViewImpl::onPatternItemRightClicked(const int position)
{
    linkerController.onPatternItemRightClicked(position);
}

void LinkerViewImpl::onWantToMoveOrDuplicatePatternItemView(const int originalPosition, const int destinationPosition, const bool duplicate)
{
    linkerController.onWantToMoveOrDuplicateSelectedPositions(originalPosition, destinationPosition, duplicate);
}

void LinkerViewImpl::onMouseWheelOnPatternItem(const int originalPosition, const int offset)
{
    linkerController.onWantToIncDecPatternItemValue(originalPosition, -offset);
}

void LinkerViewImpl::onAddNewPositionClicked(const int position) noexcept
{
    linkerController.onAddNewPositionClicked(position);
}

void LinkerViewImpl::onClonePositionClicked(const int position) noexcept
{
    linkerController.onClonePositionClicked(position);
}

void LinkerViewImpl::onRepeatPositionClicked(const int position) noexcept
{
    linkerController.onRepeatPositionClicked(position);
}


// ItemsHolder::ScrollListener method implementations.
// ===================================================

void LinkerViewImpl::manageScrollingIfNeeded(const int mouseX)
{
    constexpr auto edgeDistance = 100;
    constexpr auto scrollSpeed = 40;

    const auto viewPortViewPositionX = itemsViewport.getViewPositionX();
    auto motionX = 0;

    if (mouseX < (viewPortViewPositionX + edgeDistance)) {
        motionX = -scrollSpeed;
    } else if (mouseX > (viewPortViewPositionX + itemsViewport.getWidth() - edgeDistance)) {
        motionX = +scrollSpeed;
    }

    if (motionX != 0) {
        itemsViewport.setViewPosition(viewPortViewPositionX + motionX, itemsViewport.getViewPositionY());
    }
}

}   // namespace arkostracker
