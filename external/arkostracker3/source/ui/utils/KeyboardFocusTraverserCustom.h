#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * KeyboardFocusTraverser which can be given a Component to focus on. This is useful to the Panels for example, as they can be given focus, but don't care
 * about it: they want one specific child to have it. This child can thus be passed to the constructor.
 */
class KeyboardFocusTraverserCustom final : public juce::KeyboardFocusTraverser
{
public:
    /**
     * Constructor.
     * @param componentToFocusOn the Component that should have the focus.
     */
    explicit KeyboardFocusTraverserCustom(juce::Component& componentToFocusOn);

    // KeyboardFocusTraverser method implementations.
    // ==============================================================
    juce::Component* getDefaultComponent(juce::Component* parentComponent) override;
    juce::Component* getPreviousComponent(juce::Component* parentComponent) override;
    juce::Component* getNextComponent(juce::Component* parentComponent) override;

private:
    juce::Component& componentToFocusOn;          // The Component that should have the focus.
};

}   // namespace arkostracker

