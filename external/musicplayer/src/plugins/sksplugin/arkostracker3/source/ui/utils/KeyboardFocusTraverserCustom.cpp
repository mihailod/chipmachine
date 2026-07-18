#include "KeyboardFocusTraverserCustom.h"

namespace arkostracker 
{

KeyboardFocusTraverserCustom::KeyboardFocusTraverserCustom(juce::Component& pComponentToFocusOn) :
    componentToFocusOn(pComponentToFocusOn)
{
}

juce::Component* KeyboardFocusTraverserCustom::getDefaultComponent(juce::Component* /*parentComponent*/)
{
    return &componentToFocusOn;
}

juce::Component* KeyboardFocusTraverserCustom::getPreviousComponent(juce::Component* /*parentComponent*/)
{
    return &componentToFocusOn;
}

juce::Component* KeyboardFocusTraverserCustom::getNextComponent(juce::Component* /*parentComponent*/)
{
    // Useful for Components such as SliderIncDec which pass the focus to the next Component when their job is done (EditText validated).
    return &componentToFocusOn;
}

}   // namespace arkostracker
