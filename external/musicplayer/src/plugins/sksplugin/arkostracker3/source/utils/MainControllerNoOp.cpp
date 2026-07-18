#include "MainControllerNoOp.h"

namespace arkostracker 
{

MainControllerNoOp::MainControllerNoOp() noexcept :
        applicationCommandManager(),
        songController(),
        linkerController(),
        playerController(),
        patternViewerController(),
        songPlayer(),
        audioController(),
        expressionTableListController(),
        instrumentTableListController(),
        instrumentEditorController(),
        arpeggioTableEditorController(),
        mainToolbarController(),
        testAreaController(),

        mainControllerObservers()
{
}

SongController& MainControllerNoOp::getSongController() const noexcept
{
    return *songController;
}

juce::ApplicationCommandManager& MainControllerNoOp::getCommandManager() const noexcept
{
    return *applicationCommandManager;
}

void MainControllerNoOp::setCommandManager(juce::ApplicationCommandManager& /*applicationCommandManager*/) noexcept
{
}

void MainControllerNoOp::tryToExitApplication()
{

}

void MainControllerNoOp::onWantToChangeMuteState(int /*channelIndex*/, bool /*newMuteState*/)
{

}

void MainControllerNoOp::onWantToAllMuteExcept(int /*channelIndex*/)
{

}

bool MainControllerNoOp::canUndo() const
{
    return true;        // True is useful for the Setup of the keyboard mapping.
}

bool MainControllerNoOp::canRedo() const
{
    return true;        // True is useful for the Setup of the keyboard mapping.
}

void MainControllerNoOp::undo()
{

}

void MainControllerNoOp::redo()
{

}

juce::String MainControllerNoOp::getUndoDescription()
{
    return { };
}

juce::String MainControllerNoOp::getRedoDescription()
{
    return { };
}

void MainControllerNoOp::restoreLayout(int /*layoutIndex*/) noexcept
{

}

std::unordered_set<int> MainControllerNoOp::getChannelMuteStates() const noexcept
{
    return { };
}

OptionalId MainControllerNoOp::getSelectedExpressionId(bool /*isArpeggio*/) const noexcept
{
    return { };
}

void MainControllerNoOp::setSelectedExpressionId(bool /*isArpeggio*/, const OptionalId& /*selectedExpressionId*/, bool /*forceSingleSelectionForObservers*/) noexcept
{
}

OptionalInt MainControllerNoOp::getExpressionIndex(bool /*isArpeggio*/, const Id& /*expressionId*/) const noexcept
{
    return { };
}

void MainControllerNoOp::unmuteAll()
{
}

LinkerController& MainControllerNoOp::getLinkerControllerInstance() noexcept
{
    return *linkerController;
}

PlayerController& MainControllerNoOp::getPlayerController() const noexcept
{
    return *playerController;
}

SongPlayer& MainControllerNoOp::getSongPlayer() const noexcept
{
    return *songPlayer;
}

PatternViewerController& MainControllerNoOp::getPatternViewerControllerInstance() noexcept
{
    return *patternViewerController;
}

void MainControllerNoOp::onWantToToggleMuteState(int /*channelIndex*/)
{
}

MainControllerObservers& MainControllerNoOp::observers() noexcept
{
    return mainControllerObservers;
}

AudioController& MainControllerNoOp::getAudioController() const noexcept
{
    return *audioController;
}

ListController& MainControllerNoOp::getExpressionTableListController(bool /*isArpeggio*/) noexcept
{
    return *expressionTableListController;
}

OptionalId MainControllerNoOp::getSelectedInstrumentId() const noexcept
{
    return { };
}

std::vector<int> MainControllerNoOp::getSelectedInstrumentIndexes() const noexcept
{
    return { };
}

OptionalInt MainControllerNoOp::getInstrumentIndex(const Id& /*instrumentId*/) const noexcept
{
    return { };
}

void MainControllerNoOp::setSelectedInstrumentId(const OptionalId& /*selectedInstrumentId*/, bool /*forceSingleSelectionForObservers*/) noexcept
{
}

void MainControllerNoOp::onSelectedInstrumentsChanged(const std::vector<int>& /*selectedIndexes*/) noexcept
{
}

void MainControllerNoOp::setSelectedInstrumentFromIndex(int /*index*/) noexcept
{
}

void MainControllerNoOp::selectNextOrPreviousInstrument(bool /*next*/) noexcept
{
}

ListController& MainControllerNoOp::getInstrumentTableListController() noexcept
{
    return *instrumentTableListController;
}

InstrumentEditorController& MainControllerNoOp::getInstrumentEditorController() noexcept
{
    return *instrumentEditorController;
}

void MainControllerNoOp::setPanelSearcher(PanelSearcher* /*panelSearcher*/) noexcept
{
}

PanelSearcher* MainControllerNoOp::getPanelSearcher() const noexcept
{
    return nullptr;
}

bool MainControllerNoOp::searchAndOpenPanel(PanelType /*panelType*/) noexcept
{
    return false;
}

EditorWithBarsController& MainControllerNoOp::getArpeggioTableEditorController() noexcept
{
    return *arpeggioTableEditorController;
}

EditorWithBarsController& MainControllerNoOp::getPitchTableEditorController() noexcept
{
    return *pitchTableEditorController;
}

MainToolbarController& MainControllerNoOp::getMainToolbarControllerInstance() noexcept
{
    return *mainToolbarController;
}

StorageListener& MainControllerNoOp::getStorageListener() const noexcept
{
    return *storageListener;
}

void MainControllerNoOp::changeSong(std::unique_ptr<Song> /*newSong*/, bool /*allowedToShowSongInfoDialog*/, bool /*clearSongPath*/) noexcept
{
}

void MainControllerNoOp::notifyAllChanges(const Id& /*subsongId*/) noexcept
{
}

void MainControllerNoOp::clearRecentFiles() noexcept
{
}

void MainControllerNoOp::loadSong(const juce::String& /*pathIfKnown*/) noexcept
{
}

void MainControllerNoOp::mergeSong() noexcept
{
}

void MainControllerNoOp::saveSong(bool /*saveAs*/) noexcept
{
}

void MainControllerNoOp::saveCopy() noexcept
{
}

juce::String MainControllerNoOp::getSongPath() noexcept
{
    return { };
}

void MainControllerNoOp::setSongPathAndNotify(juce::String /*path*/) noexcept
{
}

void MainControllerNoOp::markSongAsSaved() noexcept
{
}

bool MainControllerNoOp::isSongModified() const noexcept
{
    return false;
}

void MainControllerNoOp::newSong() noexcept
{
}

void MainControllerNoOp::loadInstrument() noexcept
{
}

void MainControllerNoOp::setOctave(int /*desiredOctave*/) noexcept
{
}

int MainControllerNoOp::getCurrentOctave() const noexcept
{
    return 0;
}

void MainControllerNoOp::increaseOctave(int /*offset*/) noexcept
{
}

bool MainControllerNoOp::tryToRecordExternalNote(int /*note*/) noexcept
{
    return false;
}

void MainControllerNoOp::stopPlaying(bool /*notifyIfAlreadyStopped*/)
{
}

void MainControllerNoOp::togglePlayPatternFromTopOrBlock()
{
}

void MainControllerNoOp::switchToSubsong(const Id& /*subsongId*/) noexcept
{
}

TestAreaController& MainControllerNoOp::getTestAreaControllerInstance() noexcept
{
    return *testAreaController;
}

void MainControllerNoOp::onUserWantsToStartStopSerial() noexcept
{
}

bool MainControllerNoOp::canSaveCopy() const noexcept
{
    return false;
}

}   // namespace arkostracker
