#include "LinkerPanelAreaCommands.h"

#include "../keyboard/Category.h"
#include "../keyboard/CommandIds.h"
#include "controller/LinkerController.h"

namespace arkostracker 
{

LinkerPanelAreaCommands::LinkerPanelAreaCommands(LinkerController& pLinkerController) noexcept :
        linkerController(pLinkerController)
{
}

void LinkerPanelAreaCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    static constexpr juce::CommandID ids[] = {      // NOLINT(*-avoid-c-arrays)
            juce::StandardApplicationCommandIDs::cut,
            juce::StandardApplicationCommandIDs::copy,
            juce::StandardApplicationCommandIDs::paste,

            linkerGotoNext,
            linkerGotoPrevious,
            linkerGotoNextPage,
            linkerGotoPreviousPage,
            linkerGotoEnd,
            linkerGotoStart,

            linkerGrowNext,
            linkerGrowPrevious,
            linkerGrowNextPage,
            linkerGrowPreviousPage,
            linkerGrowStart,
            linkerGrowEnd,

            linkerDuplicateSelectedPositions,
            linkerCloneSelectedPositions,
            linkerCreateNew,
            linkerDeleteSelectedPositions,
            linkerEditPositions,
            linkerSelectPosition,
            linkerMarkAsLoopStartAndEnd,
            linkerMarkLoopStart,
            linkerMarkEnd,
            linkerIncreasePatternIndex,
            linkerDecreasePatternIndex,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
}

void LinkerPanelAreaCommands::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto category = CategoryUtils::getCategoryString(Category::linker);
    const auto categoryGeneral = CategoryUtils::getCategoryString(Category::general);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;

    // Gives information for each command.
    switch (commandID) {
        case juce::StandardApplicationCommandIDs::cut:
            result.setInfo(juce::translate("Cut"), juce::translate("Cut the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            result.setActive(linkerController.canCutSelection());
            break;
        case juce::StandardApplicationCommandIDs::copy:
            result.setInfo(juce::translate("Copy"), juce::translate("Copy the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case juce::StandardApplicationCommandIDs::paste:
            result.setInfo(juce::translate("Paste"), juce::translate("Paste the previously stored positions"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;

        case linkerGotoNext:
            result.setInfo(juce::translate("Goto next"), juce::translate("Goto next position"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerGotoPrevious:
            result.setInfo(juce::translate("Goto previous"), juce::translate("Goto previous position"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::noModifiers);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerGotoNextPage:
            result.setInfo(juce::translate("Goto next (fast)"), juce::translate("Goto next several positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerGotoPreviousPage:
            result.setInfo(juce::translate("Goto previous (fast)"), juce::translate("Goto previous several positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerGotoEnd:
            result.setInfo(juce::translate("Goto end"), juce::translate("Goto the end"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::endKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerGotoStart:
            result.setInfo(juce::translate("Goto beginning"), juce::translate("Goto the beginning"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::homeKey, juce::ModifierKeys::noModifiers);
            break;

        case linkerGrowNext:
            result.setInfo(juce::translate("Extend selection to next"), juce::translate("Extend the selection to next"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerGrowPrevious:
            result.setInfo(juce::translate("Extend selection to previous"), juce::translate("Extend the selection to previous"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::shiftModifier);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerGrowNextPage:
            result.setInfo(juce::translate("Extend selection to next (fast)"), juce::translate("Extend the selection to next (fast)"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerGrowPreviousPage:
            result.setInfo(juce::translate("Extend selection to previous (fast)"), juce::translate("Extend the selection to previous (fast)"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerGrowEnd:
            result.setInfo(juce::translate("Extend selection to end"), juce::translate("Extend the selection to end"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::endKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerGrowStart:
            result.setInfo(juce::translate("Extend selection to beginning"), juce::translate("Extend the selection to beginning"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::homeKey, juce::ModifierKeys::shiftModifier);
            break;

        case linkerDuplicateSelectedPositions:
            result.setInfo(juce::translate("Duplicate"), juce::translate("Duplicate the selected positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::insertKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerCloneSelectedPositions:
            result.setInfo(juce::translate("Clone"), juce::translate("Clone the selected positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier);
            break;
        case linkerCreateNew:
            result.setInfo(juce::translate("New"), juce::translate("Create a new position and pattern"), category, flagNoTrigger);
            result.addDefaultKeypress('n', juce::ModifierKeys::ctrlModifier);
            break;
        case linkerDeleteSelectedPositions:
            result.setInfo(juce::translate("Delete"), juce::translate("Delete the selected positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers);
            result.setActive(linkerController.canDeleteSelection());
            break;
        case linkerEditPositions:
            result.setInfo(juce::translate("Edit positions"), juce::translate("Edit the selected positions"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::shiftModifier);
            result.setActive(linkerController.isOneItemSelected(false));
            break;
        case linkerSelectPosition:
            result.setInfo(juce::translate("Open in pattern viewer"), juce::translate("Open in pattern viewer"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers);
            break;
        case linkerMarkAsLoopStartAndEnd:
            result.setInfo(juce::translate("Mark as loop start/end"), juce::translate("Mark selected positions as loop start/end"), category, flagNoTrigger);
            result.addDefaultKeypress('p', juce::ModifierKeys::ctrlModifier);
            break;
        case linkerMarkLoopStart:
            result.setInfo(juce::translate("Mark as loop start"), juce::translate("Mark as loop start"), category, flagNoTrigger);
            result.addDefaultKeypress('i', juce::ModifierKeys::ctrlModifier);
            result.setActive(linkerController.isOneItemSelected(true));
            break;
        case linkerMarkEnd:
            result.setInfo(juce::translate("Mark as end"), juce::translate("Mark as end"), category, flagNoTrigger);
            result.addDefaultKeypress('o', juce::ModifierKeys::ctrlModifier);
            result.setActive(linkerController.isOneItemSelected(true));
            break;
        case linkerIncreasePatternIndex:
            result.setInfo(juce::translate("Increase pattern"), juce::translate("Increase the pattern"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            result.setActive(linkerController.isOneItemSelected(true));
            break;
        case linkerDecreasePatternIndex:
            result.setInfo(juce::translate("Decrease pattern"), juce::translate("Decrease the pattern"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            result.setActive(linkerController.isOneItemSelected(true));
            break;
        default:
            jassertfalse;       // Forgot a command?
            break;
    }
}

}   // namespace arkostracker
