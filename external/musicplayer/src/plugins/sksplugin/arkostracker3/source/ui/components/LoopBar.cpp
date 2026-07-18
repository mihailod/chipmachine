#include "LoopBar.h"

#include "../../utils/NumberUtil.h"

namespace arkostracker 
{

const float LoopBar::markerSize = 14.0F;
const float LoopBar::markerBarHeight = 3.0F;
const int LoopBar::labelsY = 3;
const int LoopBar::labelsHeight = 20;
const float LoopBar::labelsAlphaOutOfBounds = 0.4F;
const float LoopBar::disabledAlpha = 0.3F;
const float LoopBar::hoveredAlpha = 0.2F;
const float LoopBar::hoveredSaturationMultiplier = 0.1F;

const float LoopBar::highlightItemAlpha = 0.4F;
const int LoopBar::highlightHeight = 2;
const int LoopBar::highlightYTop = labelsHeight - highlightHeight;

LoopBar::LoopBar(LoopBar::Listener& pListener, const int pId, const int pItemWidth, const int pEndMargin, const bool pShowIndexes,
    const LookAndFeelConstants::Colors pBarColorId) noexcept :
        listener(pListener),
        id(pId),
        itemWidth(pItemWidth),
        endMargin(pEndMargin),
        showIndexes(pShowIndexes),

        barColorId(pBarColorId),
        barColor(),             // The colors are calculated below.
        barColorDisabled(),
        barColorHovered(),
        highlightItemColor(),

        displayData(DisplayData(false, 0, 0)),      // Dummy data.
        hoveredLeftMarkerIndex(),
        hoveredRightMarkerIndex(),
        highlightedIndexes(),
        labels(),
        startMarkerPath(),
        endMarkerPath(),
        hoveredStartMarkerPath(),
        hoveredEndMarkerPath(),
        horizontalLinePath()
{
    updateColorsFromLookAndFeel();
}

void LoopBar::updateColorsFromLookAndFeel(const bool mustRepaint) noexcept
{
    const auto newBarColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(barColorId));
    const auto newHighlightItemColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId)
            .withAlpha(highlightItemAlpha);
    if ((newBarColor == barColor) && (newHighlightItemColor == highlightItemColor)) {
        return;
    }

    barColor = newBarColor;
    barColorDisabled = newBarColor.withAlpha(disabledAlpha);
    barColorHovered = newBarColor.withAlpha(hoveredAlpha).withMultipliedSaturation(hoveredSaturationMultiplier);

    highlightItemColor = newHighlightItemColor;

    if (mustRepaint) {
        repaint();
    }
}

void LoopBar::setDisplayData(const LoopBar::DisplayData& newDisplayData) noexcept
{
    // Any change?
    if (displayData == newDisplayData) {
        return;
    }

    displayData = newDisplayData;

    repaint();
    updateLabelsAlpha();
}

void LoopBar::setItemWidth(const int newItemWidth) noexcept
{
    if (itemWidth == newItemWidth) {
        return;
    }
    itemWidth = newItemWidth;

    locateAndResizeItems();
}

void LoopBar::setHighlightedIndexes(const std::vector<int>& indexes) noexcept
{
    if (highlightedIndexes == indexes) {
        return;
    }
    highlightedIndexes = indexes;

    // Repaints only the lower part of the labels.
    repaint(0, getHeight() - highlightHeight, getWidth(), highlightHeight);
}


// Component method implementations.
// ====================================

void LoopBar::lookAndFeelChanged()
{
    updateColorsFromLookAndFeel(true);
}

void LoopBar::resized()
{
    // Nothing to do if we don't want any label to display.
    // The bar itself is managed in the paint method, which is automatically called.
    if (!showIndexes) {
        return;
    }

    const auto width = getWidth();

    // How many labels can we fit now?
    const auto newCount = width / itemWidth;
    const auto currentCount = static_cast<int>(labels.size());

    if (newCount < currentCount) {
        // Shrinks.
        labels.resize(static_cast<size_t>(newCount));
    } else if (newCount > currentCount) {
        // Creates new labels.
        const auto baseIndex = static_cast<int>(labels.size());
        const auto endIndex = displayData.getEndIndex();
        for (auto i = 0, countToCreate = newCount - currentCount; i < countToCreate; ++i) {
            const auto index = baseIndex + i;
            const auto text = NumberUtil::toUnsignedHex(index);
            auto label = std::make_unique<juce::Label>(juce::String(), text);
            label->setJustificationType(juce::Justification::horizontallyCentred);
            label->setInterceptsMouseClicks(false, false);  // Else, the hover doesn't work.
            setLabelAlpha(*label, index, endIndex);
            label->setMinimumHorizontalScale(0.1F);      // Allows horizontal squashing.
            label->setFont(juce::Font(LookAndFeelConstants::typefaceNameSquare, markerSize, juce::Font::FontStyleFlags::plain));

            // The views are resized and positioned below.
            addAndMakeVisible(*label);

            labels.push_back(std::move(label));
        }

        locateAndResizeItems();
    }
}

void LoopBar::paint(juce::Graphics& g)
{
    // Displays the loop bar.
    // Where are the items?
    const auto startIndex = displayData.getStartIndex();
    const auto endIndex = displayData.getEndIndex();

    const auto startMarkerXAndXEnd = calculateMarkerPosition(startIndex, true);
    const auto endMarkerXAndXEnd = calculateMarkerPosition(endIndex, false);

    constexpr auto yTop = 0.0F;
    const auto yBottom = yTop + markerSize;

    // Draws the possible hovered markers, unless a "real" marker already on it.
    g.setColour(barColorHovered);
    if (hoveredLeftMarkerIndex.isPresent()) {
        const auto hoveredLeftMarkerIndexValue = hoveredLeftMarkerIndex.getValue();
        if (hoveredLeftMarkerIndexValue != startIndex) {
            const auto xAndXEnd = calculateMarkerPosition(hoveredLeftMarkerIndexValue, true);
            drawMarker(g, hoveredStartMarkerPath, xAndXEnd, yBottom, true);
        }
    }
    if (hoveredRightMarkerIndex.isPresent()) {
        const auto hoveredRightMarkerIndexValue = hoveredRightMarkerIndex.getValue();
        if (hoveredRightMarkerIndexValue != endIndex) {
            const auto xAndXEnd = calculateMarkerPosition(hoveredRightMarkerIndexValue, false);
            drawMarker(g, hoveredEndMarkerPath, xAndXEnd, yBottom, false);
        }
    }

    // Sets the marker color.
    const auto markerColor = displayData.isLoopEnabled() ? barColor : barColorDisabled;
    g.setColour(markerColor);

    // Draws the markers.
    drawMarker(g, startMarkerPath, startMarkerXAndXEnd, yBottom, true);
    drawMarker(g, endMarkerPath, endMarkerXAndXEnd, yBottom, false);
    // ...and the horizontal line. Takes care to follow the "curve" of the triangle, else, when disabled, alpha clash makes it whole ugly.
    constexpr auto horizontalLineTop = yTop;
    const auto horizontalLineBottom = yTop + markerBarHeight;
    constexpr auto correction = 0.22F;          // Not perfect... Cannot "join" the markers and the horizontal line without a small glitch. Oh, well.
    const auto horizontalLineTopLeft = startMarkerXAndXEnd.second - correction;
    const auto horizontalLineTopRight = endMarkerXAndXEnd.first + correction;
    const auto horizontalLineBottomRight = horizontalLineTopRight + markerBarHeight;
    const auto horizontalLineBottomLeft = horizontalLineTopLeft - markerBarHeight;
    horizontalLinePath.clear();
    horizontalLinePath.startNewSubPath(horizontalLineTopLeft, horizontalLineTop);
    horizontalLinePath.lineTo(horizontalLineTopRight, horizontalLineTop);
    horizontalLinePath.lineTo(horizontalLineBottomRight, horizontalLineBottom);
    horizontalLinePath.lineTo(horizontalLineBottomLeft, horizontalLineBottom);
    horizontalLinePath.closeSubPath();
    g.fillPath(horizontalLinePath);

    // Displays the highlight under the label.
    g.setColour(highlightItemColor);
    for (const auto highlightedIndex : highlightedIndexes) {
        const auto [highlightXStart, _] = calculateMarkerPosition(highlightedIndex, true);
        const auto [_2, highlightXEnd] = calculateMarkerPosition(highlightedIndex, false);

        g.fillRect(static_cast<int>(highlightXStart), highlightYTop, static_cast<int>(highlightXEnd - highlightXStart), highlightHeight);
    }
}

void LoopBar::mouseMove(const juce::MouseEvent& event)
{
    // Any new hovered marker?
    const auto hoveredIndexOptionalAndIsStartMarker = getHoveredIndex(event);
    if (hoveredIndexOptionalAndIsStartMarker.first.isAbsent()) {
        // Nothing hovered.
        // Was anything drawn? If yes, cancels.
        if (hoveredLeftMarkerIndex.isPresent() || hoveredRightMarkerIndex.isPresent()) {
            hoveredLeftMarkerIndex = OptionalInt();
            hoveredRightMarkerIndex = OptionalInt();
            repaint();
        }
        return;
    }

    // A marker is hovered.
    const auto hoveredIndex = hoveredIndexOptionalAndIsStartMarker.first.getValue();
    const auto isStartMarkerHovered = hoveredIndexOptionalAndIsStartMarker.second;

    auto mustRepaint = false;

    if (isStartMarkerHovered) {
        // Hovers over the left marker. Is it over another (or new?) index? If yes, worth a repaint.
        const auto newHoveredLeftMarkerIndex = OptionalInt(hoveredIndex);
        if (hoveredLeftMarkerIndex != newHoveredLeftMarkerIndex) {
            hoveredLeftMarkerIndex = newHoveredLeftMarkerIndex;
            hoveredRightMarkerIndex = OptionalInt();            // Cancels the other marker!
            mustRepaint = true;
        }
    } else {
        // The same for the right item.
        const auto newHoveredRightMarkerIndex = OptionalInt(hoveredIndex);
        if (hoveredRightMarkerIndex != newHoveredRightMarkerIndex) {
            hoveredRightMarkerIndex = newHoveredRightMarkerIndex;
            hoveredLeftMarkerIndex = OptionalInt();             // Cancels the other marker!
            mustRepaint = true;
        }
    }

    if (mustRepaint) {
        repaint();
    }
}

void LoopBar::mouseExit(const juce::MouseEvent& /*event*/)
{
    hoveredLeftMarkerIndex = OptionalInt();
    hoveredRightMarkerIndex = OptionalInt();
    repaint();
}

void LoopBar::mouseDown(const juce::MouseEvent& event)
{
    // If right-click (anywhere), toggles the loop.
    if (event.mods.isRightButtonDown()) {
        listener.onUserWantsToToggleLoop(id);
        return;
    }

    // Left click.
    // Any hovered marker?
    const auto hoveredIndexOptionalAndIsStartMarker = getHoveredIndex(event);
    if (hoveredIndexOptionalAndIsStartMarker.first.isAbsent()) {
        // Nothing hovered.
        return;
    }

    const auto hoveredIndex = hoveredIndexOptionalAndIsStartMarker.first.getValue();
    const auto isStartMarkerHovered = hoveredIndexOptionalAndIsStartMarker.second;

    if (isStartMarkerHovered) {
        listener.onUserWantsToSetLoopBarStart(id, hoveredIndex);
    } else {
        listener.onUserWantsToSetLoopBarEnd(id, hoveredIndex);
    }
}


// ====================================

std::pair<float, float> LoopBar::calculateMarkerPosition(const int index, const bool isStartMarker) const noexcept
{
    const auto itemWidthFloat = static_cast<float>(itemWidth);

    const auto leftX = isStartMarker ? (static_cast<float>(index) * itemWidthFloat) :
            ((static_cast<float>(index) * itemWidthFloat) + itemWidthFloat - markerSize - static_cast<float>(endMargin));
    const auto rightX = leftX + markerSize;

    return { leftX, rightX };
}

void LoopBar::drawMarker(const juce::Graphics& g, juce::Path& path, const std::pair<float, float>& xStartAndXEnd, const float yBottom, const bool isStartMarker) noexcept
{
    constexpr auto yTop = 0.0F;

    path.clear();
    if (isStartMarker) {
        path.addTriangle(xStartAndXEnd.first, yTop, xStartAndXEnd.second, yTop, xStartAndXEnd.first, yBottom);
    } else {
        path.addTriangle(xStartAndXEnd.first, yTop, xStartAndXEnd.second, yTop, xStartAndXEnd.second, yBottom);
    }
    g.fillPath(path);
}

void LoopBar::updateLabelsAlpha() const noexcept
{
    const auto endIndex = displayData.getEndIndex();

    auto index = 0;
    for (const auto& label : labels) {
        setLabelAlpha(*label, index, endIndex);

        ++index;
    }
}

void LoopBar::setLabelAlpha(juce::Label& label, const int index, const int endIndex) noexcept
{
    const auto alpha = (index <= endIndex) ? 1.0F : labelsAlphaOutOfBounds;
    label.setAlpha(alpha);
}

std::pair<OptionalInt, bool> LoopBar::getHoveredIndex(const juce::MouseEvent& event) const noexcept
{
    const auto mouseX = event.x;
    const auto widthWithoutMargin = itemWidth - endMargin;

    const auto hoveredIndex = mouseX / itemWidth;

    const auto xInItem = mouseX % itemWidth;
    // If beyond the "end", nothing is hovered.
    if (xInItem >= widthWithoutMargin) {
        return { { }, false };
    }

    const auto halfWidth = widthWithoutMargin / 2;

    // Left or right hovered?
    auto isLeftMarkerHovered = (xInItem < halfWidth);

    return { hoveredIndex, isLeftMarkerHovered };
}

void LoopBar::locateAndResizeItems() noexcept
{
    auto index = 0;
    for (const auto& label : labels) {
        label->setBounds(index * itemWidth, labelsY, itemWidth - endMargin, labelsHeight);
        ++index;
    }

    repaint();      // For the markers to be updated.
}

}   // namespace arkostracker
