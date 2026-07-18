#include "MainControllerImpl.h"

#include "../app/preferences/PreferencesManager.h"
#include "../ui/arpeggioTableEditor/controller/ArpeggioTableEditorControllerImpl.h"
#include "../ui/arpeggioTableList/controller/ArpeggioTableListController.h"
#include "../ui/instrumentEditor/controller/InstrumentEditorControllerImpl.h"
#include "../ui/linker/controller/LinkerControllerImpl.h"
#include "../ui/mainView/MainComponent.h"
#include "../ui/mainView/operation/loadSong/SongInfoDialog.h"
#include "../ui/patternViewer//controller/PatternViewerControllerImpl.h"
#include "../ui/pitchTableEditor/controller/PitchTableEditorControllerImpl.h"
#include "../ui/pitchTableList/controller/PitchTableListController.h"
#include "../ui/testArea/controller/TestAreaControllerImpl.h"
#include "../ui/toolbar/main/controller/MainToolbarControllerImpl.h"
#include "../utils/FileUtil.h"
#include "../utils/NumberUtil.h"
#include "AudioControllerImpl.h"
#include "PlayerControllerImpl.h"
#include "SongControllerImpl.h"
#include "observers/ChannelMuteObserver.h"

namespace arkostracker 
{

MainControllerImpl::MainControllerImpl(const std::shared_ptr<Song>& pSong, Exiter& pExiter, StorageListener& pStorageListener) noexcept:
        exiter(pExiter),
        storageListener(pStorageListener),
        mainControllerObservers(),              // Used by the controllers below.
        mutedChannelIndexes(),
        channelsMute(),
        currentlySelectedArpeggioId(),
        currentlySelectedPitchId(),
        currentlySelectedInstrumentId(),
        currentlySelectedInstrumentIndexes(),
        songPath(),
        applicationCommandManager(),
        panelSearcher(),

        songController(std::make_unique<SongControllerImpl>(*this, pSong)),
        audioController(std::make_unique<AudioControllerImpl>(*this, pSong)),
        playerController(std::make_unique<PlayerControllerImpl>(*this)),        // Will need the SongController and AudioController.
        linkerController(),
        patternViewerController(),
        arpeggioTableListController(),
        pitchTableListController(),
        instrumentTableListController(),
        instrumentEditorController(),
        arpeggioTableEditorController(),
        mainToolbarController(),

        songLoader(),
        songSaver(),
        checkIfSongSavedAndSave(),
        songInfoDialog(),
        newSongManager(std::make_unique<NewSong>(*this)),
        songMerger(),

        serialController(*this),
        instrumentLoader(),

        alertDialog()
{
}

SongController& MainControllerImpl::getSongController() const noexcept
{
    return *songController;
}

PlayerController& MainControllerImpl::getPlayerController() const noexcept
{
    return *playerController;
}

SongPlayer& MainControllerImpl::getSongPlayer() const noexcept
{
    return audioController->getSongPlayer();
}

AudioController& MainControllerImpl::getAudioController() const noexcept
{
    return *audioController;
}

StorageListener& MainControllerImpl::getStorageListener() const noexcept
{
    return storageListener;
}


// MainController method implementations.
// =============================================

void MainControllerImpl::onWantToChangeMuteState(const int channelIndex, const bool newMuteState)
{
    if (newMuteState) {
        mutedChannelIndexes.insert(channelIndex);
    } else {
        mutedChannelIndexes.erase(channelIndex);
    }

    notifyChannelMuteObservers();
}

void MainControllerImpl::onWantToToggleMuteState(const int channelIndex)
{
    const auto currentMuteState = (mutedChannelIndexes.find(channelIndex) != mutedChannelIndexes.cend());
    onWantToChangeMuteState(channelIndex, !currentMuteState);
}

void MainControllerImpl::onWantToAllMuteExcept(const int channelIndex)
{
    const auto channelCount = songController->getChannelCount(getCurrentSubsongId());
    mutedChannelIndexes = channelsMute.toggleSoloOrUnmuteAll(mutedChannelIndexes, channelIndex, channelCount);

    notifyChannelMuteObservers();
}

void MainControllerImpl::unmuteAll()
{
    mutedChannelIndexes.clear();
    notifyChannelMuteObservers();
}

void MainControllerImpl::stopPlaying(bool /*notifyIfAlreadyStopped*/)
{
    const auto alreadyPlaying = playerController->isPlaying();

    // Stops in all cases. This stops the player if it was playing, BUT if not, it will stop the trailing notes.
    playerController->stopPlaying();

    // If was already stopped, notifies a specific observer.
    if (!alreadyPlaying) {
        observers().generalDataObservers.applyOnObservers([](GeneralDataObserver* observer) {
            observer->onGeneralDataChanged(GeneralDataObserver::What::doubleStopPlay);
        });
    }
}

void MainControllerImpl::togglePlayPatternFromTopOrBlock()
{
    getPatternViewerControllerInstance().onUserWantsToTogglePlayPatternFromCursorOrBlock(false);
}

void MainControllerImpl::switchToSubsong(const Id& subsongId) noexcept
{
    // Same Subsong?
    if (subsongId == songController->getCurrentSubsongId()) {
        return;
    }

    // Stops the player.
    playerController->stopPlaying();            // FIXME Not enough? This does not stop playing.

    const auto& song = songController->getSong();

    songController->setCurrentLocation({ subsongId, 0, 0 }, false);
    // Notifies the player. This works only if the setCurrentLocation has been set before.
    const Location playStartLocation(subsongId, 0);
    const auto loopStartAndPastEndPositions = song->getLoopStartAndPastEndPositions(subsongId);
    playerController->getSongPlayerObservers().applyOnObservers([&](SongPlayerObserver* observer) {
        const SongPlayerObserver::Locations locations(playStartLocation, loopStartAndPastEndPositions.first, loopStartAndPastEndPositions.second, playStartLocation);
        observer->onPlayerNewLocations(locations);
    });
}

void MainControllerImpl::notifyChannelMuteObservers() noexcept
{
    mainControllerObservers.channelMuteStateObservers.applyOnObservers([&](ChannelMuteObserver* observer) {
        observer->onChannelsMuteStateChanged(mutedChannelIndexes);
    });
}

std::unordered_set<int> MainControllerImpl::getChannelMuteStates() const noexcept
{
    return mutedChannelIndexes;
}

bool MainControllerImpl::canUndo() const
{
    return songController->canUndo();
}

bool MainControllerImpl::canRedo() const
{
    return songController->canRedo();
}

void MainControllerImpl::undo()
{
    songController->undo();
}

void MainControllerImpl::redo()
{
    songController->redo();
}

juce::String MainControllerImpl::getUndoDescription()
{
    return songController->getUndoDescription();
}

juce::String MainControllerImpl::getRedoDescription()
{
    return songController->getRedoDescription();
}

OptionalId MainControllerImpl::getSelectedExpressionId(const bool isArpeggio) const noexcept
{
    return isArpeggio ? currentlySelectedArpeggioId : currentlySelectedPitchId;
}

void MainControllerImpl::setSelectedExpressionId(const bool isArpeggio, const OptionalId& selectedExpressionId, const bool forceSingleSelection) noexcept
{
    // Stores the current expression.
    auto& currentlySelectedExpressionId = isArpeggio ? currentlySelectedArpeggioId : currentlySelectedPitchId;
    currentlySelectedExpressionId = selectedExpressionId;

    // Notifies.
    auto& selectedExpressionIndexObservers = isArpeggio ? mainControllerObservers.selectedArpeggioIndexObservers : mainControllerObservers.selectedPitchIndexObservers;
    selectedExpressionIndexObservers.applyOnObservers([&] (SelectedExpressionIndexObserver* observer) {
        observer->onSelectedExpressionChanged(isArpeggio, selectedExpressionId, forceSingleSelection);
    });
}

OptionalInt MainControllerImpl::getExpressionIndex(const bool isArpeggio, const Id& expressionId) const noexcept
{
    return songController->getExpressionIndex(isArpeggio, expressionId);
}

OptionalId MainControllerImpl::getSelectedInstrumentId() const noexcept
{
    return currentlySelectedInstrumentId;
}

std::vector<int> MainControllerImpl::getSelectedInstrumentIndexes() const noexcept
{
    // Complicated for nothing... Patch because on startup, the indexes are not updated.
    if (currentlySelectedInstrumentIndexes.empty()) {
        if (currentlySelectedInstrumentId.isPresent()) {
            std::vector<int> items;
            const auto index = getInstrumentIndex(currentlySelectedInstrumentId.getValue());
            if (index.isPresent()) {
                items.push_back(index.getValue());
            }

            return items;
        }
    }

    return currentlySelectedInstrumentIndexes;
}

OptionalInt MainControllerImpl::getInstrumentIndex(const Id& instrumentId) const noexcept
{
    return songController->getInstrumentIndex(instrumentId);
}

void MainControllerImpl::setSelectedInstrumentId(const OptionalId& selectedInstrumentId, const bool forceSingleSelectionForObservers) noexcept
{
    // Stores the new selection.
    currentlySelectedInstrumentId = selectedInstrumentId;

    // Notifies.
    auto& selectedObservers = mainControllerObservers.selectedInstrumentIndexObservers;
    selectedObservers.applyOnObservers([&] (SelectedInstrumentIndexObserver* observer) {
        observer->onSelectedInstrumentChanged(selectedInstrumentId, forceSingleSelectionForObservers);
    });
}

void MainControllerImpl::onSelectedInstrumentsChanged(const std::vector<int>& selectedIndexes) noexcept
{
    // Stores the selection.
    currentlySelectedInstrumentIndexes = selectedIndexes;

    auto& observer = mainControllerObservers.selectedInstrumentIndexesObserver;
    observer.applyOnObservers([&] (SelectedInstrumentIndexesObserver* localObserver) {
        localObserver->onSelectedInstrumentsChanged(selectedIndexes);
    });
}

void MainControllerImpl::setSelectedInstrumentFromIndex(int instrumentIndex) noexcept
{
    // Corrects the selection.
    const auto instrumentCount = songController->getInstrumentCount();
    instrumentIndex = NumberUtil::correctNumber(instrumentIndex, 0, instrumentCount - 1);
    const auto instrumentIdOptional = songController->getInstrumentId(instrumentIndex);
    if (instrumentIdOptional.isAbsent()) {
        jassertfalse;       // Not found? Abnormal.
        return;
    }

    setSelectedInstrumentId(instrumentIdOptional.getValue(), true);
}

void MainControllerImpl::selectNextOrPreviousInstrument(const bool next) noexcept
{
    auto instrumentIndex = 0;
    // If nothing is selected, sticks to the instrument index 0.
    if (currentlySelectedInstrumentId.isPresent()) {
        const auto foundInstrumentIndex = songController->getInstrumentIndex(currentlySelectedInstrumentId.getValue());
        instrumentIndex = foundInstrumentIndex.isAbsent() ? 0 : foundInstrumentIndex.getValue() + (next ? 1 : -1);
    }

    setSelectedInstrumentFromIndex(instrumentIndex);
}

void MainControllerImpl::setCommandManager(juce::ApplicationCommandManager& newApplicationCommandManager) noexcept
{
    applicationCommandManager = &newApplicationCommandManager;
}

juce::ApplicationCommandManager& MainControllerImpl::getCommandManager() const noexcept
{
    return *applicationCommandManager;
}

void MainControllerImpl::tryToExitApplication()
{
    // Tests if the song is modified before leaving.
    jassert(checkIfSongSavedAndSave == nullptr);        // Already present?
    checkIfSongSavedAndSave = std::make_unique<CheckIfSongSavedAndSave>(*this, [&](const bool success) {
        auto& localExiter = exiter;             // Gets the reference BEFORE deleting the object.
        checkIfSongSavedAndSave.reset();
        if (success) {
            localExiter.exitApplication();
        }
    });
    checkIfSongSavedAndSave->perform();
}

void MainControllerImpl::restoreLayout(const int layoutIndex) noexcept
{
    storageListener.onRestoreArrangement(layoutIndex);
}

Id MainControllerImpl::getCurrentSubsongId() const noexcept
{
    return songController->getCurrentSubsongId();
}

LinkerController& MainControllerImpl::getLinkerControllerInstance() noexcept
{
    if (linkerController == nullptr) {
        linkerController = std::make_unique<LinkerControllerImpl>(getSongController());
    }

    return *linkerController;
}

PatternViewerController& MainControllerImpl::getPatternViewerControllerInstance() noexcept
{
    if (patternViewerController == nullptr) {
        patternViewerController = std::make_unique<PatternViewerControllerImpl>(*this);
    }

    return *patternViewerController;
}

ListController& MainControllerImpl::getExpressionTableListController(const bool isArpeggio) noexcept
{
    ListController* controller;      // NOLINT(*-init-variables)

    if (isArpeggio) {
        if (arpeggioTableListController == nullptr) {
            arpeggioTableListController = std::make_unique<ArpeggioTableListController>(*this);
        }
        controller = arpeggioTableListController.get();
    } else {
        if (pitchTableListController == nullptr) {
            pitchTableListController = std::make_unique<PitchTableListController>(*this);
        }
        controller = pitchTableListController.get();
    }
    return *controller;
}

ListController& MainControllerImpl::getInstrumentTableListController() noexcept
{
    if (instrumentTableListController == nullptr) {
        instrumentTableListController = std::make_unique<InstrumentTableListControllerImpl>(*this);
    }

    return *instrumentTableListController;
}

InstrumentEditorController& MainControllerImpl::getInstrumentEditorController() noexcept
{
    if (instrumentEditorController == nullptr) {
        instrumentEditorController = std::make_unique<InstrumentEditorControllerImpl>(*this);
    }

    return *instrumentEditorController;
}

EditorWithBarsController& MainControllerImpl::getArpeggioTableEditorController() noexcept
{
    if (arpeggioTableEditorController == nullptr) {
        arpeggioTableEditorController = std::make_unique<ArpeggioTableEditorControllerImpl>(*this);
    }

    return *arpeggioTableEditorController;
}

EditorWithBarsController& MainControllerImpl::getPitchTableEditorController() noexcept
{
    if (pitchTableEditorController == nullptr) {
        pitchTableEditorController = std::make_unique<PitchTableEditorControllerImpl>(*this);
    }

    return *pitchTableEditorController;
}

MainToolbarController& MainControllerImpl::getMainToolbarControllerInstance() noexcept
{
    if (mainToolbarController == nullptr) {
        mainToolbarController = std::make_unique<MainToolbarControllerImpl>(*this);
    }

    return *mainToolbarController;
}

TestAreaController& MainControllerImpl::getTestAreaControllerInstance() noexcept
{
    if (testAreaController == nullptr) {
        testAreaController = std::make_unique<TestAreaControllerImpl>(*this);
    }

    return *testAreaController;
}

MainControllerObservers& MainControllerImpl::observers() noexcept
{
    return mainControllerObservers;
}

void MainControllerImpl::setPanelSearcher(PanelSearcher* newPanelSearcher) noexcept
{
    panelSearcher = newPanelSearcher;
}

PanelSearcher* MainControllerImpl::getPanelSearcher() const noexcept
{
    return panelSearcher;
}

bool MainControllerImpl::searchAndOpenPanel(const PanelType panelType) noexcept
{
    if (panelSearcher == nullptr) {
        jassertfalse;       // PanelSearcher not present.
        return false;
    }

    return panelSearcher->searchAndOpenPanel(panelType);
}

void MainControllerImpl::loadSong(const juce::String& pathIfKnown) noexcept
{
    songLoader = std::make_unique<LoadSong>(*this, *this, true);
    songLoader->startLoadSongProcess(pathIfKnown);
}

void MainControllerImpl::mergeSong() noexcept
{
    songMerger = std::make_unique<MergeSong>(*this);
    songMerger->merge();
}

void MainControllerImpl::loadInstrument() noexcept
{
    instrumentLoader = std::make_unique<LoadInstrument>(*this);
    instrumentLoader->load();
}


// LoadInstrument::Listener method implementations.
// ===================================================

void MainControllerImpl::onLoadInstrumentFinished(std::unique_ptr<Instrument> loadedInstrument) noexcept
{
    instrumentLoader.reset();

    if (loadedInstrument != nullptr) {
        songController->addInstrument(std::move(loadedInstrument));
    }
}

void MainControllerImpl::onLoadInstrumentCancelled() noexcept
{
    instrumentLoader.reset();
}


// LoadSong::Listener method implementations.
// ===================================================

void MainControllerImpl::onLoadedSongSuccess(std::unique_ptr<Song> song)
{
    songLoader.reset();

    changeSong(std::move(song), true, false);
}

void MainControllerImpl::onLoadedSongFailure()
{
    songLoader.reset();
}

void MainControllerImpl::saveSong(bool saveAs) noexcept
{
    jassert(songSaver == nullptr);     // Already present?
    songSaver = std::make_unique<SaveSong>(saveAs, *this, [&](const bool success) {
        songSaver.reset();
        jassert(success); (void)success;
    });
    songSaver->saveSong();
}

void MainControllerImpl::saveCopy() noexcept
{
    // First, saves the song to be sure the file on disk is up to date.
    jassert(checkIfSongSavedAndSave == nullptr);        // Already present?
    checkIfSongSavedAndSave = std::make_unique<CheckIfSongSavedAndSave>(*this, [&](const bool success) {
        jassert(success);
        if (success) {
            performSaveCopy();
        }
        checkIfSongSavedAndSave.reset();        // MUST be done after, else crash.
    });
    checkIfSongSavedAndSave->perform();
}

void MainControllerImpl::performSaveCopy() noexcept
{
    if (songPath.isEmpty()) {
        jassertfalse;           // Security, shouldn't happen, the UI should prevent this.
        showSaveCopyFailed();
        return;
    }

    // Determines the new file name.
    const auto sourceFile = FileUtil::getFileFromString(songPath);
    if (!sourceFile.existsAsFile()) {
        showSaveCopyFailed();
        return;
    }

    // Removes the possible index.
    const auto fullPathWithoutCopyIndex = FileUtil::getFileWithoutCopyIndex(sourceFile);
    const auto pathAndFileWithoutExtension = fullPathWithoutCopyIndex.getParentDirectory().getFullPathName() + "/" + fullPathWithoutCopyIndex.getFileNameWithoutExtension();
    auto copyNumber = 0;
    auto fileAlreadyExist = true;
    juce::String newFileName;
    // Increases a counter, finds a file that doesn't exist with this counter appended to the file name.
    while (fileAlreadyExist) {
        ++copyNumber;
        newFileName = pathAndFileWithoutExtension + " (" + juce::String(copyNumber) + ")" + sourceFile.getFileExtension();
        const auto newFile = FileUtil::getFileFromString(newFileName);
        fileAlreadyExist = newFile.existsAsFile();
    }

    // Copies the song file... It must exist!
    const auto success = sourceFile.copyFileTo(newFileName);
    if (!success) {
        showSaveCopyFailed();
    }
}

void MainControllerImpl::showSaveCopyFailed() noexcept
{
    alertDialog = SuccessOrErrorDialog::buildForError(juce::translate("Saving of a copy failed!"),
        [&] {
            alertDialog.reset();
        });
}

void MainControllerImpl::newSong() noexcept
{
    // Song modified?
    jassert(checkIfSongSavedAndSave == nullptr);        // Already present?
    checkIfSongSavedAndSave = std::make_unique<CheckIfSongSavedAndSave>(*this, [&](const bool success) {
        if (success) {
            newSongAfterModificationPrompt();
        }
        checkIfSongSavedAndSave.reset();        // MUST be done after, else crash.
    });
    checkIfSongSavedAndSave->perform();
}

void MainControllerImpl::newSongAfterModificationPrompt() const noexcept
{
    newSongManager->showDialog();
}


// NewSong::Listener method implementations.
// ===================================================

void MainControllerImpl::onNewSongValidatedOrCanceled(std::unique_ptr<Song> newSong) noexcept
{
    if (newSong != nullptr) {
        // Important to clear the path, else the new song will use the previously loaded song path!
        changeSong(std::move(newSong), false, true);
    }
}


// ===================================================

void MainControllerImpl::changeSong(std::unique_ptr<Song> newSong, const bool allowedToShowSongInfoDialog, bool clearSongPath) noexcept
{
    const std::shared_ptr sharedSong = std::move(newSong);
    const auto subsongId = sharedSong->getFirstSubsongId();

    songController->setSong(sharedSong);
    audioController->stopPlayerAndSetSong(sharedSong);

    if (clearSongPath) {
        songPath = juce::String();
    }

    // Clears the Undo.
    songController->clearUndo();

    notifyAllChanges(subsongId);

    // Shows the Song Info Dialog, if authorized and wanted.
    if (allowedToShowSongInfoDialog && PreferencesManager::getInstance().isSongInfoShownAfterLoad()) {
        const auto songTitle = sharedSong->getTitle();
        const auto author = sharedSong->getAuthor();
        const auto composer = sharedSong->getComposer();
        const auto comments = sharedSong->getComments();
        songInfoDialog = std::make_unique<SongInfoDialog>(songTitle, author, composer, comments, [&] {
            songInfoDialog.reset();
        });
    }

    playerController->stopPlaying();        // To stop playing expressions and sounds from the previous song, to stop assertions. Not perfect, but ok.
}

void MainControllerImpl::notifyAllChanges(const Id& subsongId) noexcept
{
    const auto& sharedSong = songController->getSong();

    // Notifies everyone.
    songController->getSongMetadataObservers().applyOnObservers([](SongMetadataObserver* observer) {
        observer->onSongMetadataChanged(SongMetadataObserver::What::all);        // Forced to have the meters being resized if needed.
    });
    songController->getSubsongMetadataObservers().applyOnObservers([&](SubsongMetadataObserver* observer) {
        observer->onSubsongMetadataChanged(subsongId, SubsongMetadataObserver::What::all);
    });

    const Location playStartLocation(subsongId, 0);
    const auto loopStartAndPastEndPositions = sharedSong->getLoopStartAndPastEndPositions(subsongId);
    playerController->getSongPlayerObservers().applyOnObservers([&](SongPlayerObserver* observer) {
        const SongPlayerObserver::Locations locations(playStartLocation, loopStartAndPastEndPositions.first, loopStartAndPastEndPositions.second, playStartLocation);
        observer->onPlayerNewLocations(locations);
    });

    songController->getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) {
       observer->onLinkerPositionsInvalidated(subsongId, {});
    });
    songController->getInstrumentObservers().applyOnObservers([](InstrumentChangeObserver* observer) {
        observer->onInstrumentsInvalidated();
    });
    songController->getExpressionObservers(true).applyOnObservers([](ExpressionChangeObserver* observer) {
        observer->onExpressionsInvalidated();
    });
    songController->getExpressionObservers(false).applyOnObservers([](ExpressionChangeObserver* observer) {
        observer->onExpressionsInvalidated();
    });

    unmuteAll();

    // Selected the "second" instrument, if any.
    const auto instrumentIds = sharedSong->getInstrumentIds();
    jassert(!instrumentIds.empty());        // Should never happen!
    const auto instrumentId = instrumentIds.size() > 1U ? instrumentIds.at(1U) : instrumentIds.at(0U);
    setSelectedInstrumentId(instrumentId, true);
    // Selects 0th Expression (stay neutral).
    const auto arpeggio0Id = sharedSong->getExpressionHandler(true).getId(0);
    setSelectedExpressionId(true, arpeggio0Id, true);
    const auto pitch0Id = sharedSong->getExpressionHandler(false).getId(0);
    setSelectedExpressionId(false, pitch0Id, true);
    jassert(arpeggio0Id.isPresent());           // Should never happen!
    jassert(pitch0Id.isPresent());           // Should never happen!
}

void MainControllerImpl::clearRecentFiles() noexcept
{
    PreferencesManager::getInstance().clearRecentFiles();
}

juce::String MainControllerImpl::getSongPath() noexcept
{
    return songPath;
}

void MainControllerImpl::setSongPathAndNotify(juce::String newPath) noexcept
{
    if (newPath == songPath) {
        return;
    }
    songPath = std::move(newPath);

    // Notifies listeners of the change.
    songController->getSongMetadataObservers().applyOnObservers([](SongMetadataObserver* observer) {
        observer->onSongMetadataChanged(SongMetadataObserver::What::savedPath);
    });
}

void MainControllerImpl::markSongAsSaved() noexcept
{
    songController->markSongAsSaved();
}

bool MainControllerImpl::isSongModified() const noexcept
{
    return songController->isSongModified();
}

void MainControllerImpl::setOctave(const int desiredOctave) noexcept
{
    playerController->setOctave(desiredOctave);

    // Notifies the listeners.
    observers().getGeneralDataObservers().applyOnObservers([](GeneralDataObserver* observer) {
        observer->onGeneralDataChanged(GeneralDataObserver::What::octave);
    });
}

int MainControllerImpl::getCurrentOctave() const noexcept
{
    return playerController->getCurrentOctave();
}

void MainControllerImpl::increaseOctave(const int offset) noexcept
{
    const auto currentOctave = playerController->getCurrentOctave();
    setOctave(currentOctave + offset);
}

void MainControllerImpl::onUserWantsToStartStopSerial() noexcept
{
    serialController.onUserWantsToStartStopSerial();
}

bool MainControllerImpl::canSaveCopy() const noexcept
{
    return songPath.isNotEmpty();
}

bool MainControllerImpl::tryToRecordExternalNote(const int note) noexcept
{
    if (patternViewerController->isRecording() && isPanelVisible(PanelType::patternViewer)) {
        patternViewerController->onNote(note);

        return true;
    }

    return false;
}

// ReSharper disable once CppDFAConstantParameter
bool MainControllerImpl::isPanelVisible(const PanelType panelType) const noexcept
{
    const auto* localPanelSearcher = getPanelSearcher();
    if (localPanelSearcher == nullptr) {
        jassertfalse;       // PanelSearcher not present.
        return false;
    }

    return localPanelSearcher->isPanelVisible(panelType);
}

}   // namespace arkostracker
