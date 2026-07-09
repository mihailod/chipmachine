#include "AutoSizeLabel.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

AutoSizeLabel::AutoSizeLabel(const juce::String& text) noexcept :
        Label(juce::String(), text)
{
    // Once again a magic number, else the text is shrunk, I don't know why.
    const auto width = static_cast<int>(getFont().getStringWidth(text) * 1.25);
    const auto labelHeight = getLabelHeight();

    setSize(width, labelHeight);
}

int AutoSizeLabel::getLabelHeight() noexcept
{
    return LookAndFeelConstants::labelsHeight;
}

}   // namespace arkostracker
