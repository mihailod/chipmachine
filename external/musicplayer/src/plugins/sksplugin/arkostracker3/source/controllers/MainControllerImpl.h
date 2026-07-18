#pragma once

#include <memory>

#include "../business/channels/ChannelsMute.h"
#include "../ui/editorWithBars/controller/EditorWithBarsController.h"
#include "../ui/instrumentEditor/controller/InstrumentEditorController.h"
#include "../ui/instrumentList/controller/InstrumentTableListControllerImpl.h"
#include "../ui/linker/controller/LinkerController.h"
#include "../ui/mainView/operation/CheckIfSongSavedAndSave.h"
#include "../ui/mainView/operation/SaveSong.h"
#include "../ui/mainView/operation/loadSong/LoadSong.h"
#include "../ui/newSong/NewSong.h"
#include "../ui/patternViewer/controller/PatternViewerController.h"
#include "../ui/testArea/controller/TestAreaController.h"
#include "../ui/toolbar/main/controller/MainToolbarController.h"
#include "AudioController.h"
#include "MainController.h"
#include "MainControllerObservers.h"
#include "PlayerController.h"
#include "SongController.h"
#include "../ui/mainView/operation/mergeSong/MergeSong.h"
#include "serial/SerialController.h"

namespace arkostracker 
{

class Exiter;
class StorageListener;

/** Implementation of the MainController. */
class MainControllerImpl final : public MainController,
                                 public LoadSong::Callback,
                                 public NewSong::Listener,
                                 public LoadInstrument::Listener
{
public:
    /**
     * Constructor.
     * @param song the Song.
     * @param exiter knows how to exit the app.
     * @param storageListener to change the layouts.
     */
    explicit MainControllerImpl(const std::shared_ptr<Song>& song, Exiter& exiter, StorageListener& storageListener) noexcept;

    // MainController method implementations.
    // =============================================
    SongController& getSongController() const noexcept override;
    PlayerController& getPlayerController() const noexcept override;
    SongPlayer& getSongPlayer() const noexcept override;
    AudioController& getAudioController() const noexcept override;
    StorageListener& getStorageListener() const noexcept override;

    void tryToExitApplication() override;
    MainControllerObservers& observers() noexcept override;

    void onWantToChangeMuteState(int channelIndex, bool newMuteState) override;
    void onWantToToggleMuteState(int channelIndex) override;
    void onWantToAllMuteExcept(int channelIndex) override;
    void unmuteAll() override;
    void stopPlaying(bool notifyIfAlreadyStopped) override;
    void togglePlayPatternFromTopOrBlock() override;
    void switchToSubsong(const Id& subsongId) noexcept override;

    std::unordered_set<int> getChannelMuteStates() const noexcept override;

    bool canUndo() const override;
    bool canRedo() const override;
    void undo() override;
    void redo() override;
    juce::String getUndoDescription() override;
    juce::String getRedoDescription() override;
    void restoreLayout(int layoutIndex) noexcept override;

    void setOctave(int desiredOctave) noexcept override;
    int getCurrentOctave() const noexcept override;
    void increaseOctave(int offset) noexcept override;

    bool tryToRecordExternalNote(int note) noexcept override;

    OptionalId getSelectedExpressionId(bool isArpeggio) const noexcept override;
    void setSelectedExpressionId(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelection) noexcept override;
    OptionalInt getExpressionIndex(bool isArpeggio, const Id& expressionId) const noexcept override;

    OptionalId getSelectedInstrumentId() const noexcept override;
    std::vector<int> getSelectedInstrumentIndexes() const noexcept override;
    void setSelectedInstrumentId(const OptionalId& selectedInstrumentId, bool forceSingleSelectionForObservers) noexcept override;
    void onSelectedInstrumentsChanged(const std::vector<int>& selectedIndexes) noexcept override;
    void setSelectedInstrumentFromIndex(int index) noexcept override;
    void selectNextOrPreviousInstrument(bool next) noexcept override;
    OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept override;

    juce::ApplicationCommandManager& getCommandManager() const noexcept override;
    void setCommandManager(juce::ApplicationCommandManager& applicationCommandManager) noexcept override;
    void setPanelSearcher(PanelSearcher* panelSearcher) noexcept override;
    PanelSearcher* getPanelSearcher() const noexcept override;
    bool searchAndOpenPanel(PanelType panelType) noexcept override;

    LinkerController& getLinkerControllerInstance() noexcept override;
    PatternViewerController& getPatternViewerControllerInstance() noexcept override;
    ListController& getExpressionTableListController(bool isArpeggio) noexcept override;
    ListController& getInstrumentTableListController() noexcept override;
    InstrumentEditorController& getInstrumentEditorController() noexcept override;
    EditorWithBarsController& getArpeggioTableEditorController() noexcept override;
    EditorWithBarsController& getPitchTableEditorController() noexcept override;
    MainToolbarController& getMainToolbarControllerInstance() noexcept override;
    TestAreaController& getTestAreaControllerInstance() noexcept override;

    void changeSong(std::unique_ptr<Song> newSong, bool allowedToShowSongInfoDialog, bool clearSongPath) noexcept override;
    void notifyAllChanges(const Id& subsongId) noexcept override;
    void loadSong(const juce::String& pathIfKnown) noexcept override;
    void mergeSong() noexcept override;
    void saveSong(bool saveAs) noexcept override;
    void saveCopy() noexcept override;
    void newSong() noexcept override;
    void clearRecentFiles() noexcept override;
    void loadInstrument() noexcept override;
    juce::String getSongPath() noexcept override;
    void setSongPathAndNotify(juce::String path) noexcept override;
    void markSongAsSaved() noexcept override;
    bool isSongModified() const noexcept override;

    void onUserWantsToStartStopSerial() noexcept override;
    bool canSaveCopy() const noexcept override;

    // LoadSong::Listener method implementations.
    // ===================================================
    void onLoadedSongSuccess(std::unique_ptr<Song> song) override;
    void onLoadedSongFailure() override;

    // NewSong::Listener method implementations.
    // ===================================================
    void onNewSongValidatedOrCanceled(std::unique_ptr<Song> song) noexcept override;

    // LoadInstrument::Listener method implementations.
    // ===================================================
    void onLoadInstrumentFinished(std::unique_ptr<Instrument> loadedInstrument) noexcept override;
    void onLoadInstrumentCancelled() noexcept override;

private:
    /** Notifies all the observers of a change in the Channel Mute state. */
    void notifyChannelMuteObservers() noexcept;

    /** @return the id of the current Subsong. */
    Id getCurrentSubsongId() const noexcept;

    /** After the modification state is checked and the song possibly saved, proceed to the New Song process. */
    void newSongAfterModificationPrompt() const noexcept;

    /** @return true if the given panel type is visible in this layout. */
    bool isPanelVisible(PanelType panelType) const noexcept;

    /** Once the song is saved, performs the "save copy" itself. */
    void performSaveCopy() noexcept;
    /** Shows a pop-up because the save copy failed. */
    void showSaveCopyFailed() noexcept;

    Exiter& exiter;                                                     // Knows how to exit the app.
    StorageListener& storageListener;                                   // To change the layouts.

    MainControllerObservers mainControllerObservers;                    // Holds observers to register to.

    std::unordered_set<int> mutedChannelIndexes;                        // Indexes of the channels that are muted.
    ChannelsMute channelsMute;                                          // Knows how to manage the mute logic.

    OptionalId currentlySelectedArpeggioId;                             // The currently selected Arpeggio, useful for the UI.
    OptionalId currentlySelectedPitchId;                                // The currently selected Pitch, useful for the UI.
    OptionalId currentlySelectedInstrumentId;                           // The ID of the currently selected Instrument, useful for the UI.
    std::vector<int> currentlySelectedInstrumentIndexes;                // The currently selected Instrument indexes, the last only being the selected.

    juce::String songPath;                                              // Empty if not known. Only for native Songs (when an import is done, it is not set).

    juce::ApplicationCommandManager* applicationCommandManager;
    PanelSearcher* panelSearcher;

    std::unique_ptr<SongController> songController;                     // The Song Controller.
    std::unique_ptr<AudioController> audioController;                   // The Audio Controller.
    std::unique_ptr<PlayerController> playerController;                 // The Player Controller.
    std::unique_ptr<LinkerController> linkerController;                 // A lazily-instantiated Controller.
    std::unique_ptr<PatternViewerController> patternViewerController;   // A lazily-instantiated Controller.
    std::unique_ptr<ListController> arpeggioTableListController;        // A lazily-instantiated Controller.
    std::unique_ptr<ListController> pitchTableListController;           // A lazily-instantiated Controller.
    std::unique_ptr<ListController> instrumentTableListController;      // A lazily-instantiated Controller.
    std::unique_ptr<InstrumentEditorController> instrumentEditorController;             // A lazily-instantiated Controller.
    std::unique_ptr<EditorWithBarsController> arpeggioTableEditorController;            // A lazily-instantiated Controller.
    std::unique_ptr<EditorWithBarsController> pitchTableEditorController;               // A lazily-instantiated Controller.
    std::unique_ptr<MainToolbarController> mainToolbarController;       // A lazily-instantiated Controller.
    std::unique_ptr<TestAreaController> testAreaController;             // A lazily-instantiated Controller.

    std::unique_ptr<LoadSong> songLoader;
    std::unique_ptr<SaveSong> songSaver;
    std::unique_ptr<CheckIfSongSavedAndSave> checkIfSongSavedAndSave;
    std::unique_ptr<ModalDialog> songInfoDialog;
    std::unique_ptr<NewSong> newSongManager;
    std::unique_ptr<MergeSong> songMerger;

    SerialController serialController;
    std::unique_ptr<LoadInstrument> instrumentLoader;

    std::unique_ptr<ModalDialog> alertDialog;
};

}   // namespace arkostracker
