#include "MainApplicationCommands.h"

#include "../../controllers/MainController.h"
#include "Category.h"
#include "CommandIds.h"
#include "KeyCodes.h"

namespace arkostracker 
{

MainApplicationCommands::MainApplicationCommands(MainController& pMainController) noexcept :
        mainController(pMainController)
{
}

void MainApplicationCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    static constexpr juce::CommandID ids[] = { // NOLINT(*-avoid-c-arrays)
            juce::StandardApplicationCommandIDs::quit,
            juce::StandardApplicationCommandIDs::undo,
            juce::StandardApplicationCommandIDs::redo,

            /*StandardApplicationCommandIDs::cut,           // Somewhat complicated to show (because of the active state)
            StandardApplicationCommandIDs::copy,
            StandardApplicationCommandIDs::paste,*/

            newSong,
            loadSong,
            saveSong,
            saveSongAs,
            saveCopy,
            clearRecentFiles,
            loadInstrument,
            mergeSong,

            playSongFromStart,
            playSongContinue,
            playPatternFromStart,
            playPatternFromCursorOrBlock,
            togglePlayPatternFromBlockIfPresentElsePlayPattern,
            stop,

            increaseOctave,
            decreaseOctave,
            previousInstrument,
            nextInstrument,

            setupGeneral,
            setupKeyboardMapping,
            setupThemeEditor,
            setupPatternViewer,
            setupAudioInterface,
            setupOutput,
            setupSourceProfile,
            setupSerial,

            exportToAkg,
            exportToAkm,
            exportToAky,
            exportToFap,
            exportToMidi,
            exportToMod,
            exportToText,
            exportToYm,
            exportYmToFap,
            exportToVgm,
            exportToWav,
            exportInstrumentToWav,
            exportSfxs,
            exportEvents,
            exportToRaw,
            exportToRawLinear,
            exportSamples,

            quickExport,
            quickExportToBasic,

            openStreamedMusicAnalyzer,
            openInstrumentOptimizer,
            openErrorChecker,
            rearrangePatterns,
            clearPatterns,
            rearrangeAndDeleteUnusedPatterns,
            openAboutPage,

            editSongProperties,

            restoreLayout0,
            restoreLayout1,
            restoreLayout2,
            restoreLayout3,

            openLinker,
            openPatternViewer,
            openInstrumentEditor,
            openInstrumentList,
            openArpeggioTable,
            openArpeggioList,
            openPitchTable,
            openPitchList,
            openTestArea,
    };

    // Lists all the Ids of the commands used here.
    commands.addArray(ids, juce::numElementsInArray(ids));      // NOLINT(clion-misra-cpp2008-5-2-12, *-pro-bounds-array-to-pointer-decay, *-no-array-decay)
}

void MainApplicationCommands::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    const auto categoryGeneral = CategoryUtils::getCategoryString(Category::general);
    const auto categoryPlay = CategoryUtils::getCategoryString(Category::play);
    constexpr auto flagNoTrigger = static_cast<int>(juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback);
    constexpr auto flagHiddenFromKeyEditor = static_cast<int>(static_cast<unsigned int>(flagNoTrigger) | juce::ApplicationCommandInfo::CommandFlags::hiddenFromKeyEditor);

    // Gives information for each command.
    switch (commandId) {
        case juce::StandardApplicationCommandIDs::quit:
            result.setInfo(juce::translate("Quit"), juce::translate("Quit the application"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('q', juce::ModifierKeys::commandModifier);
            break;
        case juce::StandardApplicationCommandIDs::undo:
            result.setInfo(juce::translate("Undo"), juce::translate("Undo the last action"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            result.setActive(mainController.canUndo());
            break;
        case juce::StandardApplicationCommandIDs::redo:
            result.setInfo(juce::translate("Redo"), juce::translate("Redo the last action"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('z', (juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier));
            result.setActive(mainController.canRedo());
            break;

        case newSong:
            result.setInfo(juce::translate("New song"), juce::translate("Create a new song"), categoryGeneral, flagNoTrigger);
            break;
        case mergeSong:
            result.setInfo(juce::translate("Merge song"), juce::translate("Merge a song into the current one"), categoryGeneral, flagNoTrigger);
            break;
        case loadSong:
            result.setInfo(juce::translate("Open song"), juce::translate("Open a song"), categoryGeneral, flagNoTrigger);
            break;
        case saveSong:
            result.setInfo(juce::translate("Save song"), juce::translate("Save the song"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier);
            break;
        case saveSongAs:
            result.setInfo(juce::translate("Save song as..."), juce::translate("Save the song as..."), categoryGeneral, flagNoTrigger);
            break;
        case saveCopy:
            result.setInfo(juce::translate("Save a copy"), juce::translate("Save a copy"), categoryGeneral, flagNoTrigger);
            result.setActive(mainController.canSaveCopy());
            break;
        case clearRecentFiles:
            result.setInfo(juce::translate("Clear recent file list"), juce::translate("Clears the list of the recent files."),
                           categoryGeneral, flagHiddenFromKeyEditor);
            break;
        case loadInstrument:
            result.setInfo(juce::translate("Load instrument"), juce::translate("Load an instrument"), categoryGeneral, flagNoTrigger);
            break;

        case setupGeneral:
            result.setInfo(juce::translate("General setup"), juce::translate("General setup"), categoryGeneral, flagNoTrigger);
            break;
        case setupKeyboardMapping:
            result.setInfo(juce::translate("Keyboard mapping setup"), juce::translate("Set up the keyboard mapping"), categoryGeneral, flagNoTrigger);
            break;
        case setupThemeEditor:
            result.setInfo(juce::translate("Theme editor"), juce::translate("Edit the theme (colors, etc.)"), categoryGeneral, flagNoTrigger);
            break;
        case setupPatternViewer:
            result.setInfo(juce::translate("Pattern viewer setup"), juce::translate("Set up the pattern viewer"), categoryGeneral, flagNoTrigger);
            break;
        case setupAudioInterface:
            result.setInfo(juce::translate("Audio/MIDI interfaces setup"), juce::translate("Set up the audio/MIDI interfaces"), categoryGeneral, flagNoTrigger);
            break;
        case setupOutput:
            result.setInfo(juce::translate("Output/mix setup"), juce::translate("Set up the output and mix"), categoryGeneral, flagNoTrigger);
            break;
        case setupSourceProfile:
            result.setInfo(juce::translate("Source profile setup"), juce::translate("Set up the source profile"), categoryGeneral, flagNoTrigger);
            break;
        case setupSerial:
            result.setInfo(juce::translate("Serial communication setup"), juce::translate("Set up the serial communication"), categoryGeneral, flagNoTrigger);
            break;

        case exportToAkg:
            result.setInfo(juce::translate("Export to AKG"), juce::translate("Export to AKG"), categoryGeneral, flagNoTrigger);
            break;
        case exportToAky:
            result.setInfo(juce::translate("Export to AKY"), juce::translate("Export to AKY"), categoryGeneral, flagNoTrigger);
            break;
        case exportToAkm:
            result.setInfo(juce::translate("Export to AKM"), juce::translate("Export to AKM"), categoryGeneral, flagNoTrigger);
            break;
        case exportToFap:
            result.setInfo(juce::translate("Export to FAP"), juce::translate("Export to FAP"), categoryGeneral, flagNoTrigger);
            break;
        case exportToMidi:
            result.setInfo(juce::translate("Export to MIDI"), juce::translate("Export to MIDI"), categoryGeneral, flagNoTrigger);
            break;
        case exportToMod:
            result.setInfo(juce::translate("Export to MOD"), juce::translate("Export to MOD"), categoryGeneral, flagNoTrigger);
            break;
        case exportToText:
            result.setInfo(juce::translate("Export to text"), juce::translate("Export to text"), categoryGeneral, flagNoTrigger);
            break;
        case exportToYm:
            result.setInfo(juce::translate("Export to YM"), juce::translate("Export to YM"), categoryGeneral, flagNoTrigger);
            break;
        case exportYmToFap:
            result.setInfo(juce::translate("Export YM to FAP"), juce::translate("Export YM to FAP"), categoryGeneral, flagNoTrigger);
            break;
        case exportToVgm:
            result.setInfo(juce::translate("Export to VGM"), juce::translate("Export to VGM"), categoryGeneral, flagNoTrigger);
            break;
        case exportToWav:
            result.setInfo(juce::translate("Export to WAV"), juce::translate("Export to WAV"), categoryGeneral, flagNoTrigger);
            break;
        case exportInstrumentToWav:
            result.setInfo(juce::translate("Export instrument to WAV"), juce::translate("Export instrument to WAV"), categoryGeneral, flagNoTrigger);
            break;
        case exportSfxs:
            result.setInfo(juce::translate("Export sound effects"), juce::translate("Export sound effects"), categoryGeneral, flagNoTrigger);
            break;
        case exportEvents:
            result.setInfo(juce::translate("Export events"), juce::translate("Export events"), categoryGeneral, flagNoTrigger);
            break;
        case exportToRaw:
            result.setInfo(juce::translate("Export to RAW"), juce::translate("Export to RAW"), categoryGeneral, flagNoTrigger);
            break;
        case exportToRawLinear:
            result.setInfo(juce::translate("Export to RAW linear"), juce::translate("Export to RAW linear"), categoryGeneral, flagNoTrigger);
            break;
        case exportSamples:
            result.setInfo(juce::translate("Export samples"), juce::translate("Export samples"), categoryGeneral, flagNoTrigger);
            break;

        case quickExport:
            result.setInfo(juce::translate("Quick export"), juce::translate("Quick export for testing"), categoryGeneral, flagNoTrigger);
            break;
        case quickExportToBasic:
            result.setInfo(juce::translate("Quick export to Basic"), juce::translate("Quick export to Basic (CPC)"), categoryGeneral, flagNoTrigger);
            break;

        case openStreamedMusicAnalyzer:
            result.setInfo(juce::translate("Open the streamed music analyzer"), juce::translate("Open the streamed music analyzer"), categoryGeneral, flagNoTrigger);
            break;
        case openInstrumentOptimizer:
            result.setInfo(juce::translate("Open the instrument optimizer"), juce::translate("Open the instrument optimizer"), categoryGeneral, flagNoTrigger);
            break;
        case openErrorChecker:
            result.setInfo(juce::translate("Open the error checker"), juce::translate("Open the error checker"), categoryGeneral, flagNoTrigger);
            break;
        case rearrangePatterns:
            result.setInfo(juce::translate("Rearrange the patterns"), juce::translate("Rearrange the patterns"), categoryGeneral, flagNoTrigger);
            break;
        case clearPatterns:
            result.setInfo(juce::translate("Clears the patterns"), juce::translate("Clears the patterns"), categoryGeneral, flagNoTrigger);
            break;
        case rearrangeAndDeleteUnusedPatterns:
            result.setInfo(juce::translate("Deletes the unused patterns"), juce::translate("Deletes the unused patterns"), categoryGeneral, flagNoTrigger);
            break;

        case openAboutPage:
            result.setInfo(juce::translate("Open the About page"), juce::translate("Open the About page"), categoryGeneral, flagNoTrigger);
            break;

        case playSongFromStart:
            result.setInfo(juce::translate("Play from start"), juce::translate("Play the song from start"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F9Key, juce::ModifierKeys::noModifiers);
            break;
        case playSongContinue:
            result.setInfo(juce::translate("Continue playing"), juce::translate("Continue playing the song"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F10Key, juce::ModifierKeys::noModifiers);
            break;
        case playPatternFromStart:
            result.setInfo(juce::translate("Play pattern from start"), juce::translate("Play the pattern from start"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F11Key, juce::ModifierKeys::noModifiers);
            break;
        case playPatternFromCursorOrBlock:
            result.setInfo(juce::translate("Play pattern from cursor or block selection"), juce::translate("Play the pattern from the cursor or block selection"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F12Key, juce::ModifierKeys::noModifiers);
            break;
        case togglePlayPatternFromBlockIfPresentElsePlayPattern:
            result.setInfo(juce::translate("Toggle play pattern at block if present, else play pattern"), juce::translate("Toggle play pattern at block if present, else play pattern"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers);
            break;
        case stop:
            result.setInfo(juce::translate("Stop playing"), juce::translate("Stop playing"), categoryPlay, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::escapeKey, juce::ModifierKeys::noModifiers);
            break;

        case increaseOctave:
            result.setInfo(juce::translate("Increase octave"), juce::translate("Increase the octave"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress('+', juce::ModifierKeys::noModifiers);
            break;
        case decreaseOctave:
            result.setInfo(juce::translate("Decrease octave"), juce::translate("Decrease the octave"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(KeyCodes::keypadMinus(), juce::ModifierKeys::noModifiers);
            break;

        case previousInstrument:
            result.setInfo(juce::translate("Previous instrument"), juce::translate("Previous instrument"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::altModifier);
            break;
        case nextInstrument:
            result.setInfo(juce::translate("Next instrument"), juce::translate("Next instrument"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::altModifier);
            break;

        case editSongProperties:
            result.setInfo(juce::translate("Song properties"), juce::translate("Edit the song properties"), categoryGeneral, flagNoTrigger);
            break;

        case restoreLayout0:
            setResultForRestoreLayout(0, juce::KeyPress::F1Key, result, categoryGeneral);
            break;
        case restoreLayout1:
            setResultForRestoreLayout(1, juce::KeyPress::F2Key, result, categoryGeneral);
            break;
        case restoreLayout2:
            setResultForRestoreLayout(2, juce::KeyPress::F3Key, result, categoryGeneral);
            break;
        case restoreLayout3:
            setResultForRestoreLayout(3, juce::KeyPress::F4Key, result, categoryGeneral);
            break;

        case openLinker:
            result.setInfo(juce::translate("Open linker"), juce::translate("Open linker"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F1Key, juce::ModifierKeys::noModifiers);
            break;
        case openPatternViewer:
            result.setInfo(juce::translate("Open pattern viewer"), juce::translate("Open pattern viewer"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F2Key, juce::ModifierKeys::noModifiers);
            break;
        case openInstrumentList:
            result.setInfo(juce::translate("Open instrument list"), juce::translate("Open instrument list"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F3Key, juce::ModifierKeys::shiftModifier);
            break;
        case openInstrumentEditor:
            result.setInfo(juce::translate("Open instrument editor"), juce::translate("Open instrument editor"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F3Key, juce::ModifierKeys::noModifiers);
            break;
        case openArpeggioList:
            result.setInfo(juce::translate("Open arpeggio list"), juce::translate("Open arpeggio list"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F4Key, juce::ModifierKeys::shiftModifier);
            break;
        case openArpeggioTable:
            result.setInfo(juce::translate("Open arpeggio editor"), juce::translate("Open arpeggio editor"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F4Key, juce::ModifierKeys::noModifiers);
            break;
        case openPitchList:
            result.setInfo(juce::translate("Open pitch list"), juce::translate("Open pitch list"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F5Key, juce::ModifierKeys::shiftModifier);
            break;
        case openPitchTable:
            result.setInfo(juce::translate("Open pitch editor"), juce::translate("Open pitch editor"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F5Key, juce::ModifierKeys::noModifiers);
            break;
        case openTestArea:
            result.setInfo(juce::translate("Open test area"), juce::translate("Open test area"), categoryGeneral, flagNoTrigger);
            result.addDefaultKeypress(juce::KeyPress::F6Key, juce::ModifierKeys::noModifiers);
            break;
            // FIXME the empty page must have keyboard focus, else keyboard don't work.
            // FIXME Open does not give the focus (Linker for ex).

        /*case CommandIds::restoreLayout4:
            setResultForRestoreLayout(4, juce::KeyPress::F5Key, result, categoryGeneral);
            break;
        case CommandIds::restoreLayout5:
            setResultForRestoreLayout(5, juce::KeyPress::F6Key, result, categoryGeneral);
            break;
        case CommandIds::restoreLayout6:
            setResultForRestoreLayout(6, juce::KeyPress::F7Key, result, categoryGeneral);
            break;
        case CommandIds::restoreLayout7:
            setResultForRestoreLayout(7, juce::KeyPress::F8Key, result, categoryGeneral);
            break;
        case CommandIds::restoreLayout8:
            setResultForRestoreLayout(8, juce::KeyPress::F9Key, result, categoryGeneral);
            break;*/

        default:
            // Not abnormal. This might be another command managed by another object, such as the VirtualKeyboardCommands.
            break;
    }
}

void MainApplicationCommands::setResultForRestoreLayout(const int layoutIndex, const int keyPress, juce::ApplicationCommandInfo& result, const juce::String& category) noexcept
{
    constexpr auto commandFlags = juce::ApplicationCommandInfo::CommandFlags::dontTriggerVisualFeedback;
    const auto layoutIndexString = juce::String(layoutIndex + 1);       // Visually, from 1 to 9.
    result.setInfo(juce::translate("Change layout ") + layoutIndexString, translate("Show the layout " + layoutIndexString + "."),
                   category, commandFlags);
    result.addDefaultKeypress(keyPress, juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
}


}   // namespace arkostracker

