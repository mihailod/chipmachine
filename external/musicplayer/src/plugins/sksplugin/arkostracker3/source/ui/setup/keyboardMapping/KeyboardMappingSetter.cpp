#include "KeyboardMappingSetter.h"

#include "../../keyboard/VirtualKeyboardCommands.h"
#include "../../../app/preferences/PreferencesManager.h"

namespace arkostracker 
{

void KeyboardMappingSetter::changeKeyboardLayout(juce::ApplicationCommandManager& commandManager, const VirtualKeyboardLayout keyboardLayout) noexcept
{
    VirtualKeyboardCommands virtualKeyboardCommands(keyboardLayout);

    // Removes the previous commands, else the overwrite fails.
    juce::Array<juce::CommandID> commands;
    virtualKeyboardCommands.getAllCommands(commands);
    for (const auto commandId : commands) {
        commandManager.removeCommand(commandId);
    }

    // Overwrites.
    commandManager.registerAllCommandsForTarget(&virtualKeyboardCommands);

    // Saves the layout in the preferences.
    auto& preferencesManager = PreferencesManager::getInstance();
    preferencesManager.setVirtualKeyboardLayout(keyboardLayout);
}

}   // namespace arkostracker
