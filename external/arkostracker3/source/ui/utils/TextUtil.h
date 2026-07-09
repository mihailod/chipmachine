#pragma once

#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

class TextUtil
{
public:
    /** Prevents instantiation. */
    TextUtil() = delete;

    /**
     * @return the size of a text.
     * @param text the text.
     * @param font the font of the text (do a "label.getFont()" for example).
     * @param maximumWidth the maximum width you want the text to have.
     */
    static std::pair<int, int> calculateTextSize(const juce::String& text, const juce::Font& font, int maximumWidth) noexcept;
};

}   // namespace arkostracker
