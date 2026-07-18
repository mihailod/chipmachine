#pragma once

#include <unordered_set>

#include "../ui/lists/controller/ListController.h"
#include "MainControllerObservers.h"
#include "SongController.h"

namespace arkostracker 
{

class ChannelMuteObserver;
class SelectedExpressionIndexObserver;
class LinkerController;
class PlayerController;
class ParentViewLifeCycleAware;
class PatternViewerController;
class SongPlayer;
class AudioController;
class InstrumentEditorController;
class PanelSearcher;
class EditorWithBarsController;
class MainToolbarController;
class TestAreaController;
class StorageListener;

/**
 * The MainController holds several critical controllers, such as the Song Controller.
 * It also holds some data, which are not "inside" the Songs, such as the current position, and
 * the Channel mute states, the current instrument and expressions.
 */
class MainController
{
public:
    /** Destructor. */
    virtual ~MainController() = default;

    /**
     * Shows a new Song. This refreshes all the UI, and shows a Song Info dialog if the user wants it.
     * This stops the player.
     * @param newSong the new song.
     * @param allowedToShowSongInfoDialog if true, the song info dialog may be shown (according to the preferences). For New song, it shouldn't be necessary.
     * @param clearSongPath true to clear the internal song path, so Save will ask for a new path. New songs should set it to true. Loaded ones to false.
     */
    virtual void changeSong(std::unique_ptr<Song> newSong, bool allowedToShowSongInfoDialog, bool clearSongPath) noexcept = 0;

    /**
     * Notifies all the observers about all the changes. Useful when changing a Song.
     * @param subsongId the SubsongId for the notification to target.
     */
    virtual void notifyAllChanges(const Id& subsongId) noexcept = 0;

    /**
     * Loads a Song. This takes care of showing the pop-ups, asking for the configuration, and ultimately changing the Song.
     * @param pathIfKnown empty to show a file picker.
     */
    virtual void loadSong(const juce::String& pathIfKnown) noexcept = 0;

    /** Merges a Song. The current one will be replaced. */
    virtual void mergeSong() noexcept = 0;

    /**
     * Saves the song. This takes care of showing the file picker if needed.
     * @param saveAs true to "save as", forcing the file picker.
     */
    virtual void saveSong(bool saveAs) noexcept = 0;

    /** Saves a copy of the song, if possible (path is set). Asserts if not possible, as the UI should have prevented this. */
    virtual void saveCopy() noexcept = 0;

    /** Starts a new song. Prompts the user if the user has been modified. Shows a page to choose data about the new song. */
    virtual void newSong() noexcept = 0;

    /** Prompt to load an instrument. */
    virtual void loadInstrument() noexcept = 0;

    /** @return the path of the current Song. If not known yet (new song or imported one), it is empty. */
    virtual juce::String getSongPath() noexcept = 0;
    /**
     * Sets the path (or absence of) of the current Song. The path must be full only for a native Song, not an import,
     * in which case the path must be empty.
     * This notifies the Song Metadata Observer about the change (if any).
     * @param path the path.
     */
    virtual void setSongPathAndNotify(juce::String path) noexcept = 0;

    /** Marks the song as saved. This notifies observers. */
    virtual void markSongAsSaved() noexcept = 0;
    /** @return true if the song is modified. */
    virtual bool isSongModified() const noexcept = 0;

    /** @return the Song Controller. */
    virtual SongController& getSongController() const noexcept = 0;
    /** @return the Player Controller. */
    virtual PlayerController& getPlayerController() const noexcept = 0;
    /** @return the Song Player. */
    virtual SongPlayer& getSongPlayer() const noexcept = 0;
    /** @return the Audio Controller. */
    virtual AudioController& getAudioController() const noexcept = 0;
    /** @return the Storage Listener. */
    virtual StorageListener& getStorageListener() const noexcept = 0;

    /** @return the Command Manager. Warning, it is not advised to store this directly from the MainController. Use a Get every time whenever useful. */
    virtual juce::ApplicationCommandManager& getCommandManager() const noexcept = 0;
    /** Sets the ApplicationCommandManager. */
    virtual void setCommandManager(juce::ApplicationCommandManager& applicationCommandManager) noexcept = 0;
    /** Sets the Panel Searcher (to open a specific panel). May be nullptr. */
    virtual void setPanelSearcher(PanelSearcher* panelSearcher) noexcept = 0;
    /** @return the possible Panel Searcher. */
    virtual PanelSearcher* getPanelSearcher() const noexcept = 0;
    /**
     * Searches a panel in the current layout, and opens it if needed. It is focused in all cases.
     * @param panelType the panel type.
     * @return true if found or already opened. False if not found.
     */
    virtual bool searchAndOpenPanel(PanelType panelType) noexcept = 0;

    /** Tries to exit the application. This might open pop-ups for confirmation. */
    virtual void tryToExitApplication() = 0;

    /** @return an object which allows to observe various elements. */
    virtual MainControllerObservers& observers() noexcept = 0;

    /**
     * The user wants to change the mute state. Notifies.
     * @param channelIndex the channel which state to change.
     * @param newMuteState true if the channel must be muted.
     */
    virtual void onWantToChangeMuteState(int channelIndex, bool newMuteState) = 0;

    /**
     * The user wants to toggle the mute state. Notifies.
     * @param channelIndex the channel which state to change.
     */
    virtual void onWantToToggleMuteState(int channelIndex) = 0;

    /**
     * The user wants to must all except one channel. But if others are all muted, unmute all. Notifies.
     * @param channelIndex the target channel index.
     */
    virtual void onWantToAllMuteExcept(int channelIndex) = 0;

    /** Unmutes all the channels. Notifies. */
    virtual void unmuteAll() = 0;

    /**
     * Stops playing the Song.
     * @param notifyIfAlreadyStopped if true, if the player was already stopped, broadcast listener to this.
     * This is useful to perform an action, such as dismissing a selection, etc.
     */
    virtual void stopPlaying(bool notifyIfAlreadyStopped) = 0;

    /** Toggles Play Pattern from the top of the pattern, or from the block if present. */
    virtual void togglePlayPatternFromTopOrBlock() = 0;

    /**
     * Selects a Subsong. This switches to it, which will notify all the UI.
     * Nothing happens if the subsong is the same.
     * @param subsongId the subsong id. It must be valid.
     */
    virtual void switchToSubsong(const Id& subsongId) noexcept = 0;

    /** Called when the user wants to start/stop the serial communication. */
    virtual void onUserWantsToStartStopSerial() noexcept = 0;

    // ======================================================================

    /** @return true if an Undo is possible. */
    virtual bool canUndo() const = 0;
    /** @return true if a Redo is possible. */
    virtual bool canRedo() const = 0;
    /** Performs an Undo, if possible. */
    virtual void undo() = 0;
    /** Performs a Redo, if possible. */
    virtual void redo() = 0;
    /** @returns the displayable name of the Undo action. */
    virtual juce::String getUndoDescription() = 0;
    /** @returns the displayable name of the Redo action. */
    virtual juce::String getRedoDescription() = 0;

    /**
     * Restores a layout.
     * @param layoutIndex the layout index (>=0).
     */
    virtual void restoreLayout(int layoutIndex) noexcept = 0;

    /**
     * Sets the current base octave. This notifies the listeners.
     * @param desiredOctave the octave. Will be corrected.
     */
    virtual void setOctave(int desiredOctave) noexcept = 0;
    /** @return the current base octave. */
    virtual int getCurrentOctave() const noexcept = 0;
    /**
     * Increases the octave of a certain value (will be corrected). This notifies the listeners.
     * @param offset the offset. May be negative.
     */
    virtual void increaseOctave(int offset) noexcept = 0;

    /**
     * Call this when an external note (such as MIDI note) is received and the PatternViewer may be interested
     * in recording it/playing it. If not, false it returned.
     * For example, the PV must be visible and recording for the note to be interesting to it.
     * @param note the note.
     * @return true if the PV manages it. False if nothing is done.
     */
    virtual bool tryToRecordExternalNote(int note) noexcept = 0;

    // ======================================================================

    /** @return the index of the channels that are muted. May be empty. */
    virtual std::unordered_set<int> getChannelMuteStates() const noexcept = 0;


    // Expressions.
    // ======================================================================

    /**
     * @returns the id of the selected Expression. May be absent if none is.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual OptionalId getSelectedExpressionId(bool isArpeggio) const noexcept = 0;

    /**
     * Sets the selected Expression. Notifies the listeners.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param selectedExpressionId the selected id. May be empty (corner case though).
     * @param forceSingleSelectionForObservers true to indicate the observers the selection must be unique. Useful after expressions are deleted, for example.
     */
    virtual void setSelectedExpressionId(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelectionForObservers) noexcept = 0;

    /**
     * @return the possible index of the Expression which ID is given.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID. If not found, empty is returned.
     */
    virtual OptionalInt getExpressionIndex(bool isArpeggio, const Id& expressionId) const noexcept = 0;


    // Instruments.
    // ======================================================================

    /** @return the id of the selected instrument, if any. */
    virtual OptionalId getSelectedInstrumentId() const noexcept = 0;

    /** @return the indexes of the selected instruments, the last one being the currently selected. */
    virtual std::vector<int> getSelectedInstrumentIndexes() const noexcept = 0;

    /**
     * Sets the selected Expression. Notifies the listeners.
     * @param selectedInstrumentId the ID of selected Instrument. May be empty (corner case though).
     * @param forceSingleSelectionForObservers true to indicate the observers the selection must be unique. Useful after instruments are deleted, for example.
     */
    virtual void setSelectedInstrumentId(const OptionalId& selectedInstrumentId, bool forceSingleSelectionForObservers) noexcept = 0;

    /**
     * Called when the selected instruments changed (in the Instrument list for example). Notifies the listeners.
     * @param selectedIndexes the selected indexes, the last one being the selected.
     */
    virtual void onSelectedInstrumentsChanged(const std::vector<int>& selectedIndexes) noexcept = 0;

    /**
     * Sets the selected instrument from an index. This should only be used when only an index if relevant (such as a MIDI IN channel).
     * @param index the index. Will be corrected.
     */
    virtual void setSelectedInstrumentFromIndex(int index) noexcept = 0;

    /**
    * Selects the next or previous instrument, if possible. This notifies the listeners.
    * @param next true if next, false if previous.
    */
    virtual void selectNextOrPreviousInstrument(bool next) noexcept = 0;

    /**
     * @return the possible index of the Instrument which ID is given.
     * @param instrumentId the instrument ID. May be invalid.
     */
    virtual OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept = 0;

    // Controller.
    // ======================================================================

    /** @return a lazily-instantiated Linker Controller. It is automatically creating the LinkerView. */
    virtual LinkerController& getLinkerControllerInstance() noexcept = 0;

    /** @return a lazily-instantiated Pattern Controller. */
    virtual PatternViewerController& getPatternViewerControllerInstance() noexcept = 0;

    /**
     * @return a lazily-instantiated Expression Table List Controller.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual ListController& getExpressionTableListController(bool isArpeggio) noexcept = 0;

    /** @return a lazily-instantiated Instrument Table List Controller. */
    virtual ListController& getInstrumentTableListController() noexcept = 0;

    /** @return a lazily-instantiated Instrument Editor Controller. */
    virtual InstrumentEditorController& getInstrumentEditorController() noexcept = 0;

    /** @return a lazily-instantiated Arpeggio Table Editor Controller. */
    virtual EditorWithBarsController& getArpeggioTableEditorController() noexcept = 0;
    /** @return a lazily-instantiated Pitch Table Editor Controller. */
    virtual EditorWithBarsController& getPitchTableEditorController() noexcept = 0;

    /** @return a lazily-instantiated MainToolbar Controller. */
    virtual MainToolbarController& getMainToolbarControllerInstance() noexcept = 0;

    /** @return a lazily-instantiated TestArea Controller. */
    virtual TestAreaController& getTestAreaControllerInstance() noexcept = 0;


    // Preferences.
    // ======================================================================

    /** Clears the list of recent files. */
    virtual void clearRecentFiles() noexcept = 0;


    // ======================================================================

    /** @return true if the current song can have a copy saved (the path must be set). */
    virtual bool canSaveCopy() const noexcept = 0;
};

}   // namespace arkostracker
