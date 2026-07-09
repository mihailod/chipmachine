#include "PatternItemView.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/dragnDrop/DragAndDropHelper.h"

namespace arkostracker 
{

const int PatternItemView::draggedOverRectangleSize = 12;

// PatternItemView::DisplayedData
// ===============================================

PatternItemView::DisplayedData::DisplayedData(const int pPosition, const int pPatternIndex, const juce::Colour pPatternColor, const bool pWithinPlayRange, const bool pWithTransposition,
                                              const bool pSelected, const bool pCurrent) noexcept :
        position(pPosition),
        patternIndex(pPatternIndex),
        patternColor(pPatternColor),
        withinPlayRange(pWithinPlayRange),
        withTransposition(pWithTransposition),
        selected(pSelected),
        current(pCurrent)
{
}

int PatternItemView::DisplayedData::getPosition() const noexcept
{
    return position;
}

int PatternItemView::DisplayedData::getPatternIndex() const noexcept
{
    return patternIndex;
}

juce::Colour PatternItemView::DisplayedData::getPatternColor() const noexcept
{
    return patternColor;
}

bool PatternItemView::DisplayedData::isWithinPlayRange() const noexcept
{
    return withinPlayRange;
}

bool PatternItemView::DisplayedData::isWithTransposition() const noexcept
{
    return withTransposition;
}

bool PatternItemView::DisplayedData::isSelected() const noexcept
{
    return selected;
}

bool PatternItemView::DisplayedData::isCurrent() const noexcept
{
    return current;
}

bool PatternItemView::DisplayedData::operator==(const PatternItemView::DisplayedData& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    return (position == rhs.position) &&
           (patternIndex == rhs.patternIndex) &&
           (patternColor == rhs.patternColor) &&
           (withinPlayRange == rhs.withinPlayRange) &&
           (withTransposition == rhs.withTransposition) &&
           (selected == rhs.selected) &&
           (current == rhs.current);
}

bool PatternItemView::DisplayedData::operator!=(const PatternItemView::DisplayedData& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    return !(rhs == *this);
}


// ===============================================

PatternItemView::PatternItemView(juce::DragAndDropContainer& pDragAndDropContainer, Listener& pListener, DisplayedData pDisplayedData) noexcept :
        dragAndDropContainer(pDragAndDropContainer),
        listener(pListener),
        displayedData(pDisplayedData),
        draggedOverLeft(false),
        draggedOverRight(false)
{
}

void PatternItemView::refreshUi(const PatternItemView::DisplayedData& newDisplayedData) noexcept
{
    if (displayedData == newDisplayedData) {
        return;
    }

    displayedData = newDisplayedData;
    repaint();
}


// Component method implementations.
// ===================================

void PatternItemView::paint(juce::Graphics& g)
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto heightFloat = static_cast<float>(height);
    const auto bounds = getLocalBounds();

    const auto patternColor = displayedData.getPatternColor();
    const auto withTransposition = displayedData.isWithTransposition();
    const auto patternIndex = displayedData.getPatternIndex();

    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    // The background.
    const auto selected = displayedData.isSelected();
    auto background = displayedData.isWithinPlayRange()
            ? localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionWithinSongRangeBackground))
            : localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionOutsideSongRangeBackground));
    if (selected) {
        background = background.contrasting(0.4F);
    }
    g.fillAll(background);

    // The border.
    const auto isCurrent = displayedData.isCurrent();
    const auto border = isCurrent
                                    ? localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionCurrentlyPlayedBorder))
                                    : localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionBorder));
    g.setColour(border);
    const auto borderThickness = isCurrent ? 3 : (selected ? 2 : 1);
    g.drawRect(bounds, borderThickness);

    // The swatch.
    const auto swatchBounds = getSwatchBounds();
    g.setColour(patternColor);
    g.fillRect(swatchBounds);
    const auto swatchBorderColor = patternColor.contrasting(0.7F);
    g.setColour(swatchBorderColor);
    g.drawRect(swatchBounds);

    // The pattern number.
    const auto textColor = background.contrasting(1.0F);
    const auto textHeight = heightFloat * 0.8F;
    const auto textY = (heightFloat - textHeight) / 2.0F;
    g.setColour(textColor);
    const auto text = juce::String::toHexString(patternIndex);
    g.setFont(textHeight);
    g.drawText(text, 0, static_cast<int>(textY), width, static_cast<int>(textHeight), juce::Justification::centred, false);

    // The transposition.
    if (withTransposition) {
        constexpr auto transpositionWidth = 8.0F;
        constexpr auto transpositionHeight = 12.0F;
        const auto transpositionX = width - static_cast<int>(transpositionWidth) - 1;
        const auto transpositionY = swatchBounds.getY() - 1;
        g.setFont(transpositionHeight);
        g.drawText("T", transpositionX, transpositionY, static_cast<int>(transpositionWidth),
                   static_cast<int>(transpositionHeight), juce::Justification::topLeft, false);
    }

    // Dragged over?
    if (draggedOverLeft || draggedOverRight) {
        const auto hoverColor = background.contrasting(0.8F);

        // Displays left or right drop location.
        g.setColour(hoverColor);
        const auto x = draggedOverLeft ? 0 : width - draggedOverRectangleSize;
        g.fillRect(x, 0, draggedOverRectangleSize, height);
    }
}

void PatternItemView::mouseDown(const juce::MouseEvent& event)
{
    const auto position = displayedData.getPosition();

    if (event.mods.isLeftButtonDown()) {
        const auto shift = event.mods.isShiftDown();
        const auto control = event.mods.isCtrlDown();
        // Maybe the Swatch was clicked? If yes and no modifiers are used, edit.
        if (!event.mods.isAnyModifierKeyDown() && getSwatchBounds().contains(event.getPosition())) {
            listener.onUserWantsToEditPosition(position);
        } else {
            listener.onPatternItemClicked(displayedData.getPosition(), true, shift, control);
        }
    } else if (event.mods.isRightButtonDown()) {
        listener.onPatternItemRightClicked(displayedData.getPosition());
    }
}

void PatternItemView::mouseUp(const juce::MouseEvent& event)
{
    // If Up after dragging, do NOT send this event, else it will be considered as a new selection.
    if (event.mouseWasDraggedSinceMouseDown()) {
        return;
    }

    if (event.mods.isLeftButtonDown()) {
        const auto shift = event.mods.isShiftDown();
        const auto control = event.mods.isCtrlDown();
        listener.onPatternItemClicked(displayedData.getPosition(), false, shift, control);
    }
}

void PatternItemView::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        listener.onPatternItemDoubleClicked(displayedData.getPosition());
    }
}

void PatternItemView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Shift must be pressed, else ignores this event (this would prevent the ItemsHolder from scrolling).
    if (!event.mods.isShiftDown()) {
        Component::mouseWheelMove(event, wheel);
        return;
    }

    const auto currentStep = (wheel.deltaY < 0.0F) ? 1 : -1;
    listener.onMouseWheelOnPatternItem(displayedData.getPosition(), currentStep);
}

void PatternItemView::mouseDrag(const juce::MouseEvent& event)
{
    if (DragAndDropHelper::isMouseDragInvalid(dragAndDropContainer, event)) {
        return;
    }

    // Changes the cursor, stored if move or duplicate.
    // NOTE: the cursor will be dismissed by the DragAndDrop Container thanks to the CursorChanger marker class.
    const auto duplicate = event.mods.isCtrlDown();
    setMouseCursor(duplicate ? juce::MouseCursor::StandardCursorType::CopyingCursor : juce::MouseCursor::StandardCursorType::DraggingHandCursor);

    // Creates the object to transfer.
    auto* obj = new DraggedPatternItemView(displayedData.getPosition(), duplicate);     // Reference-counted.
    const juce::var sourceDescription(obj);

    dragAndDropContainer.startDragging(sourceDescription, this);
}


// DragAndDropTarget method implementations.
// ===========================================

bool PatternItemView::isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    // Is it a PatternItemView?
    auto* ptr = dragSourceDetails.description.getObject();
    const auto* draggedItem = dynamic_cast<DraggedPatternItemView*>(ptr);
    if (draggedItem == nullptr) {
        return false;
    }

    // Is it the same position? If yes, don't do anything.
    return (draggedItem->getPosition() != displayedData.getPosition());
}

void PatternItemView::itemDropped(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    setDraggedOverAndRepaint(false, false);

    // Is it a PatternItemView? Shouldn't be useful, has been tested before.
    auto* ptr = dragSourceDetails.description.getObject();
    const auto* draggedItem = dynamic_cast<DraggedPatternItemView*>(ptr);
    if (draggedItem == nullptr) {
        jassertfalse;
        return;
    }

    // Notifies the listener.
    listener.onWantToMoveOrDuplicatePatternItemView(draggedItem->getPosition(), displayedData.getPosition(),
                                                    draggedItem->isDuplicated());
}

void PatternItemView::itemDragEnter(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    auto* ptr = dragSourceDetails.description.getObject();
    const auto* draggedItem = dynamic_cast<DraggedPatternItemView*>(ptr);
    if (draggedItem == nullptr) {
        jassertfalse;           //  Abnormal, has been tested before!
        return;
    }

    // If the current position is inferior, we use the left hover, else the right (MPT behavior, good idea).
    auto newDraggedOverLeft = false;
    auto newDraggedOverRight = false;
    if (displayedData.getPosition() <= draggedItem->getPosition()) {
        newDraggedOverLeft = true;
    } else {
        newDraggedOverRight = true;
    }

    setDraggedOverAndRepaint(newDraggedOverLeft, newDraggedOverRight);
}

void PatternItemView::itemDragExit(const DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
{
    setDraggedOverAndRepaint(false, false);
}

void PatternItemView::setDraggedOverAndRepaint(const bool newDraggedOverLeft, const bool newDraggedOverRight) noexcept
{
    if ((newDraggedOverLeft == draggedOverLeft) && (newDraggedOverRight == draggedOverRight)) {
        return;
    }

    draggedOverLeft = newDraggedOverLeft;
    draggedOverRight = newDraggedOverRight;
    repaint();
}

juce::Rectangle<int> PatternItemView::getSwatchBounds() const noexcept
{
    const auto height = getHeight();
    const auto heightFloat = static_cast<float>(height);

    constexpr auto margins = 3;
    const auto colorSwatchHeight = static_cast<int>(heightFloat * 0.2F);
    const auto colorSwatchWidth = colorSwatchHeight;

    const auto swatchY = height - colorSwatchHeight - margins;
    constexpr auto swatchX = margins;

    return { swatchX, swatchY, colorSwatchWidth, colorSwatchHeight };
}

}   // namespace arkostracker
