#include "ItemsHolder.h"

#include "BinaryData.h"

namespace arkostracker 
{

const int ItemsHolder::markerY = 0;
const int ItemsHolder::positionNumberY = 20;
const int ItemsHolder::positionNumberHeight = 20;
const int ItemsHolder::loopBarHeight = positionNumberHeight;
const int ItemsHolder::patternItemY = positionNumberY + positionNumberHeight;
const int ItemsHolder::patternItemWidth = 45;
const int ItemsHolder::patternItemHeight = patternItemWidth;
const int ItemsHolder::separatorWidth = 12;
const int ItemsHolder::itemWidth = patternItemWidth + separatorWidth;
const int ItemsHolder::separatorBetweenItemsAndCursorLine = 4;

ItemsHolder::ItemsHolder(ScrollListener& pScrollListener, PatternItemView::Listener& pPatternItemListener, LoopBar::Listener& loopBarListener) noexcept :
        horizontalMargin(LookAndFeelConstants::margins),
        mouseListener(*this),
        scrollListener(pScrollListener),
        patternItemListener(pPatternItemListener),
        patternItemViews(),
        loopBar(loopBarListener, 0, itemWidth, separatorWidth, true, LookAndFeelConstants::Colors::loopStartEnd),
        playLineView(itemWidth),
        cachedPositionToMarker(),
        positionToMarkerView(),
        addNewPositionImage(BinaryData::LinkerNew_png, static_cast<size_t>(BinaryData::LinkerNew_pngSize), juce::Label::ColourIds::textColourId),
        addRepeatPositionImage(BinaryData::LinkerRepeat_png, static_cast<size_t>(BinaryData::LinkerRepeat_pngSize), juce::Label::ColourIds::textColourId),
        hoveredPositionIndex()
{
    addAndMakeVisible(loopBar);
    addAndMakeVisible(playLineView);

    // The "hovered" images are set, but not displayed yet.
    const auto imageWidth = addNewPositionImage.getImageWidth();
    const auto imageHeight = addNewPositionImage.getImageHeight();
    addNewPositionImage.setSize(imageWidth, imageHeight);
    addRepeatPositionImage.setSize(imageWidth, imageHeight);
    jassert(imageWidth == addRepeatPositionImage.getImageWidth());      // The images should be the same size.
    jassert(imageHeight == addRepeatPositionImage.getImageHeight());      // The images should be the same size.
    addChildComponent(addNewPositionImage);     // Added but not visible at first.
    addChildComponent(addRepeatPositionImage);

    addNewPositionImage.onClick = [&] (const bool isLeftButtonClicked) { onAddNewPositionClicked(isLeftButtonClicked); };
    addRepeatPositionImage.onClick = [&] (const bool isLeftButtonClicked) { onRepeatPositionClicked(isLeftButtonClicked); };

    addMouseListener(&mouseListener, true);
}

void ItemsHolder::setDisplayedData(const std::vector<PatternItemView::DisplayedData>& patternData, const LoopBar::DisplayData& loopData,
                                   const PlayLineView::DisplayedData& playLineData, const std::unordered_map<int, juce::String>& positionToMarker) noexcept
{
    // Enough Views? Reduces their size if needed, or reserves for more if some are missing.
    const auto newItemCount = patternData.size();
    if (patternItemViews.size() > newItemCount) {
        patternItemViews.resize(newItemCount);
    } else if (patternItemViews.size() < newItemCount) {
        patternItemViews.reserve(newItemCount);
    }

    // Fills each view, creating new ones if needed.
    for (size_t position = 0U; position < newItemCount; ++position) {
        const auto& displayedData = patternData.at(position);
        if (position < patternItemViews.size()) {
            // This View already exists. Simply updates its content.
            patternItemViews.at(position)->refreshUi(displayedData);
        } else {
            // Creates a new view.
            const auto viewX = getItemXAndWidth(static_cast<int>(position)).first;
            constexpr auto viewWidth = patternItemWidth;
            auto patternItemView = std::make_unique<PatternItemView>(*this, patternItemListener, displayedData);
            patternItemView->setBounds(viewX, patternItemY, viewWidth, patternItemHeight);
            addAndMakeVisible(*patternItemView);
            patternItemViews.push_back(std::move(patternItemView));
        }
    }

    const auto fullWidth = (static_cast<int>(patternItemViews.size()) * itemWidth) + (horizontalMargin * 2);
    setSize(fullWidth, getHeight());

    // Sets the loop data.
    loopBar.setBounds(horizontalMargin, positionNumberY, fullWidth, loopBarHeight);
    loopBar.setDisplayData(loopData);

    // Sets the line view. Its size is set by itself.
    playLineView.setTopLeftPosition(horizontalMargin, patternItemY + patternItemHeight + separatorBetweenItemsAndCursorLine);
    playLineView.refreshUi(playLineData);

    displayMarkers(positionToMarker);
}

std::pair<int, int> ItemsHolder::getItemXAndWidth(const int position) const noexcept
{
    return { (position * itemWidth) + horizontalMargin, itemWidth };
}

void ItemsHolder::displayMarkers(const std::unordered_map<int, juce::String>& positionToMarker) noexcept
{
    // Any change?
    if (cachedPositionToMarker == positionToMarker) {
        return;
    }
    cachedPositionToMarker = positionToMarker;

    // Very raw, but should be enough. Clears and recreates everything!
    positionToMarkerView.clear();
    for (const auto& [positionIndex, markerName] : positionToMarker) {
        const auto markerX = getItemXAndWidth(positionIndex).first;

        auto markerView = std::make_unique<MarkerView>(markerName);
        markerView->setTopLeftPosition(markerX, markerY);
        addAndMakeVisible(*markerView);

        positionToMarkerView.insert({ positionIndex, std::move(markerView) });
    }
}


// juce::DragAndDropTarget method implementations.
// =================================================

bool ItemsHolder::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
{
    return true;        // Only interested to get the Drag Move event.
}

void ItemsHolder::itemDropped(const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
{
    // Nothing to do.
}

void ItemsHolder::itemDragMove(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    // beginDragAutoRepeat does not seem to work fine :(. It works fine but stops after a while.

    const auto dragX = dragSourceDetails.localPosition.getX();
    scrollListener.manageScrollingIfNeeded(dragX);
}


// Component method implementations.
// =================================================

void ItemsHolder::onMouseHovered(const juce::MouseEvent& event) noexcept
{
    // Try to determine on what position must be shown the floating icons (new, clone).
    if (patternItemViews.empty()) {
        return;         // Shouldn't happen, but security.
    }

    const auto& firstPatternItemView = *patternItemViews.at(0U);
    const auto leftX = firstPatternItemView.getX();

    const auto mousePosition = event.getPosition();
    const auto mousePositionX = mousePosition.getX();
    const auto mousePositionY = mousePosition.getY();

    const auto positionIndex = (mousePositionX - leftX) / itemWidth;

    constexpr auto patternItemsYTop = patternItemY;
    constexpr auto patternItemsYBottom = patternItemY + patternItemHeight - 1;
    // If out of bounds horizontally or vertically, makes it disappear.
    if ((mousePositionX < firstPatternItemView.getX()) || (positionIndex >= static_cast<int>(patternItemViews.size()))
        || (mousePositionY < patternItemsYTop) || (mousePositionY > patternItemsYBottom)) {
        hideHoveredImages();
        return;
    }

    // Same hovered position? If yes, stops.
    if (hoveredPositionIndex == positionIndex) {
        return;
    }
    hoveredPositionIndex = positionIndex;

    constexpr auto patternItemsYMiddle = (patternItemsYTop + patternItemsYBottom) / 2;
    constexpr auto yIconsSpread = 2;
    const auto iconsWidth = addNewPositionImage.getWidth();
    const auto iconsHeight = addNewPositionImage.getHeight();
    const auto iconsX = (positionIndex * itemWidth) + leftX + itemWidth - iconsWidth;
    const auto iconNewY = patternItemsYMiddle - yIconsSpread - iconsHeight;
    constexpr auto iconRepeatY = patternItemsYMiddle + yIconsSpread;

    addNewPositionImage.setTopLeftPosition(iconsX, iconNewY);
    addRepeatPositionImage.setTopLeftPosition(iconsX, iconRepeatY);

    addNewPositionImage.setVisible(true);
    addRepeatPositionImage.setVisible(true);
    addNewPositionImage.toFront(false);
    addRepeatPositionImage.toFront(false);
}

void ItemsHolder::onMouseExited() noexcept
{
    if (hoveredPositionIndex.isAbsent()) {
        return;
    }
    hideHoveredImages();
}

void ItemsHolder::hideHoveredImages() noexcept
{
    hoveredPositionIndex = { };
    addNewPositionImage.setVisible(false);
    addRepeatPositionImage.setVisible(false);
}

void ItemsHolder::onAddNewPositionClicked(const bool isLeftButtonClicked) const noexcept
{
    if (hoveredPositionIndex.isAbsent()) {
        jassertfalse;
        return;
    }

    const auto positionIndex = hoveredPositionIndex.getValue();
    if (isLeftButtonClicked) {
        patternItemListener.onAddNewPositionClicked(positionIndex);
    } else {
        patternItemListener.onClonePositionClicked(positionIndex);
    }
}

void ItemsHolder::onRepeatPositionClicked(const bool /*isLeftButtonClicked*/) const noexcept
{
    if (hoveredPositionIndex.isAbsent()) {
        jassertfalse;
        return;
    }

    patternItemListener.onRepeatPositionClicked(hoveredPositionIndex.getValue());
}

}   // namespace arkostracker
