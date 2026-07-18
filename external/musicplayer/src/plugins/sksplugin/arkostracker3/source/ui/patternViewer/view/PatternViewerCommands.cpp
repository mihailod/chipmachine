#include "PatternViewerCommands.h"

#include "../../keyboard/Category.h"
#include "../../keyboard/CommandIds.h"
#include "../../keyboard/KeyCodes.h"
#include "../controller/PatternViewerController.h"

namespace arkostracker 
{

PatternViewerCommands::PatternViewerCommands(PatternViewerController& pPatternViewerController) noexcept :
        patternViewerController(pPatternViewerController)
{
}

void PatternViewerCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    static constexpr juce::CommandID ids[] = { // NOLINT(*-avoid-c-arrays)
            juce::StandardApplicationCommandIDs::copy,
            juce::StandardApplicationCommandIDs::cut,
            juce::StandardApplicationCommandIDs::paste,
            patternViewerPasteAndMoveBelow,
            patternViewerPasteContinuously,
            patternViewerPasteMix,
            patternViewerClearSelection,

            patternViewerMoveDown,
            patternViewerMoveUp,
            patternViewerMoveFastDown,
            patternViewerMoveFastUp,
            patternViewerMoveTop,
            patternViewerMoveBottom,

            patternViewerMoveLeft,
            patternViewerMoveRight,
            patternViewerMoveToNextChannel,
            patternViewerMoveToPreviousChannel,
            patternViewerMoveToNextPosition,
            patternViewerMoveToPreviousPosition,

            patternViewerMoveDownEnlargeSelection,
            patternViewerMoveUpEnlargeSelection,
            patternViewerMoveFastDownEnlargeSelection,
            patternViewerMoveFastUpEnlargeSelection,
            patternViewerMoveTopEnlargeSelection,
            patternViewerMoveBottomEnlargeSelection,

            patternViewerMoveLeftEnlargeSelection,
            patternViewerMoveRightEnlargeSelection,

            patternViewerToggleRecord,
            patternViewerToggleFollow,
            patternViewerDelete,
            patternViewerDeleteWholeCell,
            patternViewerPlayLine,
            patternViewerSetBlockTogglePlayPatternFromBlock,
            patternViewerPlayPatternFromStartIgnoreBlock,
            patternViewerToggleOverwriteDefaultEffect,
            patternViewerNextDefaultEffect,
            patternViewerPreviousDefaultEffect,

            patternViewerTransposePlusOne,
            patternViewerTransposeMinusOne,
            patternViewerTransposePlusOneOctave,
            patternViewerTransposeMinusOneOctave,
            patternViewerToggleReadOnly,
            patternViewerGenerateArpeggio,
            patternViewerGenerateInstrument,
            patternViewerSetTrackNameAndColor,
            patternViewerLinkTrack,

            patternViewerInsert,
            patternViewerRemove,

            patternViewerIncreaseEditStep,
            patternViewerDecreaseEditStep,

            patternViewerToggleSolo,
            patternViewerToggleMute,

            patternViewerSelectTrack,
            patternViewerSelectAllTracks,
            patternViewerCapture,
            patternViewerRst,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids)); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)*/
}

void PatternViewerCommands::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    const auto category = CategoryUtils::getCategoryString(Category::patternViewer);
    const auto categoryGeneral = CategoryUtils::getCategoryString(Category::general);
    constexpr auto flagNoTrigger = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;
    constexpr auto flagReadOnly = flagNoTrigger | juce::ApplicationCommandInfo::CommandFlags::readOnlyInKeyEditor;

    // Gives information for each command.
    switch (commandId) {

        case juce::StandardApplicationCommandIDs::cut:
            result.setInfo(juce::translate("Cut"), juce::translate("Cut the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            break;
        case juce::StandardApplicationCommandIDs::copy:
            result.setInfo(juce::translate("Copy"), juce::translate("Copy the selection"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case juce::StandardApplicationCommandIDs::paste:
            result.setInfo(juce::translate("Paste"), juce::translate("Paste"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;
        case patternViewerPasteAndMoveBelow:
            result.setInfo(juce::translate("Paste and move below"), juce::translate("Paste, move below"), category, flagNoTrigger);
            result.addDefaultKeypress('v', juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerPasteContinuously:
            result.setInfo(juce::translate("Paste continuously"), juce::translate("Paste continuously"), category, flagNoTrigger);
            result.addDefaultKeypress('p', juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerPasteMix:
            result.setInfo(juce::translate("Paste mix"), juce::translate("Paste and mix"), category, flagNoTrigger);
            result.addDefaultKeypress('v', juce::ModifierKeys::shiftModifier | juce::ModifierKeys::commandModifier);
            break;
        case patternViewerClearSelection:
            result.setInfo(juce::translate("Clear selection"), juce::translate("Clear the selection"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerTransposePlusOne:
            result.setInfo(juce::translate("Transpose up"), juce::translate("Transpose the selection up"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerTransposeMinusOne:
            result.setInfo(juce::translate("Transpose down"), juce::translate("Transpose the selection down"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerTransposePlusOneOctave:
            result.setInfo(juce::translate("Transpose up fast"), juce::translate("Transpose the selection up fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerTransposeMinusOneOctave:
            result.setInfo(juce::translate("Transpose down fast"), juce::translate("Transpose the selection down fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerToggleReadOnly:
            result.setInfo(juce::translate("Toggle read-only"), juce::translate("Toggle read-only flag"), category, flagNoTrigger);
            break;
        case patternViewerSetTrackNameAndColor:
            result.setInfo(juce::translate("Set track name/color"), juce::translate("Set the track name and color"), category, flagNoTrigger);
            result.setActive(patternViewerController.canSetNameAndColorToTrack());
            break;
        case patternViewerLinkTrack:
            result.setInfo(juce::translate("Link track"), juce::translate("Link the track"), category, flagNoTrigger);
            result.setActive(patternViewerController.canLinkTrack());
            break;
        case patternViewerGenerateArpeggio:
            result.setInfo(juce::translate("Generate arpeggio from selection"), juce::translate("Generate arpeggio from selection"), category, flagNoTrigger);
            result.setActive(patternViewerController.canGenerateArpeggioFromSelection());
            break;
        case patternViewerGenerateInstrument:
            result.setInfo(juce::translate("Generate instrument from selection"), juce::translate("Generate instrument from selection"), category, flagNoTrigger);
            result.setActive(patternViewerController.canGenerateInstrumentFromSelection());
            break;

        case patternViewerInsert:
            result.setInfo(juce::translate("Insert"), juce::translate("Insert empty cell"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::insertKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerRemove:
            result.setInfo(juce::translate("Remove"), juce::translate("Remove cell and shift"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers);
            break;

        case patternViewerMoveDown:
            result.setInfo(juce::translate("Move cursor down"), juce::translate("Move the cursor down"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveDownEnlargeSelection:
            result.setInfo(juce::translate("Move cursor down with selection"), juce::translate("Move the cursor down, with selection"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveUp:
            result.setInfo(juce::translate("Move cursor up"), juce::translate("Move the cursor up"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveUpEnlargeSelection:
            result.setInfo(juce::translate("Move cursor up, with selection"), juce::translate("Move the cursor up, with selection"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveFastDown:
            result.setInfo(juce::translate("Move cursor down fast"), juce::translate("Move the cursor down, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveFastDownEnlargeSelection:
            result.setInfo(juce::translate("Move cursor down fast, with selection"), juce::translate("Move the cursor down, fast, with selection"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageDownKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveFastUp:
            result.setInfo(juce::translate("Move cursor up fast"), juce::translate("Move the cursor up, fast"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveFastUpEnlargeSelection:
            result.setInfo(juce::translate("Move cursor up fast, with selection"), juce::translate("Move the cursor up, fast, with selection"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::pageUpKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveTop:
            result.setInfo(juce::translate("Move cursor to top"), juce::translate("Move the cursor to the top"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::homeKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveTopEnlargeSelection:
            result.setInfo(juce::translate("Move cursor to top, with selection"), juce::translate("Move the cursor to the top, with selection"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::homeKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveBottom:
            result.setInfo(juce::translate("Move cursor to bottom"), juce::translate("Move the cursor to the bottom"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::endKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveBottomEnlargeSelection:
            result.setInfo(juce::translate("Move cursor to bottom, with selection"), juce::translate("Move the cursor to the bottom, with selection"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::endKey, juce::ModifierKeys::shiftModifier);
            break;

        case patternViewerMoveLeft:
            result.setInfo(juce::translate("Move cursor left"), juce::translate("Move the cursor to the left"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveLeftEnlargeSelection:
            result.setInfo(juce::translate("Move cursor left, with selection"), juce::translate("Move the cursor to the left, with selection"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveRight:
            result.setInfo(juce::translate("Move cursor right"), juce::translate("Move the cursor to the right"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerMoveRightEnlargeSelection:
            result.setInfo(juce::translate("Move cursor right, with selection"), juce::translate("Move the cursor to the right, with selection"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerMoveToNextChannel:
            result.setInfo(juce::translate("Move cursor to next channel"), juce::translate("Move the cursor to the next channel"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerMoveToPreviousChannel:
            result.setInfo(juce::translate("Move cursor to previous channel"), juce::translate("Move the cursor to the previous channel"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerMoveToNextPosition:
            result.setInfo(juce::translate("Move cursor to next position"), juce::translate("Move the cursor to the next position"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::altModifier);
            break;
        case patternViewerMoveToPreviousPosition:
            result.setInfo(juce::translate("Move cursor to previous position"), juce::translate("Move the cursor to the previous position"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::altModifier);
            break;

        case patternViewerToggleRecord:
            result.setInfo(juce::translate("Toggle record"), juce::translate("Toggle the record state"), category, flagReadOnly);
#ifdef JUCE_MAC
            result.addDefaultKeypress('r', juce::ModifierKeys::ctrlModifier);
#else
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::ctrlModifier);
#endif
            break;
        case patternViewerToggleFollow:
            result.setInfo(juce::translate("Toggle follow mode"), juce::translate("Toggle the follow mode"), category, flagReadOnly);
            result.addDefaultKeypress('f', juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerDelete:
            result.setInfo(juce::translate("Delete"), juce::translate("Delete where the cursor is"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerDeleteWholeCell:
            result.setInfo(juce::translate("Delete whole cell"), juce::translate("Delete the whole cell"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey, juce::ModifierKeys::shiftModifier);
            break;

        case patternViewerPlayLine:
            result.setInfo(juce::translate("Play line"), juce::translate("Plays the line"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers);
            break;
        case patternViewerSetBlockTogglePlayPatternFromBlock:
            result.setInfo(juce::translate("Set block. Toggle play pattern at block"),
                           juce::translate("Set block. Toggle play pattern at block"), category, flagReadOnly);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerPlayPatternFromStartIgnoreBlock:
            result.setInfo(juce::translate("Toggle play pattern from start, ignore block"),
                           juce::translate("Toggle play pattern from start, ignore block"), category, flagReadOnly);
            result.addDefaultKeypress('w', juce::ModifierKeys::ctrlModifier);
            break;

        case patternViewerToggleOverwriteDefaultEffect:
            result.setInfo(juce::translate("Toggle overwrite default effect"),
                           juce::translate("Toggle overwrite default effect"), category, flagReadOnly);
            result.addDefaultKeypress('e', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerPreviousDefaultEffect:
            result.setInfo(juce::translate("Previous default effect"),
                           juce::translate("Previous default effect"), category, flagReadOnly);
            result.addDefaultKeypress('e', juce::ModifierKeys::shiftModifier);
            break;
        case patternViewerNextDefaultEffect:
            result.setInfo(juce::translate("Next default effect"),
                           juce::translate("Next default effect"), category, flagReadOnly);
            result.addDefaultKeypress('e', juce::ModifierKeys::ctrlModifier);
            break;

        case patternViewerIncreaseEditStep:
            result.setInfo(juce::translate("Increase edit step"), juce::translate("Increases the edit step"), category, flagNoTrigger);
            result.addDefaultKeypress('*', juce::ModifierKeys::noModifiers);
            break;
        case patternViewerDecreaseEditStep:
            result.setInfo(juce::translate("Decrease edit step"), juce::translate("Decreases the edit step"), category, flagNoTrigger);
            result.addDefaultKeypress('/', juce::ModifierKeys::noModifiers);
            break;

        case patternViewerToggleSolo:
            result.setInfo(juce::translate("Solo track"), juce::translate("Solo/unsolo the track"), category, flagNoTrigger);
            result.addDefaultKeypress(KeyCodes::keypad1(), juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerToggleMute:
            result.setInfo(juce::translate("Mute track"), juce::translate("Mute/unmute the track"), category, flagNoTrigger);
            result.addDefaultKeypress(KeyCodes::keypad0(), juce::ModifierKeys::ctrlModifier);
            break;

        case patternViewerSelectTrack:
            result.setInfo(juce::translate("Select track"), juce::translate("Select the track notes"), category, flagNoTrigger);
            result.addDefaultKeypress('t', juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerSelectAllTracks:
            result.setInfo(juce::translate("Select all tracks"), juce::translate("Select all the tracks"), category, flagNoTrigger);
            result.addDefaultKeypress('a', juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerCapture:
            result.setInfo(juce::translate("Capture instrument, expression, effect"),
                juce::translate("Capture the effect, expression, or the nearest instrument in the track"), category, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::returnKey, juce::ModifierKeys::ctrlModifier);
            break;
        case patternViewerRst:
            result.setInfo(juce::translate("Write a RST"), juce::translate("Write a RST note"), category, flagNoTrigger);
            result.addDefaultKeypress('r', juce::ModifierKeys::shiftModifier);
            break;

        default:
            // Don't assert: it may be a note from the keyboard, which is tested in the PatternViewerViewImpl.perform.
            break;
    }
}

}   // namespace arkostracker
