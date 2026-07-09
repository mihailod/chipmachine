#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A simple Label, which size is automatically set on construction, according to the text size. */
class AutoSizeLabel final : public juce::Label
{
public:
    /**
     * Constructor. The size is set on this.
     * @param text the text to show.
     */
    explicit AutoSizeLabel(const juce::String& text) noexcept;

    /** @return the static height of the label. */
    static int getLabelHeight() noexcept;
};

}   // namespace arkostracker
