#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "PatternViewerController.h"

#include "../../../controllers/observers/ChannelMuteObserver.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../../controllers/observers/GeneralDataObserver.h"
#include "../../../controllers/observers/InstrumentChangeObserver.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../controllers/observers/PatternViewerMetadataObserver.h"
#include "../../../controllers/observers/SongMetadataObserver.h"
#include "../../../controllers/observers/SongPlayerObserver.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../../controllers/observers/TrackChangeObserver.h"
#include "../../../player/PsgRegisters.h"
#include "../../../song/Location.h"
#include "../../../song/subsong/Position.h"
#include "../../../song/tracks/SpecialTrack.h"
#include "../../../song/tracks/Track.h"
#include "../CursorLocation.h"
#include "../view/PatternViewerView.h"
#include "../view/PatternViewerViewImpl.h"
#include "ActionManager.h"
#include "CursorManager.h"
#include "Selection.h"
#include "toolbox/ToolboxController.h"

namespace arkostracker 
{

class CellView;
class EffectContext;
class MainController;
class SongController;
class PlayerController;
class PreferencesManager;

/**
 * Implementation of the Controller of the Pattern Viewer.
 * It holds the data (tracks) to show, and will transmit it to the PV View.
 */
class PatternViewerControllerImpl final : public PatternViewerController,
                                          public ChannelMuteObserver,                 // To be aware of new mute states.
                                          public SongMetadataObserver,
                                          public SubsongMetadataObserver,             // For the highlight spacing.
                                          public PatternViewerMetadataObserver,       // Setup change.
                                          public LinkerObserver,
                                          public SongPlayerObserver,                  // Played location/range change.
                                          public ExpressionChangeObserver,            // To update the view if the expression ordering changes.
                                          public InstrumentChangeObserver,            // To update the view if the instrument ordering changes.
                                          public TrackChangeObserver,                 // To update when Tracks are modified.
                                          public GeneralDataObserver,                 // To manage the double-esc.
                                          public juce::Timer,                         // To update the Meters.
                                          ActionManager::Controller,
                                          public ToolboxController::Controller
{
public:
    friend class CursorManager;

    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit PatternViewerControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~PatternViewerControllerImpl() override;

    // PatternViewerController method implementations.
    // ================================================
    SongController& getSongController() override;
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void onUserLeftClickedOnChannelNumber(int channelIndex) override;
    void onUserRightClickedOnChannelNumber(int channelIndex) override;
    void onUserWantsToScrollVerticallyWithMouseWheel(bool scrollToBottom) override;
    void onUserWantsToMoveVertically(bool goDown, bool fast, bool enlargeSelection) override;
    void onUserWantsToMoveVerticallyTo(int cellIndex, bool enlargeSelection) override;
    void onUserWantsToMoveHorizontally(bool goToRight, bool enlargeSelection) override;
    void onUserWantsToMoveToNextChannel(bool next) override;
    void onUserWantsToMoveToNextPosition(bool next) override;

    void onUserWantsToToggleMinimize() override;
    void onUserWantsToToggleRecord() override;
    void onUserWantsToToggleFollow() override;
    void onUserWantsToToggleWriteInstrument() override;
    void onUserWantsToToggleOverwriteDefaultEffect() override;
    void onUserWantsToChangeDefaultEffect(bool next) override;
    void onUserWantsToDelete(bool wholeCell) override;

    void onUserWantsToModifyTrackHeight(int newHeight) override;
    void onUserWantsToModifyTrackHeightWithOffset(int offset) override;
    void onUserWantsToModifyTransposition(int channelIndex, int newTransposition) override;
    void onUserWantsToSetTrackNameAndColor(int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) override;
    void onUserWantsToRenameSpecialTrack(bool isSpeedTrack, const juce::String& newName) override;
    void onUserWantsToCreateLink(int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) override;
    void onUserWantsToUnlink(int positionIndexToUnlink, int channelIndexToUnlink) noexcept override;
    void onUserWantsToCreateSpecialLink(bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) override;
    void onUserWantsToUnlinkSpecial(bool isSpeedTrack, int positionIndexToUnlink) noexcept override;
    void onUserWantsToGoTo(int positionIndexToGoTo, int channelIndexToGoTo) noexcept override;
    void onUserWantsToGoToChannel(int channelIndexToGoTo) noexcept override;
    void onUserWantsToGoToSpecialTrack(bool isSpeedTrack, int positionIndexToGoTo) noexcept override;
    void onUserClickedOnRankOfNormalTrack(int channelIndex, int rankFromCenterAsOrigin, CellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromNormalTrack(bool leftButton) override;
    void onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event) override;
    void onUserClickedOnRankOfSpecialTrack(bool isSpeedTrack, int rankFromCenterAsOrigin, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromSpecialTrack(bool leftButton) override;
    void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) override;
    void onUserClickedOnRankOfLineTrack(int rankFromCenterAsOrigin, bool leftButton) override;
    void onUserClickIsUpFromLineTrack(bool leftButton) override;
    void onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event) override;

    void onKeyboardNote(int baseNote) override;
    void onNote(int note) override;
    bool onKeyPressed(const juce::KeyPress& key) override;
    void onUserWantsToPlayLine() override;
    void onUserWantsToTogglePlayPatternFromCursorOrBlock(bool forceSetBlock) override;
    void onUserWantsToTogglePlayPatternFromStart() override;

    void onUserWantsToCopy() override;
    void onUserWantsToCut() override;
    void onUserWantsToPaste(bool moveBelowAfter, bool pasteContinuously) override;
    void onUserWantsToPasteMix() override;
    void onUserWantsToClearSelection() override;

    void onUserWantsToTransposeSelection(TransposeRate transposeRate) override;
    void onUserWantsToToggleReadOnly() override;
    void onUserWantsToGenerateArpeggio() override;
    void onUserWantsToGenerateInstrument() override;
    void onUserWantsToSetTrackNameAndColor() override;
    void onUserWantsToLinkTrack() override;

    void onUserWantsToInsert() override;
    void onUserWantsToRemove() override;

    void onUserWantsToSetEditStep(int stepToValidate) override;
    void onUserWantsToIncreaseEditStep(int offset) override;

    void onUserSelectedLockedEffect(Effect effect) override;

    void onUserWantsToToggleSoloTrack() override;
    void onUserWantsToToggleMuteTrack() override;

    void onUserWantsToSelectTrack() override;
    void onUserWantsToSelectAllTracks() override;

    void onUserWantsToCapture() override;
    void onUserWantsToWriteRst() override;

    void getKeyboardFocus() noexcept override;
    bool isReadOnlyTrack(int channelIndex) noexcept override;
    bool canGenerateArpeggioFromSelection() noexcept override;
    bool canGenerateInstrumentFromSelection() noexcept override;
    bool canSetNameAndColorToTrack() noexcept override;
    bool canLinkTrack() noexcept override;
    int getTrackIndex(int channelIndex) noexcept override;
    int getPositionIndex() noexcept override;
    Id getSubsongId() noexcept override;

    bool isRecording() noexcept override;
    Effect getDefaultEffect() const noexcept override;
    const std::vector<Effect>& getDefaultEffects() const noexcept override;

    void showOrHideToolbox(bool open, juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void locateToolbox(int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void showWarningBecauseRecordingOff() noexcept override;

    // ChannelMuteObserver method implementations.
    // ================================================
    void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) override;

    // PatternViewerMetadataObserver method implementations.
    // =======================================================
    void onPatternViewerMetadataChanged() override;
    void onRecordStateChanged(bool newIsRecordingState) override;

    // SongMetadataObserver method implementations.
    // ==============================================
    void onSongMetadataChanged(unsigned int what) override;

    // SubsongMetadataObserver method implementations.
    // ==================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) override;

    // LinkerObserver method implementations.
    // ==============================================
    void onLinkerPositionChanged(const Id& subsongId, int index, unsigned int whatChanged) override;
    void onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& indexes, unsigned int whatChanged) override;
    void onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& highlightedItems) override;
    void onLinkerPatternInvalidated(const Id& subsongId, int patternIndex, unsigned int whatChanged) override;

    // SongPlayerObserver method implementations.
    // ==============================================
    void onPlayerNewLocations(const Locations& locations) noexcept override;
    void onNewPsgRegisters(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept override;

    // ExpressionChangeObserver method implementations.
    // ==================================================
    void onExpressionChanged(const Id& expressionId, unsigned int whatChanged) override;
    void onExpressionCellChanged(const Id& expressionId, int cellIndex, bool mustAlsoRefreshPastIndex) override;
    void onExpressionsInvalidated() override;

    // InstrumentChangeObserver method implementations.
    // ==================================================
    void onInstrumentChanged(const Id& instrumentId, unsigned int whatChanged) override;
    void onInstrumentsInvalidated() override;
    void onPsgInstrumentCellChanged(const Id& instrumentId, int cellIndex, bool mustRefreshAllAfterToo) override;

    // TrackChangeObserver method implementations.
    // ==================================================
    void onTrackDataChanged(const Id& subsongId) override;
    void onTrackMetaDataChanged(const Id& subsongId) override;

    // GeneralDataObserver method implementations.
    // ==================================================
    void onGeneralDataChanged(unsigned int whatChanged) override;

    // Timer method implementations.
    // ==============================================
    void timerCallback() override;

    // ToolboxController::Controller method implementations.
    // ==========================================================
    SelectedData getSelectedData() override;
    SelectedData getPatternSelection() override;
    SelectedData getCurrentSubsongSelection() override;
    std::vector<SelectedData> getAllSubsongsSelection() override;

    // ==========================================================

    /** @return the possible effect context. */
    const EffectContext* getEffectContext() const noexcept;

private:
    static const int metersRefreshMs;                                   // How often to update the Meters, in ms.
    static const int moveVerticallyFastSteps;                           // Speed (steps) when using fast move.
    static const int dragHorizontalScrollingSpeed;                      // Speed when horizontally scrolling because of dragging the selection.
    static const int maximumStep;

    /** Sends the internal displayed metadata to the View, for it to be refreshed, if needed. */
    void sendDisplayedMetadataToView() noexcept;

    /**
     * Gets the data from the song, stores them internally, and asks for a View update.
     * @param forceRefresh true if the View has been recreated, and the Tracks must be sent in all cases. No diff test.
     * @param refreshMetadata true to refresh the metadata (such as instrument colors). Considered true if forceRefresh is true.
     */
    void refreshStoredSongDataAndUpdateView(bool forceRefresh, bool refreshMetadata = false) noexcept;

    /** @return the DisplayedMetadata to send to the View. The internal values are updated from the song. */
    PatternViewerViewImpl::DisplayedMetadata updateInternalDisplayedMetadataAndBuild() noexcept;

    /**
     * Smaller part of the refreshStoredSongDataAndUpdateViewIfWanted method, for the Special Tracks.
     * @param localSpecialTrack the local special track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param forceRefresh true if the View has been recreated, and the Tracks must be sent in all cases. No diff test.
     * @return the SpecialTrack to sent, or nullptr if no change.
     */
    SpecialTrack* getSongDataAndUpdateViewInternalForSpecialTrack(SpecialTrack& localSpecialTrack, bool isSpeedTrack, bool forceRefresh) noexcept;

    /**
     * Sets a new height, which will update the Song and UI.
     * @param desiredHeight the desired height. May be corrected.
     */
    void setNewTrackHeightAndUpdate(int desiredHeight) const noexcept;

    /** @return the Position data of the current position. */
    Position getPositionData() const noexcept;

    /**
     * Called when a change has been done on the linker. It is very vague, but since we rely on diffs, this shouldn't be a problem.
     * @param subsongIdWhereChange the subsong id on which the change happened.
     * @param what the bits of what has changed.
     */
    void onLinkerChange(const Id& subsongIdWhereChange, unsigned int what) noexcept;

    /** @return the played location line, from the current Location and PlayedLocationLine, plus the Follow mode. */
    OptionalInt determinePlayedLocationLine() const noexcept;

    /** @return true if the cursor is following the played location. */
    bool isFollowing() const noexcept;

    /**
     * Stores the internal shown location, but corrects the height.
     * @param location the location.
     */
    void storeShownLocationWithHeightCorrection(const Location& location) noexcept;

    /**
     * @return a Location with a corrected line from the given location (the height if checked).
     * @param location the corrected location.
     */
    Location correctLocationFromShownLocation(const Location& location) const noexcept;

    /** @return true if the player is playing a song/pattern. */
    bool isPlaying() const noexcept;

    /**
     * The user wants to go to a certain line and/or X location. Due to the input (mouse wheel or click), it may be out of bounds.
     * Depending on the player state and the follow, the behavior will differ.
     * @param cursorLocation the possible location to go to.
     * @param desiredLineToGoTo the desired line, perhaps invalid.
     * @param enlargeSelection true if the user wants to enlarge the selection (typically, shift+move). False to set it to the cursor. Absent not to change the selection at all.
     */
    void onUserWantsToGoTo(OptionalValue<CursorLocation> cursorLocation, OptionalInt desiredLineToGoTo, OptionalBool enlargeSelection) noexcept;

    /**
     * Tries to move vertically, losing the selection.
     * @param step the step. May be negative.
     */
    void tryToMoveVertically(int step) noexcept override;

    /**
     * Tries to move vertically.
     * @param step the step. May be negative.
     * @param enlargeSelection true if the user wants to enlarge the selection (typically, shift+move). False to set it to the cursor. Absent not to change the selection at all.
     */
    void tryToMoveVertically(int step, OptionalBool enlargeSelection) noexcept;
    /** Tries to move forward, according to the current step. */
    void tryToMoveForward() noexcept;

    /** Asks the view to show the current line and cursor location, which are internally set. */
    void setCurrentLineAndCursorLocationToView() const noexcept;

    /**
     * Manages the down click on a rank of any kind of Track.
     * @param rankFromCenterAsOrigin the rank from the center as origin. This is not the line index! May be negative.
     * @param leftButton true if the left button is clicked, false if right.
     * @param shift true if shift is pressed.
     * @param getSelection a lambda that will return a Selection of one rank, and the clicked index as a parameter. May not be called.
     */
    void onUserClickedOnRank(int rankFromCenterAsOrigin, bool leftButton, bool shift, const std::function<Selection(int)>& getSelection) noexcept;

    /**
     * Called when the user dragged the cursor, from any kind of tracks.
     * Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank. It is up to the client to find it.
     * This is called from the callback of any track but it does NOT mean the cell under the cursor is a track! It can be anything!
     * @param event the mouse event.
     * @param checkHorizontalScrolling true to check if an horizontal scrolling must be performed. False is useful when dragging from and in the Line Track.
     */
    void onUserDraggedCursor(const juce::MouseEvent& event, bool checkHorizontalScrolling = true) noexcept;

    /**
     * We have determined a drag is performed over a Cell View.
     * @param event the event.
     * @param cellView the CellView.
     */
    void onUserDraggedCursorOverCellView(const juce::MouseEvent& event, CellView& cellView) noexcept;

    /**
     * We have determined a drag is performed over a Simple Cell View. It can be a Line, or a Special Cell.
     * @param event the event.
     * @param simpleCellView the SimpleCellView.
     */
    void onUserDraggedCursorOverSimpleCellView(const juce::MouseEvent& event, const SimpleCellView& simpleCellView) noexcept;

    /**
     * Called when a mouse click is up, from any track (normal or special).
     * @param leftButton true if left button, false if right.
     */
    void onUserClickIsUpFromAnyTrack(bool leftButton) noexcept;

    /**
     * Enlarges the selection to where the cursor is, or dismisses it. Refreshes the UI.
     * @param enlargeOrDismiss true to enlarge the selection. False to dismiss it.
     * @param cursorLineIndexBeforeMoving the line index where the cursor is, before moving. This is only used in case there is no selection, to know where the selection starts.
     * @param cursorLocationBeforeMoving the cursor location, before moving. This is only used in case there is no selection, to know where the selection starts.
     */
    void enlargeSelectionToCursorOrDismissAndRefreshUi(bool enlargeOrDismiss, int cursorLineIndexBeforeMoving, const CursorLocation& cursorLocationBeforeMoving) noexcept;

    /**
     * Determines the clicked index, from 0 to the current height.
     * @param parentTrackView the TrackView of the clicked Cell/Special Cell.
     * @param cellRank the cell rank (this is not the cell index! May be negative).
     * @return the clicked index.
     */
    int determineClickedIndex(const AbstractTrackView& parentTrackView, int cellRank) const noexcept;

    /**
     * Only used when dragging a selection, and a scrolling is performed. Extends the selection to the visible top of bottom line.
     * @param abstractTrackView one Abstract Track View (any will do).
     * @param topOrBottomTarget true to reach the top, false the bottom.
     */
    void extendSelectionToTopOrBottomVisibleLine(const AbstractTrackView& abstractTrackView, bool topOrBottomTarget) noexcept;

    /** Updates the Information View text and Effect Context, according to the cursor location and the Cell it is pointing. */
    void updateInformationViewAndEffectContext() const noexcept;

    /**
     * @return a correct cell index, from 0 to the height of the currently stored position.
     * @param cellIndex the cell index.
     */
    int correctIndexFromCurrentHeight(int cellIndex) const noexcept;

    /**
     * @return a corrected cell index from the given CellRank, from 0 to the height of the currently stored position.
     * @param rankFromCenterAsOrigin the cell rank (as returned by the CellViews). May be negative.
     */
    int determineCorrectedLineIndexFromRank(int rankFromCenterAsOrigin) const noexcept;

    /** Notifies the listener that the block selection has changed. */
    void notifyBlockSelectionChange() const noexcept;

    void setBlockSelection(bool refreshUi, bool notifyChange, const Selection& newSelection) noexcept;

    /**
     * @return a block selection that is within the pattern limits.
     * @param newBlock the blocK to check.
     */
    Selection correctBlock(const Selection& newBlock) const noexcept;

    /**
     * Sets the default effects. This updates the UI.
     * @param effect the effect.
     */
    void setDefaultEffectAndRefreshUi(Effect effect) noexcept;

    /**
     * @return the corrected cursor location. If out-of-bounds, sets the cursor to the first track.
     * @param cursorLocation the input cursor location.
     */
    CursorLocation correctCursorLocation(const CursorLocation& cursorLocation) const noexcept;

    // ActionManager::Controller method implementations.
    // ====================================================
    Selection getCurrentSelection() const noexcept override;
    int getChannelCount() const noexcept override;
    int getChannelCount(const Id& subsongId) const noexcept override;
    int getPositionHeight() const noexcept override;
    CursorLocation getCursorLocation() const noexcept override;
    Location getShownLocation() const noexcept override;
    int getSubsongLength(const Id& subsongId) const noexcept override;

    Cell getCell(int channelIndex, int lineIndex) const noexcept override;
    SpecialCell getSpecialCell(bool isSpeedTrack, int lineIndex) const noexcept override;
    juce::ApplicationCommandManager& getCommandManager() const noexcept override;
    SongController& getSongController() const noexcept override;
    bool isRecording() const noexcept override;
    void setSelectedInstrument(int instrumentIndex) noexcept override;
    void setSelectedEffect(Effect effect) noexcept override;
    void setSelectedExpression(bool isArpeggio, int expressionIndex) noexcept override;
    void notifyActionFailedBecauseRecordingOff() noexcept override;

    // ====================================================

    CursorManager cursorManager;
    ActionManager actionManager;

    MainController& mainController;
    SongController& songController;
    PlayerController& playerController;
    std::unique_ptr<ToolboxController> toolboxController;
    PreferencesManager& preferences;

    Location shownLocation;
    CursorLocation cursorLocation;
    std::unique_ptr<PatternViewerView> patternViewerView;

    bool recording;
    Follow follow;
    bool writeInstrumentOnWriteNote;
    int mouseWheelSteps;
    bool overwriteDefaultEffect;                            // If true, the default effect overwrites the possibly present one.
    Effect defaultEffect;                                   // The effect to use when writing.

    // Metadata
    std::unordered_set<int> mutedChannelIndexes;
    std::unordered_set<int> minimizedChannelIndexes;
    std::unordered_map<int, LinkState> channelToLinkedState;  // If empty, considered as "invalidated".
    LinkState speedTrackLinkState;                            // Updated at the same time as channelToLinkedState above.
    LinkState eventTrackLinkState;
    int highlightStep;
    int secondaryHighlight;
    std::unordered_map<Effect, juce::juce_wchar> effectToChar;
    std::unordered_map<int, juce::uint32> instrumentToColorArgb;
    bool areLineNumberInHexadecimal;
    int fontSize;
    bool zoomMiddleLine;

    // The data.
    int height;
    std::unordered_map<int, Track> channelToTrack;
    SpecialTrack speedTrack;
    SpecialTrack eventTrack;
    std::unordered_map<int, int> channelToTransposition;
    OptionalValue<Location> playedLocation;                     // May be absent if unknown yet, or on a different Position/Subsong! Check before sending.

    OptionalInt storedViewPortX;              // The X on the Viewport, useful when recreating the view. Absent if consumed.

    std::unordered_map<int, PsgRegisters> psgIndexToPendingRegisters;       // The PSG registers to show in the Meters.

    int currentStep;

    Selection currentSelection;
    CellCursorRank clickedDownCellCursorRank;                   // A temp clicked down rank on a Cell Cursor.
    Selection currentBlockSelection;                            // The possible selection on the "lines" to the left.
    bool selectingOnLineTrack;                                  // True when dragging on the Line Track, false for all the other Tracks.
};

}   // namespace arkostracker
