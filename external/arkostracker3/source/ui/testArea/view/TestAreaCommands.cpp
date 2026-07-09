#include "TestAreaCommands.h"

namespace arkostracker 
{

void TestAreaCommands::getAllCommands(juce::Array<juce::CommandID>& /*commands*/)
{
    //static const juce::CommandID ids[] = { // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays)
            //CommandIds::testAreaCommand
    //};

    // Lists all the Ids of the commands used here.
    //commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
}

void TestAreaCommands::getCommandInfo(juce::CommandID /*commandID*/, juce::ApplicationCommandInfo& /*result*/)
{
    //const juce::String categoryTestArea = CategoryUtils::getCategoryString(Category::testArea);
    //const unsigned int flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;

    // Gives information for each command.
    /*switch (commandID) {
        case CommandIds::testAreaCommand:
            result.setInfo(juce::translate("TEST AREA Command"), juce::translate("TEST AREA Command."), categoryTestArea, flagNoTrigger);
            result.addDefaultKeypress('e', juce::ModifierKeys::noModifiers);
            break;

        default:
            jassertfalse;       // Forgot a command?
            break;
    }*/
}

}   // namespace arkostracker

