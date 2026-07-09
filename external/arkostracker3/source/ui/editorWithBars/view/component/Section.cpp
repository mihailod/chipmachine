#include "Section.h"

namespace arkostracker 
{

const int Section::separatorHeight = 2;
const float Section::separatorAlpha = 0.7F;

Section::Section(const juce::String& text) noexcept :
        label(text),
        separatorLeft(juce::Label::ColourIds::textColourId),
        separatorRight(juce::Label::ColourIds::textColourId)
{
    addAndMakeVisible(label);
    addAndMakeVisible(separatorLeft);
    addAndMakeVisible(separatorRight);

    separatorLeft.setTransformationOnColour([](const juce::Colour color) {
        return color.withAlpha(separatorAlpha);
    });
    separatorRight.setTransformationOnColour([](const juce::Colour color) {
        return color.withAlpha(separatorAlpha);
    });
}

int Section::getSectionHeight() noexcept
{
    return AutoSizeLabel::getLabelHeight();
}


// Component method implementations.
// ====================================

void Section::resized()
{
    const auto width = getWidth();

    constexpr auto labelLeftMargin = 30;

    label.setTopLeftPosition(labelLeftMargin, 0);

    const auto separatorY = label.getY() + ((label.getHeight() - separatorHeight) / 2);
    const auto separatorRightX = label.getRight();
    const auto separatorRightWidth = width - separatorRightX;
    separatorLeft.setBounds(0, separatorY, labelLeftMargin - 4, separatorHeight);
    separatorRight.setBounds(separatorRightX - 5, separatorY, separatorRightWidth, separatorHeight);
}

}   // namespace arkostracker
