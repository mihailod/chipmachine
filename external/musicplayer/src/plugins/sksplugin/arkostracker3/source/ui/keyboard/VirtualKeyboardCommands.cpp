#include "VirtualKeyboardCommands.h"

#include "../../utils/NoteUtil.h"
#include "Category.h"
#include "CommandIds.h"

namespace arkostracker 
{

const std::unordered_map<int, VirtualKeyboardCommands::CommandData> VirtualKeyboardCommands::commandIdToCommandData = {    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        { CommandIds::keyboardCOctavePlus0, { "C", 0 } },
        { CommandIds::keyboardCsOctavePlus0, { "C#", 1 } },
        { CommandIds::keyboardDOctavePlus0, { "D", 2 } },
        { CommandIds::keyboardDsOctavePlus0, { "D#", 3 } },
        { CommandIds::keyboardEOctavePlus0, { "E", 4 } },
        { CommandIds::keyboardFOctavePlus0, { "F", 5 } },
        { CommandIds::keyboardFsOctavePlus0, { "F#", 6 } },
        { CommandIds::keyboardGOctavePlus0, { "G", 7 } },
        { CommandIds::keyboardGsOctavePlus0, { "Gs", 8 } },
        { CommandIds::keyboardAOctavePlus0, { "A", 9 } },
        { CommandIds::keyboardAsOctavePlus0, { "A#", 10 } },
        { CommandIds::keyboardBOctavePlus0, { "B", 11 } },

        { CommandIds::keyboardCOctavePlus1, { "C", 12 } },
        { CommandIds::keyboardCsOctavePlus1, { "C#", 13 } },
        { CommandIds::keyboardDOctavePlus1, { "D", 14 } },
        { CommandIds::keyboardDsOctavePlus1, { "D#", 15 } },
        { CommandIds::keyboardEOctavePlus1, { "E", 16 } },
        { CommandIds::keyboardFOctavePlus1, { "F", 17 } },
        { CommandIds::keyboardFsOctavePlus1, { "F#", 18 } },
        { CommandIds::keyboardGOctavePlus1, { "G", 19 } },
        { CommandIds::keyboardGsOctavePlus1, { "Gs", 20 } },
        { CommandIds::keyboardAOctavePlus1, { "A", 21 } },
        { CommandIds::keyboardAsOctavePlus1, { "A#", 22 } },
        { CommandIds::keyboardBOctavePlus1, { "B", 23 } },

        { CommandIds::keyboardCOctavePlus2, { "C", 24 } },
        { CommandIds::keyboardCsOctavePlus2, { "C#", 25 } },
        { CommandIds::keyboardDOctavePlus2, { "D", 26 } },
        { CommandIds::keyboardDsOctavePlus2, { "D#", 27 } },
        { CommandIds::keyboardEOctavePlus2, { "E", 28 } },

};

const std::vector<int> VirtualKeyboardCommands::azertyBiKeycodes = {    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        'w', 0,
        's', 0,
        'x', 0,
        'd', 0,
        'c', 0,
        'v', 0,
        'g', 0,
        'b', 0,
        'h', 0,
        'n', 0,
        'j', 0,
        ',', 0,

        'a', ';',
        L'\u00E9', 'l',
        'z', ':',
        '"', 'm',
        'e', '!',
        'r', 0,
        '(', 0,
        't', 0,
        '-', 0,
        'y', 0,
        L'\u00E8', 0,
        'u', 0,

        'i', 0,
        L'\u00E7', 0,
        'o', 0,
        L'\u00E0', 0,
        'p', 0,
};

const std::vector<int> VirtualKeyboardCommands::qwertyBiKeycodes = {    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        'z', 0,
        's', 0,
        'x', 0,
        'd', 0,
        'c', 0,
        'v', 0,
        'g', 0,
        'b', 0,
        'h', 0,
        'n', 0,
        'j', 0,
        'm', 0,

        'q', ',',
        '2', 'l',
        'w', '.',
        '3', ';',
        'e', '/',
        'r', 0,
        '5', 0,
        't', 0,
        '6', 0,
        'y', 0,
        '7', 0,
        'u', 0,

        'i', 0,
        '9', 0,
        'o', 0,
        '0', 0,
        'p', 0,
};

const std::vector<int> VirtualKeyboardCommands::qwertzBiKeycodes = {    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        'y', 0,
        's', 0,
        'x', 0,
        'd', 0,
        'c', 0,
        'v', 0,
        'g', 0,
        'b', 0,
        'h', 0,
        'n', 0,
        'j', 0,
        'm', 0,

        'q', ',',
        '2', 'l',
        'w', '.',
        '3', L'\u00F6',
        'e', '-',
        'r', 0,
        '5', 0,
        't', 0,
        '6', 0,
        'z', 0,
        '7', 0,
        'u', 0,

        'i', 0,
        '9', 0,
        'o', 0,
        '0', 0,
        'p', 0,
};

VirtualKeyboardCommands::VirtualKeyboardCommands(const VirtualKeyboardLayout virtualKeyboardLayouts) noexcept :
        virtualKeyboardLayout(virtualKeyboardLayouts)
{
}

void VirtualKeyboardCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    static constexpr juce::CommandID ids[] = {      // NOLINT(*-avoid-c-arrays)
            CommandIds::keyboardCOctavePlus0,
            CommandIds::keyboardCsOctavePlus0,
            CommandIds::keyboardDOctavePlus0,
            CommandIds::keyboardDsOctavePlus0,
            CommandIds::keyboardEOctavePlus0,
            CommandIds::keyboardFOctavePlus0,
            CommandIds::keyboardFsOctavePlus0,
            CommandIds::keyboardGOctavePlus0,
            CommandIds::keyboardGsOctavePlus0,
            CommandIds::keyboardAOctavePlus0,
            CommandIds::keyboardAsOctavePlus0,
            CommandIds::keyboardBOctavePlus0,

            CommandIds::keyboardCOctavePlus1,
            CommandIds::keyboardCsOctavePlus1,
            CommandIds::keyboardDOctavePlus1,
            CommandIds::keyboardDsOctavePlus1,
            CommandIds::keyboardEOctavePlus1,
            CommandIds::keyboardFOctavePlus1,
            CommandIds::keyboardFsOctavePlus1,
            CommandIds::keyboardGOctavePlus1,
            CommandIds::keyboardGsOctavePlus1,
            CommandIds::keyboardAOctavePlus1,
            CommandIds::keyboardAsOctavePlus1,
            CommandIds::keyboardBOctavePlus1,

            CommandIds::keyboardCOctavePlus2,
            CommandIds::keyboardCsOctavePlus2,
            CommandIds::keyboardDOctavePlus2,
            CommandIds::keyboardDsOctavePlus2,
            CommandIds::keyboardEOctavePlus2,
    };
    jassert(juce::numElementsInArray(ids) == static_cast<int>(commandIdToCommandData.size()));   // Forgot an item somewhere?

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids));       // NOLINT(clion-misra-cpp2008-5-2-12, *-pro-bounds-array-to-pointer-decay, *-no-array-decay)
}

void VirtualKeyboardCommands::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    const auto category = CategoryUtils::getCategoryString(Category::virtualKeyboard);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;

    // Finds the command in the map.
    const auto iterator = commandIdToCommandData.find(commandId);
    if (iterator == commandIdToCommandData.cend()) {
        return;
    }
    const auto& commandData = iterator->second;

    // Generates the text. Something like "C octave +0".
    const auto octave = NoteUtil::getOctaveFromNote(commandData.getNoteOffset());
    auto commandText = commandData.getDisplayedNote() + juce::translate(" octave +") + juce::String(octave);

    // The note offset gives the key rank. They are encoded as pair, the second being optional.
    const auto rank = static_cast<size_t>(commandData.getNoteOffset()) * 2U;
    const decltype(azertyBiKeycodes)* keyCodes;    // NOLINT(*-init-variables)
    switch (virtualKeyboardLayout) {
        default:
            [[fallthrough]];
        case VirtualKeyboardLayout::none:
            jassertfalse;           // Not a known layout??
        case VirtualKeyboardLayout::qwerty:
            keyCodes = &qwertyBiKeycodes;
            break;
        case VirtualKeyboardLayout::qwertz:
            keyCodes = &qwertzBiKeycodes;
            break;
        case VirtualKeyboardLayout::azerty:
            keyCodes = &azertyBiKeycodes;
            break;
    }

    const auto primaryKeyCode = keyCodes->at(rank);
    const auto secondaryKeyCode = keyCodes->at(rank + 1U);

    // Sets up the command.
    result.setInfo(commandText, commandText, category, static_cast<int>(flagNoTrigger));
    result.addDefaultKeypress(primaryKeyCode, juce::ModifierKeys::noModifiers);
    if (secondaryKeyCode != 0) {
        result.addDefaultKeypress(secondaryKeyCode, juce::ModifierKeys::noModifiers);
    }
}

int VirtualKeyboardCommands::performForNote(const juce::ApplicationCommandTarget::InvocationInfo& info)     // NOLINT(readability-convert-member-functions-to-static)
{
    // Finds the command in the map.
    const auto iterator = commandIdToCommandData.find(info.commandID);
    if (iterator == commandIdToCommandData.cend()) {
        return -1;      // No note.
    }

    const auto& commandData = iterator->second;
    return commandData.getNoteOffset();
}

}   // namespace arkostracker
