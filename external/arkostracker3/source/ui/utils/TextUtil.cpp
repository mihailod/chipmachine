#include "TextUtil.h"

namespace arkostracker
{

std::pair<int, int> TextUtil::calculateTextSize(const juce::String& text, const juce::Font& font, const int maximumWidth) noexcept
{
    // Creates a TextLayout which, when given the text and font, will calculate the size. However, this doesn't seem perfect.
    juce::TextLayout textLayout;
    juce::AttributedString as;
    as.setJustification(juce::Justification::centred);
    as.append(text, font);
    textLayout.createLayoutWithBalancedLineLengths(as, static_cast<float>(maximumWidth));

    const auto textWidth = static_cast<int>(textLayout.getWidth() * 1.1);      // Hate those magic number, but doesn't look good without them.
    const auto textHeight = static_cast<int>(textLayout.getHeight() * 1.2);

    return { textWidth, textHeight };
}

}   // namespace arkostracker
