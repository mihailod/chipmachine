#include "ListCommands.h"

#include "../keyboard/Category.h"
#include "../keyboard/CommandIds.h"
#include "../testArea/view/keyboard/KeyView.h"
#include "OperationValidityProvider.h"

namespace arkostracker 
{

ListCommands::ListCommands(const OperationValidityProvider& provider) noexcept :
        operationValidityProvider(provider)
{
}

void ListCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    static constexpr juce::CommandID ids[] = {      // NOLINT(*-avoid-c-arrays)
            juce::StandardApplicationCommandIDs::copy,
            juce::StandardApplicationCommandIDs::cut,
            juce::StandardApplicationCommandIDs::paste,

            juce::StandardApplicationCommandIDs::del,
            listRenameItem,
            listAddItem,
            listDuplicateItems,
            listInsertItemAfter,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)

    // Lets the subclass adds their possible commands.
    getSubCommands(commands);
}

void ListCommands::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto category = CategoryUtils::getCategoryString(Category::lists);
    const auto categoryGeneral = CategoryUtils::getCategoryString(Category::general);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;
    constexpr auto flagNotInEditor = juce::ApplicationCommandInfo::CommandFlags::hiddenFromKeyEditor;

    // Gives information for each command.
    switch (commandID) {
        // We don't try to set another key for the standard operations. They are declared by hidden.
        // We actually declare they only to be able to show in the popup-menu, else they don't show.
        case juce::StandardApplicationCommandIDs::cut:
            result.setInfo(juce::translate("Cut"), juce::translate("Cut the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(juce::StandardApplicationCommandIDs::cut));
            break;
        case juce::StandardApplicationCommandIDs::copy:
            result.setInfo(juce::translate("Copy"), juce::translate("Copy the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(juce::StandardApplicationCommandIDs::copy));
            break;
        case juce::StandardApplicationCommandIDs::paste:
            result.setInfo(juce::translate("Paste"), juce::translate("Paste the previously stored items"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(juce::StandardApplicationCommandIDs::paste));
            break;

        case juce::StandardApplicationCommandIDs::del:
            result.setInfo(juce::translate("Delete"), juce::translate("Delete the selected items"), categoryGeneral,
                           flagNoTrigger | flagNotInEditor);
            result.setActive(operationValidityProvider.canOperationBePerformed(juce::StandardApplicationCommandIDs::del));
            break;

        case listRenameItem:
            result.setInfo(juce::translate("Rename"), juce::translate("Rename the selected item"), category, flagNoTrigger);
            result.addDefaultKeypress('r', juce::ModifierKeys::ctrlModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(listRenameItem));
            break;
        case listAddItem:
            result.setInfo(juce::translate("Add"), juce::translate("Add a new item at the bottom"), category, flagNoTrigger);
            result.addDefaultKeypress('n', juce::ModifierKeys::ctrlModifier);
            break;
        case listDuplicateItems:
            result.setInfo(juce::translate("Duplicate"), juce::translate("Duplicate the selected items"), category, flagNoTrigger);
            result.addDefaultKeypress('d', juce::ModifierKeys::ctrlModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(listDuplicateItems));
            break;
        case listInsertItemAfter:
            result.setInfo(juce::translate("Insert"), juce::translate("Insert a new item"), category, flagNoTrigger);
            result.addDefaultKeypress('i', juce::ModifierKeys::ctrlModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(listInsertItemAfter));
            break;
        default:
            // Lets the subclass adds their possible commands.
            getSubCommandInfo(commandID, result);
            break;
    }
}

void ListCommands::getSubCommands(juce::Array<juce::CommandID>& commands) noexcept
{
    static constexpr juce::CommandID ids[] = {      // NOLINT(*-avoid-c-arrays)
            listLoadItem,
            listSaveItem,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
}

void ListCommands::getSubCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result) const noexcept
{
    const auto category = CategoryUtils::getCategoryString(Category::lists);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;

    // Gives information for each command.
    switch (commandID) {

        case listLoadItem:
            result.setInfo(juce::translate("Load item"), juce::translate("Load item"), category, flagNoTrigger);
            //result.addDefaultKeypress('l', juce::ModifierKeys::ctrlModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(listLoadItem));
            break;
        case listSaveItem:
            result.setInfo(juce::translate("Save item"), juce::translate("Save item"), category, flagNoTrigger);
            //result.addDefaultKeypress('n', juce::ModifierKeys::ctrlModifier);
            result.setActive(operationValidityProvider.canOperationBePerformed(listSaveItem));
            break;
        default:
            jassertfalse;       // Forgot a command?
            break;
    }
}

}   // namespace arkostracker
