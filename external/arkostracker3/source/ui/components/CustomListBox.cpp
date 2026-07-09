#include "CustomListBox.h"

namespace arkostracker 
{

CustomListBox::CustomListBox(juce::ListBoxModel* pModel, const bool pAllowSelectAll) noexcept :
        juce::ListBox(juce::String(), pModel),
        allowSelectAll(pAllowSelectAll)
{
}

bool CustomListBox::keyPressed(const juce::KeyPress& key)
{
    // Prevents CTRL+A, if wanted. This test comes directly from the same method from juce_ListBox.cpp
    if (!allowSelectAll && (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))) {
        return false;
    }

    return juce::ListBox::keyPressed(key);
}

}   // namespace arkostracker
