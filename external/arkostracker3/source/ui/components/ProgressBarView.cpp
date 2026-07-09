#include "ProgressBarView.h"

#include "../../utils/NumberUtil.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ProgressBarView::ProgressBarView() noexcept :
    maximumProgressValue(100),
    progressPercent(0),
    foregroundColor(juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundFocused))
        .brighter(0.4F)),
    textColor(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId))
{
}

void ProgressBarView::setMaximumProgressValue(int newMaximumProgressValue) noexcept
{
    maximumProgressValue = newMaximumProgressValue;
}

void ProgressBarView::setProgressValue(int currentProgressValue) noexcept
{
    currentProgressValue = NumberUtil::correctNumber(currentProgressValue, 0, maximumProgressValue);
    progressPercent = currentProgressValue * 100 / maximumProgressValue;
    repaint();
}


// Component method overrides.
// =====================================================

void ProgressBarView::paint(juce::Graphics& g)
{
    const auto width = getWidth();
    const auto height = getHeight();

    // Draws the outer border.
    g.setColour(foregroundColor);
    g.drawRect(0, 0, width, height, 2);

    // The inner rectangle.
    const auto margins = 4;
    const auto maximumInnerWidth = width - 2 * margins;
    const auto innerWidth = maximumInnerWidth * progressPercent / 100;

    g.fillRect(margins, margins, innerWidth, height - 2 * margins);

    // The text.
    g.setColour(textColor);
    const auto text = juce::String(progressPercent) + "%";
    g.drawFittedText(text, 0, 0, width, height, juce::Justification::centred, 1);
}

}   // namespace arkostracker

