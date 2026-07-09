#include "VirtualKeyboardLayout.h"

namespace arkostracker 
{

VirtualKeyboardLayoutNames::VirtualKeyboardLayoutNames() noexcept :
    keyboardLayoutToReadableString()
{
    // Statically adds the keyboard layouts.
    keyboardLayoutToReadableString.insert(std::make_pair<VirtualKeyboardLayout, juce::String>(VirtualKeyboardLayout::qwerty, juce::translate("Qwerty keyboard")));
    keyboardLayoutToReadableString.insert(std::make_pair<VirtualKeyboardLayout, juce::String>(VirtualKeyboardLayout::qwertz, juce::translate("Qwertz keyboard")));
    keyboardLayoutToReadableString.insert(std::make_pair<VirtualKeyboardLayout, juce::String>(VirtualKeyboardLayout::azerty, juce::translate("Azerty keyboard")));
}

juce::String VirtualKeyboardLayoutNames::getKeyboardLayoutReadableString(VirtualKeyboardLayout layout) const noexcept
{
    // Finds the related String. It must exist!
    auto it = keyboardLayoutToReadableString.find(layout);
    jassert(it != keyboardLayoutToReadableString.cend());

    return it->second;
}

void VirtualKeyboardLayoutNames::fillComboBoxWithKeyboardLayoutNames(juce::ComboBox& comboBox, VirtualKeyboardLayout selectedKeyboard) const noexcept
{
    comboBox.clear();

    // Browses through the enumeration. Not very classy in C++11...
    jassert(static_cast<int>(VirtualKeyboardLayout::first) <= static_cast<int>(VirtualKeyboardLayout::last));
    for (int i = static_cast<int>(VirtualKeyboardLayout::first); i <= static_cast<int>(VirtualKeyboardLayout::last); ++i) {
        auto layout = static_cast<VirtualKeyboardLayout>(i);
        juce::String keyboardName = getKeyboardLayoutReadableString(layout);

        comboBox.addItem(keyboardName, i + 1);          // Id 0 is reserved by Juce...
    }

    // Selects the desired item.
    comboBox.setSelectedItemIndex(static_cast<int>(selectedKeyboard), juce::NotificationType::sendNotification);
}

}   // namespace arkostracker

