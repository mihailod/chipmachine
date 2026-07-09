#include "BarEditorCommands.h"

#include "../../keyboard/Category.h"
#include "../../keyboard/CommandIds.h"

namespace arkostracker 
{

void BarEditorCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    // Declares the commands for the children, simpler.
    static constexpr juce::CommandID ids[] = { // NOLINT(*-avoid-c-arrays)
            barEditorInsert,
            barEditorDelete,
            barEditorCursorEdit,
            barEditorCursorRight,
            barEditorCursorRightFast,
            barEditorCursorRightLimit,
            barEditorCursorLeft,
            barEditorCursorLeftFast,
            barEditorCursorLeftLimit,
            barEditorCursorDown,
            barEditorCursorDownFast,
            barEditorCursorDownLimit,
            barEditorCursorUp,
            barEditorCursorUpFast,
            barEditorCursorUpLimit,
            barEditorCursorDuplicateColumn,
            barEditorCursorDuplicateValue,
            barEditorIncreaseValue,
            barEditorIncreaseValueFast,
            barEditorIncreaseValueFastest,
            barEditorDecreaseValue,
            barEditorDecreaseValueFast,
            barEditorDecreaseValueFastest,
            barEditorCursorSetMainLoopStart,
            barEditorCursorSetMainLoopEnd,
            barEditorResetValue,
            barEditorToggleMainLoop,
            barEditorCursorSetAutoSpreadLoopStart,
            barEditorCursorSetAutoSpreadLoopEnd,
            barEditorToggleAutoSpreadLoop,
            barEditorIncreaseSpeed,
            barEditorDecreaseSpeed,
            barEditorHideUnusedRows,
            barEditorHideUnusedRowsFull,
            barEditorEnlargeCurrentRow,
            barEditorShrinkCurrentRow,
            barEditorToggleHideInRowAboveCursor,
            barEditorToggleHideInRowBelowCursor,
            psgBarEditorToggleRetrig,
            psgBarEditorGenerateIncreasingVolume,
            psgBarEditorGenerateDecreasingVolume,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(clion-misra-cpp2008-5-2-12,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
}

void BarEditorCommands::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    const auto category = CategoryUtils::getCategoryString(Category::barEditor);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;

    // Gives information for each command.
    // Declares the commands for the children, simpler (but not the "perform", done by them).
    switch (commandId) {
        case barEditorInsert:
            result.setInfo(juce::translate("Insert"), juce::translate("Insert"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::insertKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorDelete:
            result.setInfo(juce::translate("Delete"), juce::translate("Delete"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers);
            break;

        case psgBarEditorToggleRetrig:
            result.setInfo(juce::translate("Toggle retrig"), juce::translate("Toggle retrig"), category, flagNoTrigger);
            result.addDefaultKeypress('r', juce::ModifierKeys::ctrlModifier);
            break;
        case psgBarEditorGenerateIncreasingVolume:
            result.setInfo(juce::translate("Generate increasing volume"), juce::translate("Generate increasing volume"), category, flagNoTrigger);
            result.addDefaultKeypress('i', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            break;
        case psgBarEditorGenerateDecreasingVolume:
            result.setInfo(juce::translate("Generate decreasing volume"), juce::translate("Generate decreasing volume"), category, flagNoTrigger);
            result.addDefaultKeypress('d', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            break;

        case barEditorCursorRight:
            result.setInfo(juce::translate("Cursor right"), juce::translate("Cursor right"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorRightFast:
            result.setInfo(juce::translate("Cursor right, fast"), juce::translate("Cursor right, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::altModifier);
            break;
        case barEditorCursorRightLimit:
            result.setInfo(juce::translate("Cursor right to end"), juce::translate("Cursor right to end"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::endKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorLeft:
            result.setInfo(juce::translate("Cursor left"), juce::translate("Cursor left"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorLeftFast:
            result.setInfo(juce::translate("Cursor left, fast"), juce::translate("Cursor left, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::altModifier);
            break;
        case barEditorCursorLeftLimit:
            result.setInfo(juce::translate("Cursor left to start"), juce::translate("Cursor left to start"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::homeKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorUp:
            result.setInfo(juce::translate("Cursor up"), juce::translate("Cursor up"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorUpFast:
            result.setInfo(juce::translate("Cursor up, fast"), juce::translate("Cursor up, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorUpLimit:
            result.setInfo(juce::translate("Cursor to top"), juce::translate("Cursor to top"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorCursorDown:
            result.setInfo(juce::translate("Cursor down"), juce::translate("Cursor down"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorDownFast:
            result.setInfo(juce::translate("Cursor down, fast"), juce::translate("Cursor down, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorCursorDownLimit:
            result.setInfo(juce::translate("Cursor to bottom"), juce::translate("Cursor to bottom"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorCursorDuplicateColumn:
            result.setInfo(juce::translate("Duplicate column"), juce::translate("Duplicate column"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorCursorDuplicateValue:
            result.setInfo(juce::translate("Duplicate value"), juce::translate("Duplicate value"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
            break;

        case barEditorCursorEdit:
            result.setInfo(juce::translate("Edit value under cursor"), juce::translate("Edit value under cursor"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers);
            break;
        case barEditorIncreaseValue:
            result.setInfo(juce::translate("Increase value"), juce::translate("Increase value"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier);
            break;
        case barEditorIncreaseValueFast:
            result.setInfo(juce::translate("Increase value, fast"), juce::translate("Increase value, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorIncreaseValueFastest:
            result.setInfo(juce::translate("Increase value, fastest"), juce::translate("Increase value, fastest"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorDecreaseValue:
            result.setInfo(juce::translate("Decrease value"), juce::translate("Decrease value"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier);
            break;
        case barEditorDecreaseValueFast:
            result.setInfo(juce::translate("Decrease value, fast"), juce::translate("Decrease value, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorDecreaseValueFastest:
            result.setInfo(juce::translate("Decrease value, fastest"), juce::translate("Decrease value, fastest"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorResetValue:
            result.setInfo(juce::translate("Reset value"), juce::translate("Reset value"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers);
            break;

        case barEditorCursorSetMainLoopStart:
            result.setInfo(juce::translate("Set loop start"), juce::translate("Set loop start from cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('i', juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorCursorSetMainLoopEnd:
            result.setInfo(juce::translate("Set end"), juce::translate("Set end from cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('o', juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorToggleMainLoop:
            result.setInfo(juce::translate("Toggle loop"), juce::translate("Toggle loop"), category, flagNoTrigger);
            result.addDefaultKeypress('p', juce::ModifierKeys::ctrlModifier);
            break;

        case barEditorCursorSetAutoSpreadLoopStart:
            result.setInfo(juce::translate("Set auto-spread loop start"), juce::translate("Set auto-spread loop start from cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('i', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorCursorSetAutoSpreadLoopEnd:
            result.setInfo(juce::translate("Set auto-spread loop end"), juce::translate("Set auto-spread loop end from cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('o', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorToggleAutoSpreadLoop:
            result.setInfo(juce::translate("Toggle auto-spread loop"), juce::translate("Toggle auto-spread loop"), category, flagNoTrigger);
            result.addDefaultKeypress('p', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier);
            break;

        case barEditorHideUnusedRows:
            result.setInfo(juce::translate("Hide all unused rows"), juce::translate("Hide all unused rows"), category, flagNoTrigger);
            result.addDefaultKeypress('h', juce::ModifierKeys::shiftModifier);
            break;
        case barEditorHideUnusedRowsFull:
            result.setInfo(juce::translate("Hide all unused rows, shrink more"), juce::translate("Hide all unused rows, shrink more"), category, flagNoTrigger);
            result.addDefaultKeypress('h', juce::ModifierKeys::ctrlModifier);
            break;
        case barEditorEnlargeCurrentRow:
            result.setInfo(juce::translate("Enlarge current row"), juce::translate("Enlarge the current row"), category, flagNoTrigger);
            result.addDefaultKeypress('g', juce::ModifierKeys::shiftModifier);
            break;
        case barEditorShrinkCurrentRow:
            result.setInfo(juce::translate("Shrink current row"), juce::translate("Shrink the current row"), category, flagNoTrigger);
            result.addDefaultKeypress('g', juce::ModifierKeys::ctrlModifier);
            break;

        case barEditorToggleHideInRowAboveCursor:
            result.setInfo(juce::translate("Toggle hide state above cursor"), juce::translate("Toggle hide state above cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('f', juce::ModifierKeys::shiftModifier);
            break;
        case barEditorToggleHideInRowBelowCursor:
            result.setInfo(juce::translate("Toggle hide state below cursor"), juce::translate("Toggle hide state below cursor"), category, flagNoTrigger);
            result.addDefaultKeypress('f', juce::ModifierKeys::ctrlModifier);
            break;

        case barEditorIncreaseSpeed:
            result.setInfo(juce::translate("Increase speed"), juce::translate("Increase speed"), category, flagNoTrigger);
            result.addDefaultKeypress('e', juce::ModifierKeys::shiftModifier);
            break;
        case barEditorDecreaseSpeed:
            result.setInfo(juce::translate("Decrease speed"), juce::translate("Decrease speed"), category, flagNoTrigger);
            result.addDefaultKeypress('e', juce::ModifierKeys::ctrlModifier);
            break;

        default:
            jassertfalse;
            break;
    }
}

}   // namespace arkostracker
