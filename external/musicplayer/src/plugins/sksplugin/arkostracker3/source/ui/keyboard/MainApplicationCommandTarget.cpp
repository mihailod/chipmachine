#include "MainApplicationCommandTarget.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../controllers/MainController.h"
#include "../../controllers/PlayerController.h"
#include "../mainView/CommandPerformer.h"
#include "CommandIds.h"

namespace arkostracker 
{

MainApplicationCommandTarget::MainApplicationCommandTarget(MainController& pMainController, CommandPerformer& pCommandPerformer) noexcept:
        mainController(pMainController),
        commandPerformer(pCommandPerformer),
        mainApplicationCommands(pMainController),
        virtualKeyboardCommands(PreferencesManager::getInstance().getVirtualKeyboardLayout())
{
}

juce::ApplicationCommandTarget* MainApplicationCommandTarget::getNextCommandTarget()
{
    return nullptr;
}

void MainApplicationCommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    virtualKeyboardCommands.getAllCommands(commands);       // Note: works very fine, but "a" is forced by the Main, not the PV. Any way to prioritize? Doesn't seem necessary.
    mainApplicationCommands.getAllCommands(commands);
}

void MainApplicationCommandTarget::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    virtualKeyboardCommands.getCommandInfo(commandID, result);
    mainApplicationCommands.getCommandInfo(commandID, result);
}

bool MainApplicationCommandTarget::perform(const InvocationInfo& info)
{
    auto success = true;

    // Performs an action.
    switch (info.commandID) {
        case juce::StandardApplicationCommandIDs::quit:
            mainController.tryToExitApplication();
            break;
        case juce::StandardApplicationCommandIDs::undo:
            mainController.undo();
            break;
        case juce::StandardApplicationCommandIDs::redo:
            mainController.redo();
            break;
        case loadSong:
            commandPerformer.loadSong();
            break;
        case mergeSong:
            commandPerformer.mergeSong();
            break;
        case newSong: {
            commandPerformer.newSong();
            break;
        }
        case saveSong: {
            commandPerformer.saveSong();
            break;
        }
        case saveSongAs: {
            commandPerformer.saveSongAs();
            break;
        }
        case saveCopy: {
            commandPerformer.saveCopy();
            break;
        }
        case clearRecentFiles: {
            commandPerformer.clearRecentFiles();
            break;
        }
        case loadInstrument: {
            commandPerformer.loadInstrument();
            break;
        }
        case setupGeneral:
            commandPerformer.openGeneralSetup();
            break;
        case setupKeyboardMapping:
            commandPerformer.openKeyboardMappingSetup();
            break;
        case setupThemeEditor:
            commandPerformer.openThemeEditor();
            break;
        case setupPatternViewer:
            commandPerformer.openPatternViewerSetup();
            break;
        case setupAudioInterface:
            commandPerformer.openAudioInterfaceSetup();
            break;
        case setupOutput:
            commandPerformer.openOutputSetup();
            break;
        case setupSourceProfile:
            commandPerformer.openSourceProfileSetup();
            break;
        case setupSerial:
            commandPerformer.openSerialSetup();
            break;

        case exportToAkg:
            commandPerformer.openExportToAkg();
            break;
        case exportToAky:
            commandPerformer.openExportToAky();
            break;
        case exportToAkm:
            commandPerformer.openExportToAkm();
            break;
        case exportToFap:
            commandPerformer.openExportToFap();
            break;
        case exportToMidi:
            commandPerformer.openExportToMidi();
            break;
        case exportToMod:
            commandPerformer.openExportToMod();
            break;
        case exportToText:
            commandPerformer.openExportToText();
            break;
        case exportToYm:
            commandPerformer.openExportToYm();
            break;
        case exportYmToFap:
            commandPerformer.openExportYmToFap();
            break;
        case exportToVgm:
            commandPerformer.openExportToVgm();
            break;
        case exportToWav:
            commandPerformer.openExportToWav();
            break;
        case exportInstrumentToWav:
            commandPerformer.openExportInstrumentToWav();
            break;
        case exportSfxs:
            commandPerformer.openExportSfxs();
            break;
        case exportEvents:
            commandPerformer.openExportEvents();
            break;
        case exportToRaw:
            commandPerformer.openExportToRaw();
            break;
        case exportToRawLinear:
            commandPerformer.openExportToRawLinear();
            break;
        case exportSamples:
            commandPerformer.openExportSamples();
            break;

        case quickExport:
            commandPerformer.openQuickExport();
            break;
        case quickExportToBasic:
            commandPerformer.openQuickExportToBasic();
            break;

        case openStreamedMusicAnalyzer:
            commandPerformer.openStreamedMusicAnalyzer();
            break;
        case openInstrumentOptimizer:
            commandPerformer.openInstrumentOptimizer();
            break;
        case openErrorChecker:
            commandPerformer.openErrorChecker();
            break;
        case rearrangePatterns:
            commandPerformer.rearrangePatterns(false);
            break;
        case clearPatterns:
            commandPerformer.clearPatterns();
            break;
        case rearrangeAndDeleteUnusedPatterns:
            commandPerformer.rearrangePatterns(true);
            break;

        case openAboutPage:
            commandPerformer.openAboutPage();
            break;

        case playSongFromStart:
            mainController.getPlayerController().playSongFromStart();
            break;
        case playSongContinue:
            mainController.getPlayerController().playSongFromCurrentLocation();
            break;
        case playPatternFromStart:
            mainController.getPlayerController().playPatternFromStart();
            break;
        case playPatternFromCursorOrBlock:
            mainController.getPlayerController().playPatternFromShownLocationOrBlock();
            break;
        case togglePlayPatternFromBlockIfPresentElsePlayPattern:
            mainController.togglePlayPatternFromTopOrBlock();
            break;
        case stop:
            mainController.stopPlaying(true);
            break;

        case increaseOctave:
            mainController.increaseOctave(1);
            break;
        case decreaseOctave:
            mainController.increaseOctave(-1);
            break;

        case previousInstrument:
            mainController.selectNextOrPreviousInstrument(false);
            break;
        case nextInstrument:
            mainController.selectNextOrPreviousInstrument(true);
            break;

        case editSongProperties:
            commandPerformer.editSongProperties();
            break;

        case restoreLayout0:
            mainController.restoreLayout(0);
            break;
        case restoreLayout1:
            mainController.restoreLayout(1);
            break;
        case restoreLayout2:
            mainController.restoreLayout(2);
            break;
        case restoreLayout3:
            mainController.restoreLayout(3);
            break;

        case openLinker:
            commandPerformer.openPanel(PanelType::linker);
            break;
        case openInstrumentEditor:
            commandPerformer.openPanel(PanelType::instrumentEditor);
            break;
        case openPatternViewer:
            commandPerformer.openPanel(PanelType::patternViewer);
            break;
        case openInstrumentList:
            commandPerformer.openPanel(PanelType::instrumentList);
            break;
        case openArpeggioTable:
            commandPerformer.openPanel(PanelType::arpeggioTable);
            break;
        case openArpeggioList:
            commandPerformer.openPanel(PanelType::arpeggioList);
            break;
        case openPitchTable:
            commandPerformer.openPanel(PanelType::pitchTable);
            break;
        case openPitchList:
            commandPerformer.openPanel(PanelType::pitchList);
            break;
        case openTestArea:
            commandPerformer.openPanel(PanelType::testArea);
            break;
        default:
            success = false;
            break;
    }

    // If command not found, maybe it is a virtual keyboard key?
    if (!success) {
        if (const auto baseNote = virtualKeyboardCommands.performForNote(info); baseNote >= 0) {
            success = true;
            mainController.getPlayerController().play(baseNote, true, { });
            DBG("KEYBOARD BASE NOTE from MAIN: " + juce::String(baseNote));
        }
    }

    if (!success) {
        jassertfalse;       // The command isn't known: not normal (oversight?).
    }

    return success;
}

}   // namespace arkostracker
