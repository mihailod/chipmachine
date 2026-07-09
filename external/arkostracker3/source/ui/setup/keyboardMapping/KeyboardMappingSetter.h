#pragma once

#include "../../keyboard/VirtualKeyboardLayout.h"

namespace arkostracker 
{

/** Class to overwrite the commands of the keyboard mapping, according to the given keyboard layout (azerty, qwerty, etc.). */
class KeyboardMappingSetter
{
public:
    /**
     * Changes the keyboard layout. This also updates the preferences.
     * @param commandManager the command manager.
     * @param keyboardLayout the new layout.
     */
    static void changeKeyboardLayout(juce::ApplicationCommandManager& commandManager, VirtualKeyboardLayout keyboardLayout) noexcept;
};

}   // namespace arkostracker
