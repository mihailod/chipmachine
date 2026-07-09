#include "CaptionedBar.h"

#include "AllBarsAreaConstants.h"

namespace arkostracker 
{

CaptionedBar::CaptionedBar(const int pBarIndex) noexcept :
        bar(pBarIndex),
        caption()
{
    addAndMakeVisible(bar);
    addAndMakeVisible(caption);
}

void CaptionedBar::setDisplayedData(const BarData& barData, const BarCaptionDisplayedData& captionDisplayData) noexcept
{
    bar.setDisplayedData(barData);
    caption.setDisplayedData(captionDisplayData);
}

int CaptionedBar::getIndex() const noexcept
{
    return bar.getIndex();
}

int CaptionedBar::getCurrentValue() const noexcept
{
    return bar.getCurrentValue();
}

int CaptionedBar::determineValue(const int y) const noexcept
{
    return bar.determineValue(y);
}


// Component method implementations.
// ====================================

void CaptionedBar::resized()
{
    constexpr auto captionHorizontalBorder = 0;
    constexpr auto captionBottomBorder = 0;

    const auto width = getWidth();
    const auto height = getHeight();

    const auto barHeight = height;
    const auto captionWidth = width - 2 * captionHorizontalBorder;
    constexpr auto captionHeight = AllBarsAreaConstants::captionHeight;
    constexpr auto captionX = captionHorizontalBorder;

    // The caption height is static. The caption is at the bottom of the bar.
    bar.setBounds(0, 0, width, barHeight);
    caption.setBounds(captionX, barHeight - captionHeight - captionBottomBorder, captionWidth, captionHeight);
}

}   // namespace arkostracker
