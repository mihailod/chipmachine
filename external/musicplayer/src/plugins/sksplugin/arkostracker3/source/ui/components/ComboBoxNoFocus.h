#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Simple juce::ComboBox with declares it doesn't want the focus. */
class ComboBoxNoFocus final : public juce::ComboBox
{
public:
    /**
     * Creates a combo-box.
     * On construction, the text field will be empty, so you should call the
     * setSelectedId() or setText() method to choose the initial value before
     * displaying it.
     * @param componentName the name to set for the component (see Component::setName())
     */
    explicit ComboBoxNoFocus(const juce::String& componentName = {});
};

}   // namespace arkostracker

