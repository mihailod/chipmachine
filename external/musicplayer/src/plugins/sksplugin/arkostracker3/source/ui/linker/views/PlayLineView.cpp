#include "PlayLineView.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int PlayLineView::playLineHeight = 14;
const float PlayLineView::cursorWidth = 10.0F;
const float PlayLineView::dashLengths[] = { 4.0F, 4.0F }; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays)

PlayLineView::PlayLineView(int pItemWidth) noexcept :
    itemWidth(pItemWidth),
    displayedData(DisplayedData(0, 0, 0, 0, 0, 0)),
    lineColor(findLineColor())
{
}

void PlayLineView::refreshUi(const DisplayedData& pDisplayedData) noexcept
{
    // Any change?
    if (displayedData == pDisplayedData) {
        return;
    }
    displayedData = pDisplayedData;

    const auto newWidth = getLineWidth() + static_cast<int>(cursorWidth);   // To allow the cursor to go beyond (before looping).
    setSize(newWidth, playLineHeight);
    repaint();      // Not done by setSize if it does not change.
}

juce::Colour PlayLineView::findLineColor() noexcept
{
    auto& defaultLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    return defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPlayCursorTimeline));
}

int PlayLineView::getLineWidth() const noexcept
{
    return itemWidth * displayedData.getPositionCount();
}

// Component method implementations.
// ======================================

void PlayLineView::paint(juce::Graphics& g)
{
    const auto height = getHeight();
    const auto middleY = static_cast<float>(height) / 2.0F;

    // Draws the dashed line.
    g.setColour(lineColor.withAlpha(0.5F));
    const auto lineWidth = static_cast<float>(getLineWidth());
    g.drawDashedLine(juce::Line(0.0F, middleY, lineWidth, middleY), &dashLengths[0], 2);

    // Draws the "normal" line.
    g.setColour(lineColor);
    const auto lineThickness = 2.0F;
    const auto halfLineThickness = lineThickness / 2.0F;        // To compensate, else the cursor seems displayed besides the start/end small lines.
    const auto lineStartX = static_cast<float>(itemWidth * displayedData.getPlayStartPosition());
    const auto lineEndX = static_cast<float>(itemWidth * (displayedData.getEndPosition() + 1) - 1);
    g.drawLine(lineStartX, middleY, lineEndX, middleY, lineThickness);

    // And its small vertical lines.
    const auto markerHalfHeight = 4.0F;
    g.drawLine(lineStartX + halfLineThickness, middleY - markerHalfHeight, lineStartX + halfLineThickness, middleY + markerHalfHeight, lineThickness);
    g.drawLine(lineEndX - halfLineThickness, middleY - markerHalfHeight, lineEndX - halfLineThickness, middleY + markerHalfHeight, lineThickness);

    // Draws the cursor.
    auto cursorX = static_cast<float>(itemWidth * displayedData.getCurrentPosition());
    const auto currentPatternHeight = displayedData.getCurrentPatternHeight();
    if (currentPatternHeight > 1) {
        cursorX += (static_cast<float>(itemWidth) - halfLineThickness)
                * static_cast<float>(displayedData.getCurrentLine()) / static_cast<float>(displayedData.getCurrentPatternHeight() - 1);
    }
    cursorX = std::min(cursorX, lineEndX - lineThickness);       // Else, doesn't look good, the cursor can go beyond the end line.

    juce::Path path;
    path.addTriangle(cursorX, 0.0F, cursorX + static_cast<float>(cursorWidth), middleY, cursorX, static_cast<float>(playLineHeight));
    g.fillPath(path);
}

void PlayLineView::lookAndFeelChanged()
{
    lineColor = findLineColor();
    repaint();
}


}   // namespace arkostracker

