#include "AbstractListItem.h"

#include "../../utils/ListItemListener.h"
#include "DraggedItem.h"

namespace arkostracker 
{

AbstractListItem::AbstractListItem(int pRowNumber, bool pIsRowSelected, DraggedItem::ItemType pDraggedItemType, ListItemListener& pListener) noexcept :
        rowNumber(pRowNumber),
        rowSelected(pIsRowSelected),
        draggedItemType(pDraggedItemType),
        listener(pListener),
        isHoveredTop(false),
        isHoveredBottom(false)
{
}

void AbstractListItem::updateData(int newRowNumber, bool newIsRowSelected) noexcept
{
    rowNumber = newRowNumber;
    rowSelected = newIsRowSelected;

    // Lets the subclass do more work, if they want to.
    updateDataInternal();

    // This is useful when drag'n'dropping and the need to update the whole list.
    repaint();
}


// DragAndDropTarget method implementations.
// =================================================

bool AbstractListItem::shouldDrawDragImageWhenOver()
{
    return true;
}

bool AbstractListItem::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    juce::ReferenceCountedObject* ptr = dragSourceDetails.description.getObject();
    auto* draggedItem = dynamic_cast<DraggedItem*>(ptr);
    if (draggedItem == nullptr) {
        return false;
    }

    // The right kind of item?
    return (draggedItem->itemType == draggedItemType);
}

void AbstractListItem::itemDropped(const SourceDetails& dragSourceDetails)
{
    removeHoverBars();

    // Extracts the dropped data.
    juce::ReferenceCountedObject* ptr = dragSourceDetails.description.getObject();
    auto* draggedItem = dynamic_cast<DraggedItem*>(ptr);
    // Sanity check, but shouldn't be useful, as it has been tested before!
    if (draggedItem == nullptr) {
        jassertfalse;
        return;
    }

    auto successAndIsTopHighlighted = manageDraggedItemValidity(dragSourceDetails);
    if (!successAndIsTopHighlighted.first) {
        return;
    }

    const auto sourceRowIndex = draggedItem->sourceRowIndex;
    const auto movingAtTop = successAndIsTopHighlighted.second;
    const auto destinationRowIndex = rowNumber + (movingAtTop ? 0 : 1);

    // Notifies the client.
    listener.onDragListItem(sourceRowIndex, destinationRowIndex);
}

void AbstractListItem::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
    removeHoverBars();
}

void AbstractListItem::itemDragMove(const SourceDetails& dragSourceDetails)
{
    // Valid drag?
    auto successAndIsTopHighlighted = manageDraggedItemValidity(dragSourceDetails);
    if (!successAndIsTopHighlighted.first) {
        return;
    }

    const auto newIsHoveredTop = successAndIsTopHighlighted.second;
    const auto newIsHoveredBottom = !newIsHoveredTop;

    // Needs to repaint if hovering over a different area?
    if ((isHoveredTop != newIsHoveredTop) || (isHoveredBottom != newIsHoveredBottom)) {
        isHoveredTop = newIsHoveredTop;
        isHoveredBottom = newIsHoveredBottom;
        repaint();
    }
}

std::pair<bool, bool> AbstractListItem::manageDraggedItemValidity(const SourceDetails& dragSourceDetails) const noexcept
{
    juce::ReferenceCountedObject* ptr = dragSourceDetails.description.getObject();
    auto* draggedItem = dynamic_cast<DraggedItem*>(ptr);
    if (draggedItem == nullptr) {
        return { false, false };
    }

    // Drop allowed? Useful for row 0 that can be protected, for example.
    if (!isDropAllowedHere(rowNumber)) {
        return { false, false };
    }

    const auto sourceRow = draggedItem->sourceRowIndex;

    // Must not be the same row, and must be the of right type.
    if ((draggedItem->itemType != draggedItemType) || (sourceRow == rowNumber)) {
        return { false, false };
    }

    // As in OpenMPT, if the source is after, insert before.
    const auto newIsHoveredTop = (rowNumber < sourceRow);

    return { true, newIsHoveredTop };
}


// Component method implementations.
// =================================================

void AbstractListItem::mouseDrag(const juce::MouseEvent& /*event*/)
{
    // Drag allowed? Useful for row 0 that can be protected, for example.
    if (!isDragAllowed(rowNumber)) {
        return;
    }

    // Huge trick to notify the container we want to start the drag'n'drop.
    // With the help of https://bitbucket.org/MatkatMusic/listboxreorder/src/master/Source/MainComponent.h
    if (juce::DragAndDropContainer* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
        // Creates the object to transfer.
        auto* obj = new DraggedItem(rowNumber, draggedItemType);     // Reference-counted.
        const juce::var sourceDescription(obj);

        container->startDragging(sourceDescription, this);
    }
}

void AbstractListItem::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown()) {
        listener.onListItemRightClicked(rowNumber);
    } else {
        // Note that I cannot make a down+drag directly...
        listener.onListItemClicked(rowNumber, event.mods, true);
    }
}

void AbstractListItem::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        listener.onListItemDoubleClicked(rowNumber);
    }
}

void AbstractListItem::mouseUp(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        listener.onListItemClicked(rowNumber, event.mods, false);
    }
}


// =================================================

void AbstractListItem::removeHoverBars()
{
    isHoveredTop = false;
    isHoveredBottom = false;
    repaint();
}

void AbstractListItem::showPossibleDragNDropHighlight(juce::Graphics& g, const juce::Colour& backgroundColor) const noexcept
{
    // Any drag'n'drop area to highlight?
    if (isHoveredTop || isHoveredBottom) {
        const auto width = getWidth();
        const auto height = getHeight();

        const auto hoveringAreaHeight = height / 3;
        const auto hoverY = isHoveredTop ? 0 : (height - hoveringAreaHeight);

        g.setColour(backgroundColor.contrasting().withAlpha(0.7F));
        g.fillRect(0, hoverY, width, hoveringAreaHeight);
    }
}

void AbstractListItem::updateDataInternal()
{
    // The default implementation does nothing.
}

DraggedItem::ItemType AbstractListItem::getDraggedItemType() const noexcept
{
    return draggedItemType;
}

int AbstractListItem::getRowNumber() const noexcept
{
    return rowNumber;
}

bool AbstractListItem::isRowSelected() const noexcept
{
    return rowSelected;
}

}   // namespace arkostracker
