#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A TextEditor with our font size set, and a better "disabled". */
class EditText final : public juce::TextEditor
{
public:
    explicit EditText(const juce::String& text = juce::String(), bool enabled = true, bool multiLine = false) noexcept;

    /**
     * Links the given component, which will be enabled/disabled when the current one is. WARNING, it must not die when this happen!
     * \param linkedComponent the component.
     */
    void setLinkedComponent(Component* linkedComponent) noexcept;

    // juce::TextEditor method implementations.
    // ==================================================
    void enablementChanged() override;

private:
    Component* linkedComponent;
};

}   // namespace arkostracker
