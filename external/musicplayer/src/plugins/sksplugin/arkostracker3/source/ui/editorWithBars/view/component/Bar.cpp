#include "Bar.h"

#include <BinaryData.h>

#include "../../../../utils/NumberUtil.h"

namespace arkostracker 
{

const float Bar::colorContrastRatioHover = 0.1F;
const float Bar::colorContrastRatioBackgroundHover = 0.01F;
const float Bar::colorContrastRatioSelected = 0.7F;
const float Bar::generatedWidthRatio = 0.5F;
const float Bar::outsideLoopAlphaMultiplier = 0.7F;
const float Bar::ignoredAlphaMultiplier = 0.1F;
const float Bar::generatedAlphaMultiplier = 0.6F;

Bar::Bar(const int pBarIndex) noexcept :
        barIndex(pBarIndex),
        displayedData(),
        baseBarColor(juce::Colours::black),
        baseBarBackgroundColor(juce::Colours::black),
        displayRange(1),
        displayRatio(1.0F),
        topTrianglePath(),
        pathBackground()
{
}

void Bar::setDisplayedData(const BarData& newBarData) noexcept
{
    // Anything to do?
    if (displayedData == newBarData) {
        return;
    }

    displayedData = newBarData;
    displayRange = displayedData.getMaximumValue() - displayedData.getMinimumValue() + 1;     // +1 because min/max are not excluded!
    calculateDisplayRatio();

    updateColorsFromCurrentData();

    repaint();
}

int Bar::getIndex() const noexcept
{
    return barIndex;
}


// Components method implementations.
// ======================================

void Bar::resized()
{
    calculateDisplayRatio();

    // Creates the out-of-bounds triangle path.
    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());

    constexpr auto triangleHeight = 10.0F;
    const auto triangleWidth = std::min(triangleHeight * 1.5F, width);  // Slightly larger base, looks better.

    const auto middle = width / 2.0F;
    const auto left = middle - (triangleWidth / 2.0F);
    const auto right = middle + (triangleWidth / 2.0F);

    // Pre-computes the drawing paths.
    topTrianglePath.clear();
    topTrianglePath.addTriangle(middle, 0.0F, right, triangleHeight, left, triangleHeight);

    pathBackground.clear();
    pathBackground.addRectangle(0.0F, 0.0F, width, height);
}

void Bar::lookAndFeelChanged()
{
    // Any new colors?
    if (updateColorsFromCurrentData()) {
        repaint();
    }
}

void Bar::paint(juce::Graphics& g)
{
    const auto widthFloat = static_cast<float>(getWidth());

    constexpr auto smallWidthLimit = 20.0F;     // Arbitrary.
    const auto smallWidth = widthFloat < smallWidthLimit;

    constexpr auto barCornerSize = 0;       //smallWidth ? 3.0F : 5.0F;

    const auto displayedValue = calculateBarTopAndHeight();

    const auto outOfBounds = displayedData.isOutOfBounds();
    const auto withinLoop = displayedData.isWithinLoop();
    const auto ignored = displayedData.isIgnored();
    const auto outOfVisibleMinMax = displayedValue.isOutOfVisibleMinMax();
    const auto generated = displayedData.isGenerated();

    // Draws the background.
    auto backgroundColor = baseBarBackgroundColor;
    if (ignored || outOfBounds || !withinLoop || generated) {
        backgroundColor = backgroundColor.withMultipliedAlpha(ignoredAlphaMultiplier);
    }
    const auto hovered = displayedData.isHovered();
    if (hovered) {
        backgroundColor = backgroundColor.contrasting(colorContrastRatioBackgroundHover);
    }
    g.setColour(backgroundColor);
    g.fillPath(pathBackground);

    // If out of bounds, don't do anything else (except showing the cursor).
    if (outOfBounds) {
        displayCursor(g);
        return;
    }

    // Draws the bar.
    auto fillColor = baseBarColor;
    // Beyond visible limit? If beyond, replaces the color.
    const auto outOfVisibleMinMaxColor = juce::Colours::red;
    if (outOfVisibleMinMax) {
        fillColor = outOfVisibleMinMaxColor;
    }

    const auto selected = displayedData.isSelected();

    if (selected) {
        fillColor = fillColor.contrasting(colorContrastRatioSelected);
    }
    if (hovered) {
        fillColor = fillColor.contrasting(colorContrastRatioHover);
    }
    if (!withinLoop) {
        fillColor = fillColor.withAlpha(outsideLoopAlphaMultiplier);
    }
    if (ignored) {
        fillColor = fillColor.withMultipliedAlpha(ignoredAlphaMultiplier);
    }
    if (generated) {
        fillColor = fillColor.withMultipliedAlpha(generatedAlphaMultiplier);
    }

    // Draws the bar, with a contour around.
    // If generated, draws "thinner" bars.
    const auto barWidth = generated ? widthFloat * generatedWidthRatio : widthFloat;
    const auto barX = (widthFloat - barWidth) / 2.0F;
    const auto path = juce::Rectangle(barX, static_cast<float>(displayedValue.getTop()), barWidth, static_cast<float>(displayedValue.getHeight()));

    g.setColour(fillColor);
    g.fillRoundedRectangle(path, static_cast<float>(barCornerSize));

    // Only shows the contour in 'normal' conditions.
    if (/*!hovered &&*/ !selected) {
        const auto strokeColor = (ignored || !withinLoop) ?
            fillColor : fillColor.brighter(2.0F);
        g.setColour(strokeColor);
        g.drawRect(path, 1.0F);
    }

    const auto isRetrig = displayedData.isRetrig();
    // Displays the triangle at the top to indicate "out of bounds". If Retrig, too cluttered, don't show it.
    if (outOfVisibleMinMax && !isRetrig) {
        g.setColour(outOfVisibleMinMaxColor.contrasting(0.9F));
        g.fillPath(topTrianglePath);
    }

    // Shows the "R" at the top-right, if retrig.
    if (isRetrig) {
        const auto& image = getRetrigImage();
        const auto margin = smallWidth ? 2.0F : 5.0F;

        const auto imageX = widthFloat - margin - static_cast<float>(image.getWidth());
        const auto imageY = margin;

        g.drawImageAt(image, static_cast<int>(imageX), static_cast<int>(imageY));
    }

    displayCursor(g);
}

void Bar::displayCursor(juce::Graphics& g) const noexcept
{
    if (displayedData.hasCursor()) {
        const auto color = baseBarColor.contrasting(0.7F);
        g.setColour(color);
        g.drawRect(0, 0, getWidth(), getHeight(), 2);
    }
}

void Bar::calculateDisplayRatio() noexcept
{
    const auto yRange = getHeight() - 1;
    displayRatio = static_cast<double>(yRange) / static_cast<double>(this->displayRange);
}

int Bar::determineValue(int y) const noexcept
{
    const auto height = getHeight();

    const auto minimumValue = displayedData.getMinimumValue();
    const auto maximumValue = displayedData.getMaximumValue();

    // Try to correct if below 0.
    const auto y0 = static_cast<double>(height) - ((0.0 - static_cast<double>(minimumValue)) * displayRatio);
    if (static_cast<double>(y) > y0) {
        y += static_cast<int>(displayRatio);
    }

    const auto valueDouble = (static_cast<double>((height - 1 - y)) / displayRatio + static_cast<double>(minimumValue));
    //DBG("   y = " + juce::String(y) + ", value: " + juce::String(valueDouble));
    const auto rawValue = static_cast<int>(valueDouble);

    // Corrects the value, required because the Y can be out of limits (especially when dragging).
    return NumberUtil::correctNumber(rawValue, minimumValue, maximumValue);
}

int Bar::getCurrentValue() const noexcept
{
    return displayedData.getValue();
}

Bar::DisplayedValue Bar::calculateBarTopAndHeight() const noexcept
{
    const auto height = getHeight();
    const auto heightDouble = static_cast<double>(height);

    const auto shortBar = displayedData.isShortBar();
    auto currentValue = displayedData.getValue();
    const auto minimumValue = displayedData.getMinimumValue();
    const auto maximumValue = displayedData.getMaximumValue();
    // Is the value out of bounds? If yes, shows the value as a limit.
    auto isOutOfVisibleBounds = false;
    if (currentValue < minimumValue) {
        currentValue = minimumValue;
        isOutOfVisibleBounds = true;
    } else if (currentValue > maximumValue) {
        currentValue = maximumValue;
        isOutOfVisibleBounds = true;
    }

    double yValue; // NOLINT(*-init-variables)
    if (currentValue == maximumValue) {
        yValue = 0.0;         // Corner case: if we are at the maximum, reach the top! Else, approximation makes it "near" it, ugly.
    } else if ((currentValue < 0) && !shortBar) {       // 0 is shown with a bar, so <0 values must be compensated for the -1 to be visible.
        yValue = heightDouble - (static_cast<double>(currentValue - minimumValue) * displayRatio);
    } else {
        yValue = heightDouble - (static_cast<double>(currentValue + 1 - minimumValue) * displayRatio);        // +1 because there is always one more value (0->15 = 16 values).
    }

    // Another trick to remove an approximation, that would make the "bottom" not be the bottom.
    if (currentValue >= 0) {
        yValue = std::floor(yValue);
    }

    double yBottom; // NOLINT(*-init-variables)
    if (shortBar) {
        // For short bars, simply shows a small block of lines.
        yBottom = yValue + displayRatio;
    } else {
        yBottom = heightDouble - (static_cast<double>(0 - minimumValue) * displayRatio);
    }

    auto barHeight = std::abs(yBottom - yValue);
    if ((currentValue >= 0) && !shortBar) {
        barHeight = std::max(1.0, std::floor(barHeight));       // Min height to 1, else a value of 1 in large height (Pitch ALL for ex) will not be seen.
    } else {
        // Allows to have the same height as a positive value in this condition.
        // Also, for short bar, allows to have a bigger bar.
        barHeight = std::ceil(barHeight);
    }

    const auto barTopY = static_cast<int>(std::min(yBottom, yValue));

    return { barTopY, static_cast<int>(barHeight), isOutOfVisibleBounds };
}

const juce::Image& Bar::getRetrigImage() noexcept
{
    // Use "magic static" to build only one image instance.
    static const auto retrigImage = juce::ImageFileFormat::loadFrom(BinaryData::RetrigInBars_png, BinaryData::RetrigInBars_pngSize);

    return retrigImage;
}

bool Bar::updateColorsFromCurrentData() noexcept
{
    // New colors?
    const auto newBarBackgroundColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(displayedData.getBarBackgroundColorId()));
    const auto newBarColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(displayedData.getBarColorId()));

    if ((newBarColor == baseBarColor) && (newBarBackgroundColor == baseBarBackgroundColor)) {
        return false;
    }

    baseBarColor = newBarColor;
    baseBarBackgroundColor = newBarBackgroundColor;

    return true;
}

}   // namespace arkostracker
