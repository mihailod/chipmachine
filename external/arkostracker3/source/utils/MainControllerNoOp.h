#pragma once

#include <memory>

#include "../controllers/AudioController.h"
#include "../controllers/MainController.h"
#include "../controllers/PlayerController.h"
#include "../player/SongPlayer.h"
#include "../ui/containerArranger/StorageListener.h"
#include "../ui/editorWithBars/controller/EditorWithBarsController.h"
#include "../ui/instrumentEditor/controller/InstrumentEditorController.h"
#include "../ui/linker/controller/LinkerController.h"
#include "../ui/lists/controller/ListController.h"
#include "../ui/patternViewer/controller/PatternViewerController.h"
#include "../ui/testArea/controller/TestAreaController.h"
#include "../ui/toolbar/main/controller/MainToolbarController.h"

namespace arkostracker 
{

/** A MainController that does nothing. */
class MainControllerNoOp final : public MainController
{
public:
    /** Constructor. */
    MainControllerNoOp() noexcept;

    void changeSong(std::unique_ptr<Song> newSong, bool allowedToShowSongInfoDialog, bool clearSongPath) noexcept override;
    void notifyAllChanges(const Id& subsongId) noexcept override;
    void loadSong(const juce::String& pathIfKnown) noexcept override;
    void mergeSong() noexcept override;
    void saveSong(bool saveAs) noexcept override;
    void saveCopy() noexcept override;
    void newSong() noexcept override;
    void loadInstrument() noexcept override;
    juce::String getSongPath() noexcept override;
    void setSongPathAndNotify(juce::String path) noexcept override;
    void markSongAsSaved() noexcept override;
    bool isSongModified() const noexcept override;

    SongController& getSongController() const noexcept override;
    PlayerController& getPlayerController() const noexcept override;
    SongPlayer& getSongPlayer() const noexcept override;
    AudioController& getAudioController() const noexcept override;
    StorageListener& getStorageListener() const noexcept override;

    juce::ApplicationCommandManager& getCommandManager() const noexcept override;
    void setCommandManager(juce::ApplicationCommandManager& applicationCommandManager) noexcept override;
    void setPanelSearcher(PanelSearcher* panelSearcher) noexcept override;
    PanelSearcher* getPanelSearcher() const noexcept override;
    bool searchAndOpenPanel(PanelType panelType) noexcept override;

    void tryToExitApplication() override;
    void onWantToChangeMuteState(int channelIndex, bool newMuteState) override;
    void onWantToToggleMuteState(int channelIndex) override;
    void onWantToAllMuteExcept(int channelIndex) override;
    void unmuteAll() override;
    void stopPlaying(bool notifyIfAlreadyStopped) override;
    void togglePlayPatternFromTopOrBlock() override;
    void switchToSubsong(const Id& subsongId) noexcept override;

    void onUserWantsToStartStopSerial() noexcept override;

    MainControllerObservers& observers() noexcept override;

    bool canUndo() const override;
    bool canRedo() const override;
    void undo() override;
    void redo() override;
    juce::String getUndoDescription() override;
    juce::String getRedoDescription() override;
    void restoreLayout(int layoutIndex) noexcept override;
    std::unordered_set<int> getChannelMuteStates() const noexcept override;
    void setOctave(int desiredOctave) noexcept override;
    int getCurrentOctave() const noexcept override;
    void increaseOctave(int offset) noexcept override;
    bool tryToRecordExternalNote(int note) noexcept override;

    OptionalId getSelectedExpressionId(bool isArpeggio) const noexcept override;
    void setSelectedExpressionId(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelectionForObservers) noexcept override;
    OptionalInt getExpressionIndex(bool isArpeggio, const Id& expressionId) const noexcept override;
    OptionalId getSelectedInstrumentId() const noexcept override;
    std::vector<int> getSelectedInstrumentIndexes() const noexcept override;
    OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept override;
    void setSelectedInstrumentId(const OptionalId& selectedInstrumentId, bool forceSingleSelectionForObservers) noexcept override;
    void onSelectedInstrumentsChanged(const std::vector<int>& selectedIndexes) noexcept override;
    void setSelectedInstrumentFromIndex(int index) noexcept override;
    void selectNextOrPreviousInstrument(bool next) noexcept override;

    LinkerController& getLinkerControllerInstance() noexcept override;
    PatternViewerController& getPatternViewerControllerInstance() noexcept override;
    ListController& getExpressionTableListController(bool isArpeggio) noexcept override;
    ListController& getInstrumentTableListController() noexcept override;
    InstrumentEditorController& getInstrumentEditorController() noexcept override;
    EditorWithBarsController& getArpeggioTableEditorController() noexcept override;
    EditorWithBarsController& getPitchTableEditorController() noexcept override;
    MainToolbarController& getMainToolbarControllerInstance() noexcept override;

    TestAreaController& getTestAreaControllerInstance() noexcept override;

    void clearRecentFiles() noexcept override;

    bool canSaveCopy() const noexcept override;

private:
    juce::ApplicationCommandManager* applicationCommandManager;
    std::unique_ptr<SongController> songController;
    std::unique_ptr<LinkerController> linkerController;
    std::unique_ptr<PlayerController> playerController;
    std::unique_ptr<PatternViewerController> patternViewerController;
    std::unique_ptr<SongPlayer> songPlayer;
    std::unique_ptr<AudioController> audioController;
    std::unique_ptr<ListController> expressionTableListController;
    std::unique_ptr<ListController> instrumentTableListController;
    std::unique_ptr<InstrumentEditorController> instrumentEditorController;
    std::unique_ptr<EditorWithBarsController> arpeggioTableEditorController;
    std::unique_ptr<EditorWithBarsController> pitchTableEditorController;
    std::unique_ptr<MainToolbarController> mainToolbarController;
    std::unique_ptr<StorageListener> storageListener;
    std::unique_ptr<TestAreaController> testAreaController;

    MainControllerObservers mainControllerObservers;
};

}   // namespace arkostracker

