#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "../../../controllers/model/Follow.h"
#include "../../../utils/WithParent.h"
#include "../../components/ButtonWithImage.h"
#include "../../components/InformationView.h"
#include "../../components/ViewPortWithHorizontalMouseWheel.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../components/dialogs/SetValueDialog.h"
#include "../../keyboard/VirtualKeyboardCommands.h"
#include "PatternViewerCommands.h"
#include "PatternViewerView.h"
#include "tracks/AbstractTrackView.h"
#include "tracks/lineTrack/LineTrackView.h"
#include "tracks/specialTrack/eventTrack/EventTrackView.h"
#include "tracks/specialTrack/speedTrack/SpeedTrackView.h"
#include "tracks/track/TrackColors.h"
#include "tracks/track/TrackView.h"

namespace arkostracker 
{

class PatternViewerController;

/**
 * Implementation of the PatternViewerView. It is important to note that this view does NOT hold any data, it merely
 * transmits them to the view that are needing them.
 */
class PatternViewerViewImpl final : public PatternViewerView,
                                    public TrackView::Listener,
                                    public SpecialTrackView::Listener,
                                    public LineTrackView::Listener,
                                    public juce::ApplicationCommandTarget,
                                    juce::ScrollBar::Listener,
                                    SliderIncDec::Listener
{
public:

    /**
     * Constructor.
     * @param patternViewerController the controller.
     * @param displayedMetadata the metadata to show.
     * @param initialStep the Step initial value.
     * @param writeInstrument the Write Instrument initial value.
     * @param recordState true if recording.
     * @param follow the Follow state.
     */
    PatternViewerViewImpl(PatternViewerController& patternViewerController, PatternViewerView::DisplayedMetadata displayedMetadata, int initialStep,
                          bool writeInstrument, bool recordState, Follow follow) noexcept;

    // Component method implementations.
    // ======================================
    void resized() override;
    void lookAndFeelChanged() override;
    bool keyPressed(const juce::KeyPress& key) override;
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    // PatternViewerView method implementations.
    // ==============================================
    void setDisplayedMetadata(const DisplayedMetadata& newDisplayedMetadata, const DisplayedData& displayedData) noexcept override;
    void setShownLineAndCursorLocation(int line, OptionalValue<CursorLocation> cursorLocation) noexcept override;
    void setDisplayedData(const DisplayedData& displayedData) noexcept override;
    int getViewportX() const noexcept override;
    void setViewportX(int x) noexcept override;
    void updateMeters(std::unordered_map<int, PeriodAndNoiseMeterInput> channelIndexToPeriodAndNoiseMeterInput) noexcept override;
    void setRecordingState(bool newRecordState) noexcept override;
    void setFollowMode(Follow newFollowMode) noexcept override;
    void setWriteInstrumentState(bool writeInstrumentOnWriteNote) noexcept override;
    void setOverwriteDefaultEffect(bool locked) noexcept override;
    void setDefaultEffect(Effect effect) noexcept override;
    void setSteps(int steps) noexcept override;
    OptionalBool determineOutOfBoundsDirectionFromMouseEvent(const juce::MouseEvent& mouseEvent) noexcept override;
    void setInformationText(const juce::String& text, bool isError) noexcept override;
    void setEffectContextText(int channelIndex, const juce::String& text) noexcept override;
    void onUserWantsToSetMusicTrackNameAndColor(int channelIndex, const juce::String& currentName, OptionalValue<juce::Colour> currentColor) noexcept override;
    void onUserWantsToSetSpecialTrackName(bool isSpeedTrack, const juce::String& currentName) noexcept override;
    void onUserWantsToSetLink(int channelIndex) noexcept override;
    void onUserWantsToSetSpecialLink(bool isSpeedTrack) noexcept override;
    void showWarningBecauseRecordingOff() noexcept override;

    // TrackView::Listener method implementations.
    // =============================================
    void onUserLeftClickedOnChannelNumber(int channelIndex) override;
    void onUserRightClickedOnChannelNumber(int channelIndex) override;
    void onUserWantsToScrollVerticallyWithMouseWheelOnTrack(bool scrollToBottom) override;
    void onUserClickedOnRankOfNormalTrack(int channelIndex, int rankFromCenterAsOrigin, CellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromNormalTrack(bool leftButton) override;
    void onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event) override;
    void onUserWantsToChangeTransposition(int channelIndex) override;
    void onUserWantsToSetTrackNameAndColor(int channelIndex, const juce::String& currentName, OptionalValue<juce::Colour> currentColor) override;
    void onUserWantsToManageLink(int channelIndex) override;

    // SpecialTrackView::Listener method implementations.
    // =====================================================
    void onUserClickedOnRankOfSpecialTrack(bool isSpeedTrack, int rankFromCenterAsOrigin, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromSpecialTrack(bool leftButton) override;
    void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) override;
    void onUserWantsToNameSpecialTrack(bool isSpeedTrack, const juce::String& currentName) override;
    void onUserWantsToManageSpecialLink(bool isSpeedTrack) override;

    // LineTrackView::Listener method implementations.
    // ===================================================
    void onUserWantsToScrollVerticallyWithMouseWheelOnLineTrack(bool scrollToBottom) override;
    void onUserClickedOnTrackHeight() override;
    void onUserWantsToModifyTrackHeightWithOffset(int offset) override;
    void onUserClickedOnLineTrack(int rankFromCenterAsOrigin, bool leftButton) override;
    void onUserClickIsUpFromLineTrack(bool leftButton) override;
    void onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event) override;

    // ApplicationCommandTarget method implementations.
    // ==================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    static const int renameDialogWidth;
    static const int spaceBetweenTracks;                    // Separation width between each track.
    static const int offsetEffectsComboBoxId;

    class SetHeightDialogListener final : public SetValueDialog::Listener,
                                          public WithParent<PatternViewerViewImpl>
    {
    public:
        explicit SetHeightDialogListener(PatternViewerViewImpl& parent);
        void onWantToCancelSetValue() override;
        void onWantToSetValue(int newHeight) override;
    };

    class SetTranspositionDialogListener final : public SetValueDialog::Listener,
                                                 public WithParent<PatternViewerViewImpl>
    {
    public:
        explicit SetTranspositionDialogListener(PatternViewerViewImpl& parent);
        void onWantToCancelSetValue() override;
        void onWantToSetValue(int newTransposition) override;
        void setChannelIndex(int channelIndex);

    private:
        int channelIndex;           // A storage for the dialog.
    };

    // juce::ScrollBar::Listener method implementations.
    // ====================================================
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

    // ====================================================

    /**
     * Sets the new height to the font.
     * @param height the new height.
     */
    void setFontNewHeight(int height) const noexcept;

    /**
     * Creates the Track Views. This might create and delete Tracks, if there were too much.
     * Nothing happens if the track count is already correct.
     * Note that the created Tracks are only "template", they should have their data updated.
     * @param trackCount how many Tracks are expected.
     */
    void createOrDeleteTrackViewsIfNeeded(int trackCount) noexcept;

    /**
     * Changes the width of a Track according to the given minimize state. Only this View is resized.
     * @param trackView the TrackView to resize.
     * @param minimized true if minimized.
     * @return the new size. Handy to know where to locate the next ones.
     */
    static int changeTrackWidthFromMinimizeState(AbstractTrackView& trackView, bool minimized) noexcept;

    /**
     * Sets the bounds and size of the Tracks, even the one outside the viewport.
     * This resizes the viewport too, and its content.
     */
    void relocateAndResizeAllTrackViews();

    /**
     * Sets the bounds and size of all Track Views, according to their minimized state.
     * This will also resize the Holder in the Viewport.
     */
    void relocateAndResizeAllTrackViewsInViewport() noexcept;

    /** Applies the stored Metadata into all the Tracks. */
    void applyMetadataToTracks() const noexcept;

    /**
     * @return true if the channel which index is given is minimized. As a convenient side-effect, a Special track will always return false.
     * @param channelIndex the channel index. May be out-of-bounds.
     */
    bool isTrackMinimized(int channelIndex) const noexcept;

    /** Builds the TrackViewMetadata, from the look and feel. */
    std::shared_ptr<TrackViewMetadata> buildInternalTrackViewMetadata() const noexcept;

    /** @return all the Track Views, including the Special Tracks.*/
    std::vector<AbstractTrackView*> getAbstractTrackViews() const noexcept;

    /** Called when the Minimize button is clicked. */
    void onMinimizeButtonClicked() const noexcept;
    /** Called when the Record button is clicked. */
    void onRecordButtonClicked() const noexcept;
    /** Called when the Follow button is clicked. */
    void onFollowButtonClicked() const noexcept;
    /** Called when the Write Instrument button is clicked. */
    void onWriteInstrumentButtonClicked() const noexcept;
    /** Called when the Lock Effect button is clicked. */
    void onLockEffectButtonClicked() const noexcept;
    /** Called when the Toolbox button is clicked. */
    void onToolboxButtonClicked() noexcept;

    /** Fills the Effect ComboBox with the available effects, from the metadata. Nothing happens if the data is the same. */
    void fillEffectComboBox() noexcept;

    /** Called when another locked effect has been selected. */
    void onDefaultEffectChanged() const noexcept;

    /** Adjusts the X of the viewport so that the Track (normal or special) where the cursor is, is well visible. */
    void adjustXScrollingToCursor(const CursorLocation& cursorLocation) noexcept;

    /**
     * Called when the dialog to rename/color a Track is validated.
     * @param channelIndex the channel index.
     * @param newName the new name.
     * @param newColor the new possible color.
     */
    void onSetTrackNameAndColorValidated(int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) noexcept;

    /**
     * Called when the dialog to rename a Special Track is validated.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param newName the new name.
     */
    void onRenameSpecialTrackNameValidated(bool isSpeedTrack, const juce::String& newName) noexcept;

    /** Closes the possible Dialog. */
    void closeDialog() noexcept;

    /**
     * Creates a Track Link. First, closes the dialog.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param sourceChannelIndex the channel index from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     * @param targetChannelIndex the channel index to link to.
     */
    void performLink(int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) noexcept;

    /**
     * Unlinks a linked Track. First, closes the dialog.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     * @param channelIndexToUnlink the channel index to remove the link from.
     */
    void performUnlink(int positionIndexToUnlink, int channelIndexToUnlink) noexcept;

    /**
      * Creates a Special Track Link. First, closes the dialog.
      * @param isSpeedTrack true if Speed Track, false if Event.
      * @param sourcePositionIndex the index of the position from which to link.
      * @param targetPositionIndex  the index of the position to link to.
      */
    void performSpecialLink(bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) noexcept;

    /**
     * Unlinks a linked Special Track. First, closes the dialog.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     */
    void performSpecialUnlink(bool isSpeedTrack, int positionIndexToUnlink) noexcept;

    /**
     * Goes to a Track. First, closes the dialog.
     * @param positionIndexToGoTo the index of the position to go to.
     * @param channelIndexToGoTo the channel index to go to.
     */
    void performGoto(int positionIndexToGoTo, int channelIndexToGoTo) noexcept;

    /**
     * Goes to a Special Track. First, closes the dialog.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param positionIndexToGoTo the index of the position to go to.
     */
    void performSpecialGoto(bool isSpeedTrack, int positionIndexToGoTo) noexcept;

    /**
     * Shows a dialog to rename a Special Track.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param currentName the current name to show.
     */
    void promptToSetSpecialTrackName(bool isSpeedTrack, const juce::String& currentName) noexcept;

    /**
     * The user wants to manage the link/unlink for a special track.
     * @param isSpeedTrack true if Speed Track, false if Event.
     */
    void promptToSetSpecialLink(bool isSpeedTrack) noexcept;

    /**
     * A custom viewport to handle the horizontal mouse wheel via Alt, but if
     * Alt is not pressed, performs a vertical scrolling.
     */
    class CustomViewPort final : public ViewPortWithHorizontalMouseWheel,
                           WithParent<PatternViewerViewImpl>
    {
    public:
        explicit CustomViewPort(PatternViewerViewImpl& pParent) :
                WithParent(pParent)
        {
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            if (event.mods.isAltDown()) {
                ViewPortWithHorizontalMouseWheel::mouseWheelMove(event, wheel);
            } else {
                if (!juce::exactlyEqual(wheel.deltaY, 0.0F)) {
                    parentObject.onUserWantsToScrollVerticallyWithMouseWheelOnTrack(wheel.deltaY < 0.0F);
                }
            }
        }
    };

    PatternViewerCommands patternViewerCommands;
    VirtualKeyboardCommands virtualKeyboardCommands;

    PatternViewerController& patternViewerController;
    DisplayedMetadata displayedMetadata;                            // Stored because we need it on look and feel change.

    std::unique_ptr<juce::Font> font;
    std::unique_ptr<juce::Font> doubleFont;

    ButtonWithImage recordButton;
    ButtonWithImage followButton;
    SliderIncDec stepSlider;
    ButtonWithImage writeInstrumentButton;
    ButtonWithImage minimizeButton;
    juce::ComboBox effectsComboBox;                                 // Shows the effects, possibly to write.
    ButtonWithImage lockEffectButton;                               // The "Lock write effect" Toggle Button.
    ButtonWithImage toolboxButton;
    InformationView informationView;

    CustomViewPort viewport;                                        // Where all the Tracks are.
    Component holderInViewport;                                     // Unique component in the Viewport. Required by JUCE.
    std::vector<std::unique_ptr<TrackView>> trackViews;             // The Track Views (excluding special tracks).
    std::unique_ptr<LineTrackView> lineTrackView;                   // The lines on the left. Out of the Viewport.
    std::unique_ptr<SpeedTrackView> speedTrackView;                 // The Speed Track, just after the Tracks (inside the Viewport).
    std::unique_ptr<EventTrackView> eventTrackView;                 // The Event Track, just after the Speed Track (inside the Viewport).
    juce::ScrollBar verticalScrollBar;                              // The vertical ScrollBar at the right. Not part of the Viewport!

    int storedTrackHeight;                                          // An exception, the track height is stored to be shown in Set Height Dialog.
    std::unordered_map<int, int> storedChannelToTransposition;      // Another exception.

    bool isToolboxOpened;                                           // Managed here. The Controller doesn't have to bother with this.

    std::unique_ptr<ModalDialog> modalDialog;
    SetHeightDialogListener setHeightDialogListener;
    SetTranspositionDialogListener setTranspositionDialogListener;

    std::unordered_map<Effect, juce::juce_wchar> cachedEffectToChar;    // Only useful to know if a change has been performed.
};

}   // namespace arkostracker
