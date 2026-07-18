#include "ComboBoxNoFocus.h"

namespace arkostracker 
{

ComboBoxNoFocus::ComboBoxNoFocus(const juce::String& pComponentName) :
        juce::ComboBox(pComponentName)
{
    setWantsKeyboardFocus(false);
}


}   // namespace arkostracker

