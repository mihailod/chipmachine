#pragma once

#include <unordered_map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/EnumHasher.h"

namespace arkostracker 
{

/** Enumeration of the possible keyboards. */
enum class VirtualKeyboardLayout
{
    first,

    qwerty = first,
    qwertz,
    azerty,

    last = azerty,

    none            // Put after the last, so that it is never reached.
};

/** Class to convert the keyboard layouts into human-readable Strings. */
class VirtualKeyboardLayoutNames
{
public:
    /** Constructor. */
    VirtualKeyboardLayoutNames() noexcept;

    /** Returns a human-readable string from the given keyboard layout. */
    juce::String getKeyboardLayoutReadableString(VirtualKeyboardLayout layout) const noexcept;

    /**
        Fills the given ComboBox with the keyboard layouts. They are in the order of the KeyboardLayouts enumeration.
        @param comboBox the ComboBox to fill. Its content is cleared first.
        @param selectedKeyboard the layout that is first selected.
    */
    void fillComboBoxWithKeyboardLayoutNames(juce::ComboBox& comboBox, VirtualKeyboardLayout selectedKeyboard = VirtualKeyboardLayout::first) const noexcept;

private:
    std::unordered_map<VirtualKeyboardLayout, juce::String, EnumHasher> keyboardLayoutToReadableString;         // Links a KeyboardLayout enumeration to a human-readable String.
};

}   // namespace arkostracker

