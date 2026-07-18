#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

/** A Label that can be clicked. */
class ClickableLabel final : public juce::Label
{
public:
    /**
     * Creates a Label.
     * @param labelText the text to show in the label.
     * @param onClick called when the label is clicked.
     */
    ClickableLabel(const juce::String& labelText, std::function<void()> onClick) noexcept;

    // Label method overrides.
    // =============================================
    void mouseDown(const juce::MouseEvent& event) override;

private:
    std::function<void()> onClick;
};

}   // namespace arkostracker
