#include "PatternViewerControllerImpl.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../business/song/validation/CheckTrackHeight.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/CollectionUtil.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "../view/PatternViewerViewNoOp.h"
#include "../view/cells/cell/CellView.h"
#include "toolbox/ToolboxControllerImpl.h"

namespace arkostracker 
{

const int PatternViewerControllerImpl::metersRefreshMs = 50;
const int PatternViewerControllerImpl::moveVerticallyFastSteps = 8;
const int PatternViewerControllerImpl::dragHorizontalScrollingSpeed = 30;
const int PatternViewerControllerImpl::maximumStep = 16;

PatternViewerControllerImpl::PatternViewerControllerImpl(MainController& pMainController) noexcept :
        cursorManager(*this),
        actionManager(*this),
        mainController(pMainController),
        songController(pMainController.getSongController()),
        playerController(pMainController.getPlayerController()),
        toolboxController(std::make_unique<ToolboxControllerImpl>(*this, songController)),
        preferences(PreferencesManager::getInstance()),
        shownLocation(songController.getCurrentViewedLocation()),
        cursorLocation(CursorLocation::onTrack(0, CellCursorRank::cursorOnNote)),
        patternViewerView(std::make_unique<PatternViewerViewNoOp>()),        // At first, no real view is used.
        recording(false),
        follow(Follow::followIfRecordOff),
        writeInstrumentOnWriteNote(true),
        mouseWheelSteps(preferences.getPatternViewerMouseWheelSteps()),
        overwriteDefaultEffect(false),
        defaultEffect(Effect::volume),
        mutedChannelIndexes(),
        minimizedChannelIndexes(),
        channelToLinkedState(),
        speedTrackLinkState(LinkState::none),
        eventTrackLinkState(LinkState::none),
        highlightStep(),
        secondaryHighlight(),
        effectToChar(preferences.getEffectToChar()),
        instrumentToColorArgb(songController.buildInstrumentToColor()),
        areLineNumberInHexadecimal(preferences.isPatternViewerLineNumberInHexadecimal()),
        fontSize(preferences.getPatternViewerFontHeight()),
        zoomMiddleLine(preferences.getPatternViewerMustZoomMiddleLine()),

        // Only boilerplate data for now.
        height(64),
        channelToTrack(),
        speedTrack(),
        eventTrack(),
        channelToTransposition(),
        playedLocation(),

        storedViewPortX(),

        psgIndexToPendingRegisters(),

        currentStep(1),
        currentSelection(),
        clickedDownCellCursorRank(CellCursorRank::first),

        currentBlockSelection(),
        selectingOnLineTrack(false)
{
    auto& observers = mainController.observers();
    observers.getChannelMuteStateObservers().addObserver(this);
    observers.getPatternViewerMetadataObservers().addObserver(this);
    observers.getGeneralDataObservers().addObserver(this);
    songController.getLinkerObservers().addObserver(this);
    songController.getSongMetadataObservers().addObserver(this);
    songController.getSubsongMetadataObservers().addObserver(this);
    songController.getExpressionObservers(true).addObserver(this);            // If Expressions moved/deleted, the Tracks may have changed!
    songController.getExpressionObservers(false).addObserver(this);
    songController.getInstrumentObservers().addObserver(this);
    songController.getTrackObservers().addObserver(this);

    mainController.getPlayerController().getSongPlayerObservers().addObserver(this);

    startTimer(metersRefreshMs);
}

PatternViewerControllerImpl::~PatternViewerControllerImpl()
{
    stopTimer();

    auto& observers = mainController.observers();
    observers.getChannelMuteStateObservers().removeObserver(this);
    observers.getPatternViewerMetadataObservers().removeObserver(this);
    observers.getGeneralDataObservers().removeObserver(this);
    songController.getLinkerObservers().removeObserver(this);
    songController.getSongMetadataObservers().removeObserver(this);
    songController.getSubsongMetadataObservers().removeObserver(this);
    songController.getExpressionObservers(true).removeObserver(this);
    songController.getExpressionObservers(false).removeObserver(this);
    songController.getInstrumentObservers().removeObserver(this);
    songController.getTrackObservers().removeObserver(this);

    mainController.getPlayerController().getSongPlayerObservers().removeObserver(this);
}

SongController& PatternViewerControllerImpl::getSongController()
{
    return songController;
}

PatternViewerViewImpl::DisplayedMetadata PatternViewerControllerImpl::updateInternalDisplayedMetadataAndBuild() noexcept
{
    auto channelCount = 0;
    const auto arpeggioCount = songController.getExpressionCount(true);
    const auto pitchCount = songController.getExpressionCount(false);
    songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const Subsong& subsong) {
        const auto& metadata = subsong.getMetadata();
        const auto psgCount = subsong.getPsgCount();

        highlightStep = metadata.getHighlightSpacing();
        secondaryHighlight = metadata.getSecondaryHighlight();
        channelCount = PsgValues::getChannelCount(psgCount);

        const auto& positionData = subsong.getPositionRef(shownLocation.getPosition());
        channelToTransposition = positionData.getChannelToTransposition();
        height = positionData.getHeight();
    });
    instrumentToColorArgb = songController.buildInstrumentToColor();

    return { mutedChannelIndexes, minimizedChannelIndexes,
             effectToChar, instrumentToColorArgb,
             areLineNumberInHexadecimal, fontSize, channelCount, highlightStep, secondaryHighlight, zoomMiddleLine,
             arpeggioCount, pitchCount};
}


// PatternViewerController method implementations.
// ================================================

void PatternViewerControllerImpl::onParentViewCreated(BoundedComponent& parentView) {
    // We can now create a "real" View.

    auto displayedMetadata = updateInternalDisplayedMetadataAndBuild();
    patternViewerView = std::make_unique<PatternViewerViewImpl>(*this, displayedMetadata, currentStep,
                                                                writeInstrumentOnWriteNote, recording, follow);

    refreshStoredSongDataAndUpdateView(true, true);

    parentView.addAndMakeVisible(*patternViewerView);
}

void PatternViewerControllerImpl::onParentViewDeleted() noexcept {
    // Gets the X on the Viewport, to apply it when/if the view is created again.
    storedViewPortX = patternViewerView->getViewportX();

    // Deletes the View, using a no-op View.
    patternViewerView = std::make_unique<PatternViewerViewNoOp>();
}

void PatternViewerControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    patternViewerView->setBounds(startX, startY, newWidth, newHeight);

    // If the view has been recreated, we try to make a "goto" on the Viewport.
    // We do it here on the resize because the views must be created for the viewport scrolling to work.
    // Once the goto is done, we clear the value.
    if (storedViewPortX.isPresent()) {
        patternViewerView->setViewportX(storedViewPortX.getValue());            // May have been stored if the view is destroyed.
        storedViewPortX = OptionalInt();    // Consumed.
    }
}

void PatternViewerControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/) { /* Nothing to do. */ }

void PatternViewerControllerImpl::onUserLeftClickedOnChannelNumber(const int channelIndex)
{
    mainController.onWantToToggleMuteState(channelIndex);
}

void PatternViewerControllerImpl::onUserRightClickedOnChannelNumber(const int channelIndex)
{
    mainController.onWantToAllMuteExcept(channelIndex);
}

void PatternViewerControllerImpl::onUserWantsToScrollVerticallyWithMouseWheel(const bool scrollToBottom)
{
    const auto wheelSpeed = mouseWheelSteps;
    const auto step = scrollToBottom ? wheelSpeed : -wheelSpeed;

    tryToMoveVertically(step, { });     // Just like AT2, scrolling with the mouse wheel does not clear the selection.
}

void PatternViewerControllerImpl::onUserWantsToMoveVertically(const bool goDown, const bool fast, const bool enlargeSelection)
{
    auto step = fast ? moveVerticallyFastSteps : 1;

    if (!goDown) {
        step = -step;
    }

    tryToMoveVertically(step, enlargeSelection);
}

void PatternViewerControllerImpl::onUserWantsToMoveVerticallyTo(const int cellIndex, const bool enlargeSelection)
{
    onUserWantsToGoTo({ }, cellIndex, enlargeSelection);
}

void PatternViewerControllerImpl::onUserWantsToMoveHorizontally(const bool goToRight, const bool enlargeSelection)
{
    const auto locations = cursorManager.buildCursorLocations();
    auto index = CollectionUtil::findIndexOfItem<CursorLocation>(locations, cursorLocation);
    if (index < 0) {
        jassertfalse;       // Strange! The cursor is nowhere?
        return;
    }

    // Moves and makes sure the limits are well managed.
    index = NumberUtil::correctWithLimitedLoop(index + (goToRight ? 1 : -1), 0, static_cast<int>(locations.size() - 1U));

    const auto& newLocation = locations.at(static_cast<size_t>(index));
    onUserWantsToGoTo(newLocation, OptionalInt(), enlargeSelection);
}

void PatternViewerControllerImpl::onUserWantsToMoveToNextChannel(const bool next)
{
    const auto locations = cursorManager.buildCursorLocations();
    auto index = CollectionUtil::findIndexOfItem<CursorLocation>(locations, cursorLocation);
    if (index < 0) {
        jassertfalse;       // Strange! The cursor is nowhere?
        return;
    }
    const auto size = static_cast<int>(locations.size());

    // Browses through the locations to find one matching the beginning of a Track.
    auto found = false;
    while (!found) {
        index += next ? 1 : -1;
        index = NumberUtil::correctWithLimitedLoop(index, 0, size - 1);

        const auto location = locations.at(static_cast<size_t>(index));
        switch (location.getTrackType()) {
            default: [[fallthrough]];
            case TrackType::line:
                jassertfalse;       // Should never happen. Returns to prevent an infinite loop.
                return;
            case TrackType::normal:
                if (location.getRankAsCellCursorRank() == CellCursorRank::first) {
                    found = true;
                }
                break;
            case TrackType::event: [[fallthrough]];
            case TrackType::speed:
                if (location.getRankAsSpecialCursorRank() == SpecialCellCursorRank::first) {
                    found = true;
                }
                break;
        }
    }

    const auto& newLocation = locations.at(static_cast<size_t>(index));
    onUserWantsToGoTo(newLocation, OptionalInt(), false);
}

void PatternViewerControllerImpl::onUserWantsToMoveToNextPosition(const bool next)
{
    const auto newLocation = shownLocation.withPosition(
        shownLocation.getPosition() + (next ? 1 : -1)
    );
    songController.setCurrentLocation(newLocation, false);
}

int PatternViewerControllerImpl::getChannelCount() const noexcept
{
    return getChannelCount(shownLocation.getSubsongId());
}

int PatternViewerControllerImpl::getChannelCount(const Id& subsongId) const noexcept
{
    return songController.getChannelCount(subsongId);
}

int PatternViewerControllerImpl::getPositionHeight() const noexcept
{
    return height;
}

void PatternViewerControllerImpl::tryToMoveVertically(const int step) noexcept
{
    tryToMoveVertically(step, false);
}

void PatternViewerControllerImpl::tryToMoveVertically(const int step, const OptionalBool enlargeSelection) noexcept
{
    const auto wantedLine = shownLocation.getLine() + step;                 // May need correction!

    onUserWantsToGoTo({ }, wantedLine, enlargeSelection);
}

void PatternViewerControllerImpl::tryToMoveForward() noexcept
{
    tryToMoveVertically(currentStep, false);
}

void PatternViewerControllerImpl::onUserWantsToGoTo(const OptionalValue<CursorLocation> newCursorLocationOptional, const OptionalInt desiredLineToGoToOptional,
                                                    const OptionalBool enlargeSelection) noexcept
{
    // Before moving, keeps the current line and cursor location, but this is only useful for enlarging selection.
    const auto currentCursorLocation = cursorLocation;
    const auto currentlyShownLine = shownLocation.getLine();

    // There is no reason to limit the cursor location in X.
    if (newCursorLocationOptional.isPresent()) {
        cursorLocation = newCursorLocationOptional.getValueRef();

        // Notifies the change of X of the cursor.
        mainController.observers().getCursorObservers().applyOnObservers([&] (CursorObserver* observer) {
            observer->onCursorChannelMoved(cursorLocation);
        });
    }

    if (desiredLineToGoToOptional.isAbsent()) {
        // Cursor motion in X only.
        if (newCursorLocationOptional.isPresent()) {
            if (enlargeSelection.isPresent()) {
                // There is no line to go to, but the cursor has moved.
                enlargeSelectionToCursorOrDismissAndRefreshUi(enlargeSelection.getValue(), currentlyShownLine,
                                                              currentCursorLocation);
                setCurrentLineAndCursorLocationToView();
            }
        }

        //notifyBlockSelectionChange();  --> No, seems a bit strange UI-wise. Don't consider a horizontal motion sets the new play position.
        return;
    }

    const auto desiredLineToGoTo = desiredLineToGoToOptional.getValue();
    const auto newShownLocation = correctLocationFromShownLocation(shownLocation.withLine(desiredLineToGoTo));       // May need correction!

    // This is a patch to allow changing pattern while keeping the line number (see "design flaw" below).
    songController.setCurrentlyShownLine(desiredLineToGoTo);

    // Second hackish way to make sure the SongController has the shown location, useful when PlayFromHere.
    // For less hack, an improvement would be the overhaul talking about above in onUserWantsToGoTo. Probably not needed for now. ShownLocation would be removed.
    songController.storeCurrentLocation(newShownLocation);

    // If playing and following, this will change the line being played.
    if (isFollowing() && isPlaying()) {
        playerController.setPlayedLocation(newShownLocation);
    } else {
        // TO IMPROVE? DESIGN FLAW... Shouldn't it set the location in the SongController and wait for the observation change in onSongMetadataChanged????
        // That way, the shown location would be modified only by this callback? However, this vastly complicates the click+drag or click+shift...
        // So only overhaul if *really* necessary.

        // Not following: only change the viewed location.
        shownLocation = newShownLocation;

        // Takes care of the selection.
        if (enlargeSelection.isPresent()) {
            enlargeSelectionToCursorOrDismissAndRefreshUi(enlargeSelection.getValue(), currentlyShownLine,
                                                          currentCursorLocation);
        }

        // Shows the new line.
        setCurrentLineAndCursorLocationToView();

        notifyBlockSelectionChange();       // So that the cursor is transmitted as a block selection, if required.
    }
}

void PatternViewerControllerImpl::setCurrentLineAndCursorLocationToView() const noexcept
{
    patternViewerView->setShownLineAndCursorLocation(shownLocation.getLine(), cursorLocation);

    updateInformationViewAndEffectContext();
}

void PatternViewerControllerImpl::onUserWantsToToggleMinimize()
{
    minimizedChannelIndexes = actionManager.toggleMinimize(minimizedChannelIndexes);

    // Updates the UI.
    sendDisplayedMetadataToView();
}

void PatternViewerControllerImpl::onUserWantsToToggleRecord()
{
    recording = !recording;
    patternViewerView->setRecordingState(recording);
    // Notifies.
    mainController.observers().getPatternViewerMetadataObservers().applyOnObservers([&] (PatternViewerMetadataObserver* observer) {
        observer->onRecordStateChanged(recording);
    });
}

void PatternViewerControllerImpl::onUserWantsToToggleFollow()
{
    switch (follow) {
        case Follow::followIfRecordOff:
            follow = Follow::followOn;
            break;
        case Follow::followOn:
            follow = Follow::followOff;
            break;
        case Follow::followOff:
            follow = Follow::followIfRecordOff;
            break;
    }
    patternViewerView->setFollowMode(follow);
}

void PatternViewerControllerImpl::onUserWantsToToggleWriteInstrument()
{
    writeInstrumentOnWriteNote = !writeInstrumentOnWriteNote;
    patternViewerView->setWriteInstrumentState(writeInstrumentOnWriteNote);
}

void PatternViewerControllerImpl::onUserWantsToToggleOverwriteDefaultEffect()
{
    overwriteDefaultEffect = !overwriteDefaultEffect;
    patternViewerView->setOverwriteDefaultEffect(overwriteDefaultEffect);
}

void PatternViewerControllerImpl::onUserWantsToChangeDefaultEffect(const bool next)
{
    const auto& effects = getDefaultEffects();
    const auto newEffect = CollectionUtil::getNextOrPrevious(effects, defaultEffect, next);
    setDefaultEffectAndRefreshUi(newEffect);
}

bool PatternViewerControllerImpl::isRecording() noexcept
{
    return recording;
}

void PatternViewerControllerImpl::onUserWantsToDelete(const bool wholeCell)
{
    // Works for all kind of tracks.
    const auto selection = actionManager.getCursorAsSelection(wholeCell);
    const auto selectedData = actionManager.buildSelectedData(selection);

    cursorManager.manageItemsToDelete(selectedData);
}

void PatternViewerControllerImpl::onUserWantsToModifyTrackHeight(const int newHeight)
{
    setNewTrackHeightAndUpdate(newHeight);
}

void PatternViewerControllerImpl::onUserWantsToModifyTrackHeightWithOffset(const int offset)
{
    // What is the current height?
    const auto positionData = getPositionData();

    setNewTrackHeightAndUpdate(positionData.getHeight() + offset);
}

void PatternViewerControllerImpl::onUserWantsToModifyTransposition(const int channelIndex, const int newTransposition)
{
    const auto correctedTransposition = NumberUtil::correctNumber(newTransposition, Position::minimumTransposition, Position::maximumTransposition);
    songController.setPositionTransposition(shownLocation, channelIndex, correctedTransposition);
}

void PatternViewerControllerImpl::onUserWantsToSetTrackNameAndColor(const int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor)
{
    songController.setTrackNameAndColor(shownLocation, channelIndex, newName, newColor);
}

void PatternViewerControllerImpl::onUserWantsToRenameSpecialTrack(const bool isSpeedTrack, const juce::String& newName)
{
    songController.setSpecialTrackName(shownLocation, isSpeedTrack, newName);
}

void PatternViewerControllerImpl::onUserWantsToCreateLink(const int sourcePositionIndex, const int sourceChannelIndex, const int targetPositionIndex, const int targetChannelIndex)
{
    songController.createTrackLink(getSubsongId(), sourcePositionIndex, sourceChannelIndex, targetPositionIndex, targetChannelIndex);
}

void PatternViewerControllerImpl::onUserWantsToUnlink(const int positionIndexToUnlink, const int channelIndexToUnlink) noexcept
{
    songController.unlinkTrack(getSubsongId(), positionIndexToUnlink, channelIndexToUnlink);
}

void PatternViewerControllerImpl::onUserWantsToGoTo(const int positionIndexToGoTo, const int channelIndexToGoTo) noexcept
{
    // First moves on the channel, then performs the goto position.
    onUserWantsToGoTo(CursorLocation::onTrack(channelIndexToGoTo, CellCursorRank::first), 0, false);

    const auto newLocation = shownLocation.withPosition(positionIndexToGoTo);
    songController.setCurrentLocation(newLocation, false);
}

void PatternViewerControllerImpl::onUserWantsToGoToChannel(const int channelIndexToGoTo) noexcept
{
    onUserWantsToGoTo(CursorLocation::onTrack(channelIndexToGoTo, CellCursorRank::first), { }, false);
}

void PatternViewerControllerImpl::onUserWantsToCreateSpecialLink(const bool isSpeedTrack, const int sourcePositionIndex, const int targetPositionIndex)
{
    songController.createSpecialTrackLink(getSubsongId(), isSpeedTrack, sourcePositionIndex, targetPositionIndex);
}

void PatternViewerControllerImpl::onUserWantsToUnlinkSpecial(const bool isSpeedTrack, const int positionIndexToUnlink) noexcept
{
    songController.unlinkSpecialTrack(getSubsongId(), isSpeedTrack, positionIndexToUnlink);
}

void PatternViewerControllerImpl::onUserWantsToGoToSpecialTrack(const bool isSpeedTrack, const int positionIndexToGoTo) noexcept
{
    // First moves on the channel, then performs the goto position.
    onUserWantsToGoTo(CursorLocation::onSpecialTrack(isSpeedTrack, SpecialCellCursorRank::first), 0, false);

    const auto newLocation = shownLocation.withPosition(positionIndexToGoTo);
    songController.setCurrentLocation(newLocation, false);
}

void PatternViewerControllerImpl::onUserClickedOnRankOfNormalTrack(const int channelIndex, const int rankFromCenterAsOrigin, const CellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    clickedDownCellCursorRank = cursorRank;     // Stores it for the Up, to know where to go if no selection is dragged.

    onUserClickedOnRank(rankFromCenterAsOrigin, leftButton, shift,  [&](int clickedIndex) -> Selection {
        // Returns a selection on the track of the channel index.
        return { clickedIndex, CursorLocation::onTrack(channelIndex, cursorRank) };
    });
}

void PatternViewerControllerImpl::onUserClickedOnRank(const int rankFromCenterAsOrigin, const bool leftButton, const bool shift, const std::function<Selection(int)>& getSelection) noexcept
{
    selectingOnLineTrack = false;

    // The right button opens a contextual menu.
    if (!leftButton) {
        actionManager.openContextualMenu();
        return;
    }

    // Left click. Determines the real clicked cell index.
    const auto clickedIndex = determineCorrectedLineIndexFromRank(rankFromCenterAsOrigin);  // The user could click on "black" area.

    // If shift is not pressed, sets the selection at this location.
    // If shift is pressed, enlarges the current selection to it.
    const auto newSelection = getSelection(clickedIndex);
    if (shift) {
        if (currentSelection.isEmpty()) {
            // No selection. Then uses the cursor as a start.
            currentSelection = Selection(shownLocation.getLine(), cursorLocation, newSelection.getEndLine(), newSelection.getEndLocation(),
                                         newSelection.isCreatedByMouse(), newSelection.isGrown());
        } else {
            // A selection already exists. Simply changes its end.
            currentSelection = currentSelection.withEnd(newSelection.getEndLine(), newSelection.getEndLocation());
        }
    } else {
        currentSelection = newSelection.enlargeSelectionIfNeeded();
    }

    refreshStoredSongDataAndUpdateView(false);
}

int PatternViewerControllerImpl::determineCorrectedLineIndexFromRank(const int rankFromCenterAsOrigin) const noexcept
{
    return correctIndexFromCurrentHeight(rankFromCenterAsOrigin + shownLocation.getLine());
}

void PatternViewerControllerImpl::enlargeSelectionToCursorOrDismissAndRefreshUi(const bool enlargeOrDismiss, const int cursorLineIndexBeforeMoving, const CursorLocation& cursorLocationBeforeMoving) noexcept
{
    // Note: we consider this is called ONLY when moving via the keyboard, not the mouse. The selection management relies on that distinction
    // to know if it must recreate the selection or not.

    const auto createdByMouse = currentSelection.isCreatedByMouse();
    const auto grown = currentSelection.isGrown();

    if (!enlargeOrDismiss) {
        // The selection should be dismissed. As an optimization, if it was already dismissed, no need to refresh.
        if (currentSelection.isEmpty()) {
            return;
        }
        currentSelection = Selection();
    } else {
        Selection newSelection;
        // Enlarges the selection.
        if (currentSelection.isEmpty()) {
            // The selection is empty, so grows from where the cursor WAS.
            newSelection = Selection(cursorLineIndexBeforeMoving, cursorLocationBeforeMoving, shownLocation.getLine(), cursorLocation, createdByMouse, grown);
        } else {
            // The selection already exists. But is it a "keyboard" selection? If created from a mouse, it is recreated
            // (so that mouse+drag then cursor+shift creates a new selection).
            if (currentSelection.isCreatedByMouse()) {
                // Creates a whole new selection from where the cursor was.
                newSelection = Selection(cursorLineIndexBeforeMoving, cursorLocationBeforeMoving, shownLocation.getLine(), cursorLocation, createdByMouse, grown);
            } else {
                newSelection = currentSelection.withEnd(shownLocation.getLine(), cursorLocation);
            }
        }
        // Grows the selection, if needed.
        currentSelection = newSelection.enlargeSelectionIfNeeded();
    }
    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onUserClickIsUpFromNormalTrack(const bool leftButton)
{
    onUserClickIsUpFromAnyTrack(leftButton);
}

void PatternViewerControllerImpl::onUserClickIsUpFromAnyTrack(const bool leftButton) noexcept
{
    // We only care about the left button.
    if (!leftButton) {
        return;
    }

    // If the selection is not a single cell, there is nothing else to do.
    if (!currentSelection.isSingle()) {
        return;
    }

    // The selection is a single cell. Hides it and goes to it.
    // We can be on a normal track or a Special track.
    const auto& startLocation = currentSelection.getStartLocation();
    const auto trackType = startLocation.getTrackType();

    CursorLocation newCursorLocation;
    switch (trackType) {
        default: [[fallthrough]];
        case TrackType::line:
            jassertfalse;       // Not supposed to happen.
            return;
        case TrackType::normal:
        {
            const auto channelIndex = startLocation.getChannelIndex();
            newCursorLocation = CursorLocation::onTrack(channelIndex, clickedDownCellCursorRank);   // Reuses the rank stored on Down.
            break;
        }
        case TrackType::speed: [[fallthrough]];
        case TrackType::event:
            // No need to store the rank like on a Normal Track, because there are only two digits, so the start is enough!
            newCursorLocation = CursorLocation::onSpecialTrack((trackType == TrackType::speed), startLocation.getRankAsSpecialCursorRank());
            break;
    }

    const auto lineIndex = currentSelection.getTopLine();                       // Same remark.

    // No more selection.
    currentSelection = Selection();
    // Go to!
    onUserWantsToGoTo(newCursorLocation, lineIndex, false);
    refreshStoredSongDataAndUpdateView(false);        // Necessary, else the selection doesn't disappear.
}

void PatternViewerControllerImpl::onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event)
{
    onUserDraggedCursor(event);
}

void PatternViewerControllerImpl::onUserDraggedCursor(const juce::MouseEvent& event, const bool checkHorizontalScrolling) noexcept
{
    // We only care about the left button.
    if (!event.mods.isLeftButtonDown()) {
        return;
    }

    // Horizontal scrolling?
    if (checkHorizontalScrolling) {
        if (const auto horizontalOutOfBounds = patternViewerView->determineOutOfBoundsDirectionFromMouseEvent(event); horizontalOutOfBounds.isPresent()) {
            auto viewPortX = patternViewerView->getViewportX();
            viewPortX += horizontalOutOfBounds.getValue() ? -dragHorizontalScrollingSpeed : dragHorizontalScrollingSpeed;
            patternViewerView->setViewportX(viewPortX);
            return;
        }
    }

    // Determines if a CellView/SpecialCellView is hovered.
    const auto relativeEvent = event.getEventRelativeTo(patternViewerView.get());
    const auto relativePosition = relativeEvent.position;

    auto* component = patternViewerView->getComponentAt(relativePosition.toInt());
    if (auto* cellView = dynamic_cast<CellView*>(component); cellView != nullptr) {
        if (!selectingOnLineTrack) {            // Corner case: dragging on the Line Track and "overflowing" on a normal Track. Ignore the event.
            onUserDraggedCursorOverCellView(event, *cellView);
        }
    } else {
        // Or maybe a SpecialCell? Warning, could be a Line Cell.
        if (const auto* simpleCellView = dynamic_cast<SimpleCellView*>(component); simpleCellView != nullptr) {
            onUserDraggedCursorOverSimpleCellView(event, *simpleCellView);
        } else {
            // Or maybe a scrolling must be performed? Not the best code ever, but it's not so simple to manage.
            // This automatically grows the selection to the top or bottom of the VISIBLE PV.
            auto* child = patternViewerView->getChildComponent(0);
            if (auto* abstractTrackView = dynamic_cast<AbstractTrackView*>(child); abstractTrackView != nullptr) {
                const auto relativeEventToTrackView = event.getEventRelativeTo(abstractTrackView);
                const auto relativeY = static_cast<int>(relativeEventToTrackView.position.getY());
                if (relativeY < AbstractTrackView::headerHeight) {
                    tryToMoveVertically(-1, OptionalBool());
                    extendSelectionToTopOrBottomVisibleLine(*abstractTrackView, true);
                } else if (relativeY >= abstractTrackView->getHeight()) {
                    tryToMoveVertically(1, OptionalBool());
                    extendSelectionToTopOrBottomVisibleLine(*abstractTrackView, false);
                }
            }
        }
    }
}

void PatternViewerControllerImpl::extendSelectionToTopOrBottomVisibleLine(const AbstractTrackView& abstractTrackView, const bool topOrBottomTarget) noexcept
{
    // Gets the ViewRank at the bottom of the VISIBLE PV, and makes it the target of the selection.
    const auto cellRank = topOrBottomTarget ? abstractTrackView.getRankOfFirstCell() : abstractTrackView.getRankOfLastCell();
    const auto newEndIndex = determineClickedIndex(abstractTrackView, cellRank);
    if (selectingOnLineTrack) {
        currentBlockSelection = currentBlockSelection.withEnd(newEndIndex, currentBlockSelection.getEndLocation()).withCreator(true);
    } else {
        currentSelection = currentSelection.withEnd(newEndIndex, currentSelection.getEndLocation()).withCreator(true).enlargeSelectionIfNeeded();
    }
    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onUserDraggedCursorOverCellView(const juce::MouseEvent& event, CellView& cellView) noexcept
{
    // Finds the parent TrackView in order to know its channel. It should be present!
    const auto* parentTrackView = dynamic_cast<const TrackView*>(cellView.getParentComponent());
    if (parentTrackView == nullptr) {
        jassertfalse;           // The CellView has TrackView as a parent! Abnormal!
        return;
    }

    const auto channelIndex = parentTrackView->getChannelIndex();
    // Determines the real clicked cell index.
    const auto cellRank = cellView.getViewRank();
    const auto clickedIndex = determineClickedIndex(*parentTrackView, cellRank);

    const auto relativeEventToCell = event.getEventRelativeTo(&cellView);
    const auto xInCell = static_cast<int>(relativeEventToCell.position.getX());
    const auto cellRankInt = cellView.getCellRankFromX(xInCell);

    // Sets the end of the current selection.
    currentSelection = currentSelection.withEnd(clickedIndex, CursorLocation::onTrack(channelIndex, static_cast<CellCursorRank>(cellRankInt)))
            .withCreator(true).enlargeSelectionIfNeeded();

    refreshStoredSongDataAndUpdateView(false);
}

int PatternViewerControllerImpl::determineClickedIndex(const AbstractTrackView& parentTrackView, const int cellRank) const noexcept
{
    const auto rankFromCenterAsOrigin = parentTrackView.determineRankFromCenterAsOrigin(cellRank);
    const auto clickedIndex = rankFromCenterAsOrigin + shownLocation.getLine();
    // Corrects it according to the current length.
    return NumberUtil::correctNumber(clickedIndex, 0, height - 1);
}

void PatternViewerControllerImpl::onUserDraggedCursorOverSimpleCellView(const juce::MouseEvent& /*event*/, const SimpleCellView& simpleCellView) noexcept
{
    const auto cellRank = simpleCellView.getViewRank();

    // Finds the parent SpecialTrackView in order to know its channel.
    // Is it a LineTrackView?
    if (const auto* parentLineTrackView = dynamic_cast<const LineTrackView*>(simpleCellView.getParentComponent()); parentLineTrackView != nullptr) {
        if (!selectingOnLineTrack) {            // Corner case: dragging on a Track and "overflowing" on a Line Track. Ignore the event.
            return;
        }
        // Grows the Line Selection to the clicked index.
        const auto clickedIndex = determineClickedIndex(*parentLineTrackView, cellRank);
        setBlockSelection(true, false, currentBlockSelection.withEnd(clickedIndex, CursorLocation::onLineTrack()));
        return;
    }

    // Then probably a Special TrackView.
    if (selectingOnLineTrack) {            // Corner case: dragging on a Line Track and "overflowing" on a Special Track. Ignore the event.
        return;
    }
    const auto* parentSpecialTrackView = dynamic_cast<const SpecialTrackView*>(simpleCellView.getParentComponent());
    if (parentSpecialTrackView == nullptr) {
        return;
    }

    jassert((parentSpecialTrackView->getTrackType() == TrackType::speed) || (parentSpecialTrackView->getTrackType() == TrackType::event));
    const auto isSpeedTrack = (parentSpecialTrackView->getTrackType() == TrackType::speed);
    // Determines the real clicked cell index.
    const auto clickedIndex = determineClickedIndex(*parentSpecialTrackView, cellRank);

    // Sets the end of the current selection.
    currentSelection = currentSelection.withEnd(clickedIndex, CursorLocation::onSpecialTrack(isSpeedTrack, SpecialCellCursorRank::last))
            .withCreator(true).enlargeSelectionIfNeeded();

    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onUserClickedOnRankOfSpecialTrack(const bool isSpeedTrack, const int rankFromCenterAsOrigin, const SpecialCellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    onUserClickedOnRank(rankFromCenterAsOrigin, leftButton, shift, [&](int clickedIndex) -> Selection {
        // Returns a selection on the special track.
        return { clickedIndex, CursorLocation::onSpecialTrack(isSpeedTrack, cursorRank) };
    });
}

void PatternViewerControllerImpl::onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event)
{
    onUserDraggedCursor(event);
}

void PatternViewerControllerImpl::onUserClickIsUpFromSpecialTrack(const bool leftButton)
{
    onUserClickIsUpFromAnyTrack(leftButton);
}

void PatternViewerControllerImpl::onUserClickedOnRankOfLineTrack(const int rankFromCenterAsOrigin, const bool leftButton)
{
    selectingOnLineTrack = true;

    // Determines the real clicked cell index.
    const auto clickedIndex = determineCorrectedLineIndexFromRank(rankFromCenterAsOrigin);  // The user could click on "black" area.

    // If right button, selects from here to the bottom.
    setBlockSelection(true, false, Selection::buildForLineTrack(clickedIndex, leftButton ? clickedIndex : (height - 1)));
}

void PatternViewerControllerImpl::onUserClickIsUpFromLineTrack(bool /*leftButton*/)
{
    // On up, if the selection is too small, makes it as small as possible.
    if (currentBlockSelection.getHeight() < 4) {
        setBlockSelection(true, false,        // No notification, done just after.
            Selection(currentBlockSelection.getTopLine(), currentBlockSelection.getStartLocation(), currentBlockSelection.isCreatedByMouse()));
    }

    notifyBlockSelectionChange();
}

void PatternViewerControllerImpl::onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event)
{
    // Don't try to make a horizontal scrolling, it would always try to do it because the Line Track is considered "out of bounds".
    onUserDraggedCursor(event, false);
}

void PatternViewerControllerImpl::onKeyboardNote(const int baseNote)
{
    //DBG("KEYBOARD NOTE from PV: " + juce::String(baseNote));
    const auto currentOctave = mainController.getPlayerController().getCurrentOctave();
    const auto note = NoteUtil::getNote(baseNote, currentOctave);

    onNote(note);
}

void PatternViewerControllerImpl::onNote(const int note)
{
    cursorManager.manageNoteReceived(note);
}

bool PatternViewerControllerImpl::onKeyPressed(const juce::KeyPress& key)
{
    return cursorManager.onKeyPressed(key);
}

void PatternViewerControllerImpl::onUserWantsToPlayLine()
{
    // Plays the line.
    playerController.playLine(shownLocation);

    tryToMoveForward();
}

void PatternViewerControllerImpl::onUserWantsToTogglePlayPatternFromCursorOrBlock(const bool forceSetBlock)
{
    // If already playing, stops.
    if (isPlaying()) {
        playerController.stopPlaying();
        return;
    }

    auto newBlock = currentBlockSelection;
    if (!forceSetBlock && newBlock.isEmpty()) {
        // Not forced, and the block is empty. Starts the pattern without setting the block.
        playerController.playPatternFromStart();
        return;
    }

    // If forced, creates a new block where the cursor is. Else, plays from the current block.
    // In all cases, corrects the Block to make sure the current Pattern can hold it.
    if (forceSetBlock) {
        const auto currentLine = shownLocation.getLine();
        newBlock = Selection::buildForLineTrack(currentLine, currentLine);
    }

    setBlockSelection(true, true, correctBlock(newBlock));
    playerController.playPatternFromShownLocationOrBlock();

}

void PatternViewerControllerImpl::onUserWantsToTogglePlayPatternFromStart()
{
    // If already playing, stops.
    if (isPlaying()) {
        playerController.stopPlaying();
        return;
    }

    playerController.playPatternFromStart();
}

void PatternViewerControllerImpl::onUserWantsToCopy()
{
    actionManager.onUserWantsToCopy();
}

void PatternViewerControllerImpl::onUserWantsToCut()
{
    actionManager.onUserWantsToCut();
}

void PatternViewerControllerImpl::onUserWantsToPaste(const bool moveBelowAfter, const bool pasteContinuously)
{
    actionManager.onUserWantsToPaste(moveBelowAfter, pasteContinuously);
}

void PatternViewerControllerImpl::onUserWantsToPasteMix()
{
    actionManager.onUserWantsToPasteMix();
}

void PatternViewerControllerImpl::onUserWantsToClearSelection()
{
    actionManager.onUserWantsToClearSelection();
}

void PatternViewerControllerImpl::onUserWantsToTransposeSelection(const TransposeRate transposeRate)
{
    actionManager.onUserWantsToTransposeSelection(transposeRate);
}

void PatternViewerControllerImpl::onUserWantsToToggleReadOnly()
{
    actionManager.onUserWantsToToggleReadOnly();
}

void PatternViewerControllerImpl::onUserWantsToGenerateArpeggio()
{
    actionManager.onUserWantsToGenerateArpeggio();
}

void PatternViewerControllerImpl::onUserWantsToGenerateInstrument()
{
    actionManager.onUserWantsToGenerateInstrument();
}

void PatternViewerControllerImpl::onUserWantsToSetTrackNameAndColor()
{
    const auto selection = actionManager.getSelectionOrCursor();
    if (selection.containsSpecialTrack()) {
        const auto isSpeedTrack = selection.containsSpecialTrack(true);
        const auto& specialTrack = isSpeedTrack ? speedTrack : eventTrack;
        const auto currentName = specialTrack.getName();
        patternViewerView->onUserWantsToSetSpecialTrackName(isSpeedTrack, currentName);
    } else {
        const auto channelIndex = selection.getStartLocation().getChannelIndex();
        const auto& track = channelToTrack.at(channelIndex);
        const auto currentName = track.getName();
        const auto currentTrackColor = track.getColor();
        const auto optionalTrackColor = currentTrackColor.isPresent() ? juce::Colour(currentTrackColor.getValueRef()) : OptionalValue<juce::Colour>();
        patternViewerView->onUserWantsToSetMusicTrackNameAndColor(channelIndex, currentName, optionalTrackColor);
    }
}

void PatternViewerControllerImpl::onUserWantsToLinkTrack()
{
    const auto selection = actionManager.getSelectionOrCursor();
    if (selection.containsSpecialTrack()) {
        patternViewerView->onUserWantsToSetSpecialLink(selection.containsSpecialTrack(true));
    } else {
        const auto channelIndex = selection.getStartLocation().getChannelIndex();
        patternViewerView->onUserWantsToSetLink(channelIndex);
    }
}

void PatternViewerControllerImpl::onUserWantsToInsert()
{
    actionManager.onUserWantsToInsert();
}

void PatternViewerControllerImpl::onUserWantsToRemove()
{
    actionManager.onUserWantsToRemove();
}

void PatternViewerControllerImpl::onUserWantsToSetEditStep(const int stepToValidate)
{
    const auto newSteps = NumberUtil::correctNumber(stepToValidate, 0, maximumStep);
    currentStep = newSteps;
    patternViewerView->setSteps(newSteps);
}

void PatternViewerControllerImpl::onUserWantsToIncreaseEditStep(const int offset)
{
    onUserWantsToSetEditStep(currentStep + offset);
}

void PatternViewerControllerImpl::onUserSelectedLockedEffect(const Effect effect)
{
    jassert((effect != Effect::noEffect) && (effect != Effect::legato));
    defaultEffect = effect;
}

void PatternViewerControllerImpl::onUserWantsToToggleSoloTrack()
{
    // Where is the cursor? This indicates what channel to collapse.
    const auto optional = cursorLocation.getChannelIndexAndRankIfOnTrack();
    if (optional.isAbsent()) {
        return;
    }
    mainController.onWantToAllMuteExcept(optional.getValue().first);
}

void PatternViewerControllerImpl::onUserWantsToToggleMuteTrack()
{
    // Where is the cursor? This indicates what channel to collapse.
    const auto optional = cursorLocation.getChannelIndexAndRankIfOnTrack();
    if (optional.isAbsent()) {
        return;
    }
    mainController.onWantToToggleMuteState(optional.getValue().first);
}

void PatternViewerControllerImpl::onUserWantsToSelectTrack()
{
    const auto selection = actionManager.getSelectionOrCursor().growSelectionHorizontally().growHeight();
    currentSelection = selection;

    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onUserWantsToSelectAllTracks()
{
    // Will switch between all music tracks and all tracks.

    // Makes a selection with only the music tracks.
    constexpr auto startLine = 0;
    const auto endLine = TrackConstants::lastPossibleIndex;
    constexpr auto createdByMouse = false;
    constexpr auto grown = false;

    const auto cursorStart = CursorLocation::onTrack(0, CellCursorRank::first);
    const auto cursorEnd = CursorLocation::onTrack(getChannelCount() - 1, CellCursorRank::last);
    const auto allMusicTrackSelection = Selection(startLine, cursorStart, endLine, cursorEnd,
        createdByMouse, grown);

    // If the selection is not the all tracks, make it so.
    if (currentSelection != allMusicTrackSelection) {
        currentSelection = allMusicTrackSelection;
    } else {
        // We already have the music tracks selected. Grows to all the tracks.
        const auto newCursorEnd = CursorLocation::onEventTrack(SpecialCellCursorRank::last);
        currentSelection = Selection(startLine, cursorStart, endLine, newCursorEnd,
        createdByMouse, grown);
    }

    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::getKeyboardFocus() noexcept
{
    patternViewerView->grabKeyboardFocus();
}

void PatternViewerControllerImpl::updateInformationViewAndEffectContext() const noexcept
{
    const auto [isError, informationText] = cursorManager.getInformationText();
    patternViewerView->setInformationText(informationText, isError);

    const auto [channelIndex, effectContextText] = cursorManager.buildEffectContextText();
    patternViewerView->setEffectContextText(channelIndex, effectContextText);
}

int PatternViewerControllerImpl::correctIndexFromCurrentHeight(const int cellIndex) const noexcept
{
    jassert(height > 0);
    return NumberUtil::correctNumber(cellIndex, 0, height - 1);
}

bool PatternViewerControllerImpl::isReadOnlyTrack(const int channelIndex) noexcept
{
    if (const auto iterator = channelToTrack.find(channelIndex); iterator != channelToTrack.cend()) {
        return iterator->second.isReadOnly();
    }

    jassertfalse;   // Not found?
    return false;
}

bool PatternViewerControllerImpl::canGenerateArpeggioFromSelection() noexcept
{
    return actionManager.getNotesForGeneration().size() > 1U;
}

bool PatternViewerControllerImpl::canGenerateInstrumentFromSelection() noexcept
{
    return !actionManager.getNotesForGeneration().empty();
}

bool PatternViewerControllerImpl::canSetNameAndColorToTrack() noexcept
{
    // Method also used below.
    return actionManager.getSelectionOrCursor().isSingleChannel();
}

bool PatternViewerControllerImpl::canLinkTrack() noexcept
{
    return canSetNameAndColorToTrack();
}

int PatternViewerControllerImpl::getTrackIndex(const int channelIndex) noexcept
{
    auto trackIndex = 0;
    songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const Subsong& subsong) {
        trackIndex = subsong.getPatternRef(shownLocation.getPosition()).getCurrentTrackIndex(channelIndex);
    });

    return trackIndex;
}

int PatternViewerControllerImpl::getPositionIndex() noexcept
{
    return shownLocation.getPosition();
}

Id PatternViewerControllerImpl::getSubsongId() noexcept
{
    return shownLocation.getSubsongId();
}

Effect PatternViewerControllerImpl::getDefaultEffect() const noexcept
{
    return defaultEffect;
}

const std::vector<Effect>& PatternViewerControllerImpl::getDefaultEffects() const noexcept
{
    static const std::vector effects = {
        Effect::reset, Effect::volume, Effect::volumeIn, Effect::volumeOut,
        Effect::arpeggioTable,
        Effect::pitchTable, Effect::arpeggio3Notes,Effect::arpeggio4Notes,
        Effect::pitchUp, Effect::pitchDown, Effect::pitchGlide,
        Effect::fastPitchUp, Effect::fastPitchDown,
        Effect::forceInstrumentSpeed, Effect::forceArpeggioSpeed, Effect::forcePitchTableSpeed,
    };

    jassert(static_cast<int>(effects.size()) == static_cast<int>(Effect::lastEffect2_0) - 1);        // One effect missing? -1 because if "no effect" not visible.

    return effects;
}

void PatternViewerControllerImpl::showOrHideToolbox(const bool open, juce::Component& parent, const int toolBoxButtonX, const int toolBoxButtonY) noexcept
{
    if (open) {
        toolboxController->show(parent, toolBoxButtonX, toolBoxButtonY);
    } else {
        toolboxController->hide();
    }
}

void PatternViewerControllerImpl::locateToolbox(const int toolBoxButtonX, const int toolBoxButtonY) noexcept
{
    toolboxController->locate(toolBoxButtonX, toolBoxButtonY);
}

void PatternViewerControllerImpl::showWarningBecauseRecordingOff() noexcept
{
    patternViewerView->showWarningBecauseRecordingOff();
}


// ChannelMuteObserver method implementations.
// ================================================

void PatternViewerControllerImpl::onChannelsMuteStateChanged(const std::unordered_set<int>& newMutedChannelIndexes)
{
    mutedChannelIndexes = newMutedChannelIndexes;
    // Updates the UI.
    sendDisplayedMetadataToView();
}


// SongMetadataObserver method implementations.
// ==============================================

void PatternViewerControllerImpl::onSongMetadataChanged(const unsigned int what)
{
    // New locations? These are locations given by the linker, for example. Not the player moving.
    if (!NumberUtil::isBitPresent(what, SongMetadataObserver::What::shownLocation)) {
        return;
    }
    const auto newLocation = songController.getCurrentViewedLocation();
    const auto newSubsongId = newLocation.getSubsongId();
    const auto subsongChanged = (newSubsongId != shownLocation.getSubsongId());
    storeShownLocationWithHeightCorrection(newLocation);

    channelToLinkedState.clear();       // Force refreshes the link states when changing position.

    if (!subsongChanged) {
        refreshStoredSongDataAndUpdateView(false);
    } else {
        // When changing Subsong, refreshes the metadata too. Cancels the cached Tracks, we don't know what the views have anymore.
        sendDisplayedMetadataToView();
        refreshStoredSongDataAndUpdateView(true);
    }

    toolboxController->onSongMetadataChanged(newSubsongId);
}


// SubsongMetadataObserver method implementations.
// ==================================================

void PatternViewerControllerImpl::onSubsongMetadataChanged(const Id& subsongId, const unsigned int what)
{
    // Only interested in the shown Subsong.
    if (subsongId != shownLocation.getSubsongId()) {
        return;
    }
    if (!NumberUtil::isBitPresent(what, subsongMetadata) && !NumberUtil::isBitPresent(what, psgsData)) {
        return;
    }

    if (NumberUtil::isBitPresent(what, psgsData)) {
        // Maybe less PSGs? Makes sure the cursor is still within bounds.
        cursorLocation = correctCursorLocation(cursorLocation);
    }

    sendDisplayedMetadataToView();          // Updates the view to show the new highlight spacing.
    toolboxController->onSubsongMetadataChanged(subsongId);
}


// SongPlayerObserver method implementations.
// ==============================================

void PatternViewerControllerImpl::onPlayerNewLocations(const SongPlayerObserver::Locations& locations) noexcept
{
    // UI thread.

    // Don't do anything else if we aren't on the same subsong.
    if (locations.playedLocation.getSubsongId() != shownLocation.getSubsongId()) {
        return;
    }

    // If neither pattern nor song is being played, don't take in account the player. This allows the Play Line and play after a pattern change to work.
    if (!isPlaying()) {
        return;
    }

    // Stores the PlayedLocation only now. If doing before, recording note without playing will show the played line move.
    const auto& playerPlayedLocation = locations.playedLocation;
    playedLocation = playerPlayedLocation;

    if (isFollowing()) {
        // Following: stick to the played location.
        shownLocation = locations.playedLocation;
    } else {
        // Not following, only stays on the same pattern.
        storeShownLocationWithHeightCorrection(shownLocation.withPosition(playerPlayedLocation.getPosition()));
    }
    // A bit hackish way to make sure the SongController has the shown location, useful when PlayFromHere.
    // For less hack, an improvement would be the overhaul talking about above in onUserWantsToGoTo. Probably not needed for now. ShownLocation would be removed.
    songController.storeCurrentLocation(shownLocation);

    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onNewPsgRegisters(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept
{
    // UI thread.
    // Only stores the registers. They will be read in due time thanks to the timer.
    psgIndexToPendingRegisters.clear();
    for (const auto&[psgIndex, data] : psgIndexToPsgRegistersAndSampleData) {
        psgIndexToPendingRegisters.insert(std::make_pair(psgIndex, data.first));
    }
}


// LinkerObserver method implementations.
// ==============================================

void PatternViewerControllerImpl::onLinkerPositionChanged(const Id& subsongId, int /*index*/, const unsigned int whatChanged)
{
    onLinkerChange(subsongId, whatChanged);
}

void PatternViewerControllerImpl::onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& /*indexes*/, const unsigned int whatChanged)
{
    onLinkerChange(subsongId, whatChanged);
}

void PatternViewerControllerImpl::onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& /*highlightedItems*/)
{
    onLinkerChange(subsongId, LinkerObserver::What::all);
}

void PatternViewerControllerImpl::onLinkerPatternInvalidated(const Id& /*subsongId*/, int /*patternIndex*/, unsigned int /*whatChanged*/)
{
    // Nothing to do, we don't care about the color or name of the Pattern.
}

void PatternViewerControllerImpl::onLinkerChange(const Id& subsongIdWhereChange, const unsigned int what) noexcept
{
    // Same subsong? If not, don't do anything.
    if (shownLocation.getSubsongId() != subsongIdWhereChange) {
        return;
    }

    // Invalidates the Link map, for it to be refreshed, if it is what has changed.
    if (NumberUtil::isBitPresent(what, link)) {
        channelToLinkedState.clear();
    }

    refreshStoredSongDataAndUpdateView(false);
}


// PatternViewerMetadataObserver method implementations.
// =======================================================

void PatternViewerControllerImpl::onPatternViewerMetadataChanged()
{
    areLineNumberInHexadecimal = preferences.isPatternViewerLineNumberInHexadecimal();
    fontSize = preferences.getPatternViewerFontHeight();
    zoomMiddleLine = preferences.getPatternViewerMustZoomMiddleLine();
    mouseWheelSteps = preferences.getPatternViewerMouseWheelSteps();
    effectToChar = preferences.getEffectToChar();

    sendDisplayedMetadataToView();
}

void PatternViewerControllerImpl::onRecordStateChanged(bool /*newIsRecordingState*/)
{
    // Nothing to do.
}


// ==============================================

void PatternViewerControllerImpl::sendDisplayedMetadataToView() noexcept
{
    const auto displayedMetadata = updateInternalDisplayedMetadataAndBuild();
    // Builds DisplayedData with as minimum data as possible.
    const auto displayedData = PatternViewerView::DisplayedData(shownLocation.getLine(), cursorLocation, height, { },
                                                          channelToLinkedState, nullptr,
                                                          nullptr, speedTrackLinkState, eventTrackLinkState,
                                                          channelToTransposition, determinePlayedLocationLine(),
                                                          currentSelection, currentBlockSelection);
    patternViewerView->setDisplayedMetadata(displayedMetadata, displayedData);
}

void PatternViewerControllerImpl::refreshStoredSongDataAndUpdateView(const bool forceRefresh, const bool refreshMetadata) noexcept
{
    auto validPosition = false;
    const auto mustRefreshLinkStates = channelToLinkedState.empty();    // Optimization to avoid refreshing the links every time.

    // Extracts some data from the current Subsong.
    auto channelCount = 0;
    songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const Subsong& subsong) {
        const auto psgCount = subsong.getPsgCount();

        channelCount = PsgValues::getChannelCount(psgCount);

        // Makes sure the position exists. Possibly not if the last Position was deleted.
        const auto positionCount = subsong.getLength();
        const auto positionIndexToShow = shownLocation.getPosition();
        validPosition = (positionIndexToShow < positionCount);

        if (validPosition) {
            const auto& positionData = subsong.getPositionRef(positionIndexToShow);
            channelToTransposition = positionData.getChannelToTransposition();
            height = positionData.getHeight();
        }
    });

    if (!validPosition) {
        jassertfalse;       // Showing a deleted position? Do nothing. Probably enough because an event should be fired to correct the shown location.
        return;
    }

    // Gets the tracks from the Song. Checks whether the ones stored locally have changed.
    std::unordered_map<int, const Track*> channelToChangedTrack;        // Sent to the UI to indicate what has changed.
    std::unordered_map<int, LinkState> localChannelToLinkedState;       // Ignored if there is no need to refresh the Link state.

    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        auto iterator = channelToTrack.find(channelIndex);
        auto mustRefreshTrack = true;
        // Gets the Song Track reference.
        const Track* trackInSong = nullptr;
        songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const Subsong& subsong) {
            const auto positionIndex = shownLocation.getPosition();
            trackInSong = &subsong.getConstTrackRefFromPosition(positionIndex, channelIndex);

            // Is the track linked?
            if (mustRefreshLinkStates) {
                const auto linkState = subsong.getTrackLinkState(positionIndex, channelIndex);
                localChannelToLinkedState[channelIndex] = linkState;

                // The same for the Special Tracks.
                speedTrackLinkState = subsong.getSpecialTrackLinkState(true, positionIndex);
                eventTrackLinkState = subsong.getSpecialTrackLinkState(false, positionIndex);
            }
        });

        if (iterator != channelToTrack.cend()) {
            // A local Track is present. Is it stale? Gets the distant Track to compare.
            const Track* localTrack = &iterator->second;

            // Must refresh the Track?
            mustRefreshTrack = !localTrack->isSameModificationCounter(*trackInSong);
        }

        if (mustRefreshTrack || forceRefresh) {
            // Overwrites the local Track.
            channelToTrack[channelIndex] = *trackInSong;

            channelToChangedTrack.insert(std::make_pair(channelIndex, trackInSong));
        }
    }
    if (mustRefreshLinkStates) {
        channelToLinkedState = localChannelToLinkedState;
    }

    // Speed/Event Track. Did it change? Updates the internal values.
    const auto* speedTrackIfChanged = getSongDataAndUpdateViewInternalForSpecialTrack(speedTrack, true, forceRefresh);
    const auto* eventTrackIfChanged = getSongDataAndUpdateViewInternalForSpecialTrack(eventTrack, false, forceRefresh);

    // Sends the data to the UI. Only the changed tracks are sent.
    const auto displayedData = PatternViewerView::DisplayedData(shownLocation.getLine(), cursorLocation, height, channelToChangedTrack,
                                                          channelToLinkedState,
                                                          speedTrackIfChanged, eventTrackIfChanged,
                                                          speedTrackLinkState, eventTrackLinkState,
                                                          channelToTransposition,
                                                          determinePlayedLocationLine(), currentSelection, currentBlockSelection);

    patternViewerView->setDisplayedData(displayedData);

    // Useful to take the possible new color ordering in account.
    if (refreshMetadata || forceRefresh) {
        sendDisplayedMetadataToView();
    }

    // Now we have new data, updates the information view.
    updateInformationViewAndEffectContext();
}

SpecialTrack* PatternViewerControllerImpl::getSongDataAndUpdateViewInternalForSpecialTrack(SpecialTrack& localSpecialTrack, bool isSpeedTrack,
                                                                                           const bool forceRefresh) noexcept
{
    SpecialTrack* specialTrackReturnedIfChanged = nullptr;      // By default, no change.

    const SpecialTrack* specialTrackInSong = nullptr;
    songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const auto& subsong) {
        specialTrackInSong = &subsong.getSpecialTrackRefFromPosition(shownLocation.getPosition(), isSpeedTrack);
    });
    /*if (specialTrackInSong == nullptr) {                // FIX ME Dirty hack?
        jassertfalse;       // Unable to reach the Subsong!
        return nullptr;
    }*/

    if (forceRefresh || !localSpecialTrack.isSameModificationCounter(*specialTrackInSong)) {
        // Uses the Track from the Song, as it is not the same.
        localSpecialTrack = *specialTrackInSong;                     // Makes a copy.
        specialTrackReturnedIfChanged = &localSpecialTrack;          // Returns the local Special Track, it has changed.
    }

    return specialTrackReturnedIfChanged;           // If warning here, it is a nonsense.
}

void PatternViewerControllerImpl::setNewTrackHeightAndUpdate(int desiredHeight) const noexcept
{
    // Corrects the height first.
    desiredHeight = CheckTrackHeight::correctHeight(desiredHeight);

    // Modifies the song.
    songController.setPositionHeight(shownLocation, desiredHeight);
}

Position PatternViewerControllerImpl::getPositionData() const noexcept
{
    auto positionData = Position::buildEmptyInstance();
    songController.performOnConstSubsong(shownLocation.getSubsongId(), [&] (const Subsong& subsong) {
        positionData = subsong.getPositionRef(shownLocation.getPosition());
    });
    return positionData;
}

OptionalInt PatternViewerControllerImpl::determinePlayedLocationLine() const noexcept
{
    auto playedLocationLine = OptionalInt();

    // Is the played Location worth being transmitted? If not in the same subsong/position, no need. Also, if following, there is no need to show it.
    if (isFollowing() || playedLocation.isAbsent()) {
        return playedLocationLine;
    }

    const auto playedLocationValue = playedLocation.getValue();
    if ((playedLocationValue.getSubsongId() == shownLocation.getSubsongId()) && (playedLocationValue.getPosition() == shownLocation.getPosition())) {
        playedLocationLine = playedLocationValue.getLine();
    }

    return playedLocationLine;
}

bool PatternViewerControllerImpl::isFollowing() const noexcept
{
    switch (follow) {
        case Follow::followOff:
            return false;
        case Follow::followIfRecordOff:
            return !recording;
        default:
            jassertfalse;
        case Follow::followOn:
            return true;
    }
}

bool PatternViewerControllerImpl::isPlaying() const noexcept
{
    return playerController.isPlaying();
}

void PatternViewerControllerImpl::storeShownLocationWithHeightCorrection(const Location& locationToCorrect) noexcept
{
    shownLocation = correctLocationFromShownLocation(locationToCorrect);
}

Location PatternViewerControllerImpl::correctLocationFromShownLocation(const Location& locationToCorrect) const noexcept
{
    auto newLocation = locationToCorrect;       // Set just in case the subsong doesn't exist.
    songController.performOnConstSubsong(locationToCorrect.getSubsongId(), [&](const Subsong& subsong) {
        const auto& position = subsong.getPositionRef(locationToCorrect.getPosition());

        const auto newLine = NumberUtil::correctNumber(newLocation.getLine(), 0, position.getHeight() - 1);
        newLocation = newLocation.withLine(newLine);
    });

    return newLocation;
}

void PatternViewerControllerImpl::setDefaultEffectAndRefreshUi(const Effect effect) noexcept
{
    defaultEffect = effect;
    patternViewerView->setDefaultEffect(defaultEffect);
}


// ExpressionChangeObserver method implementations.
// ==================================================

void PatternViewerControllerImpl::onExpressionChanged(const Id& /*expressionId*/, const unsigned int /*whatChanged*/)
{
    // We don't care about that.
}

void PatternViewerControllerImpl::onExpressionCellChanged(const Id& /*expressionId*/, int /*cellIndex*/, bool /*mustAlsoRefreshPastIndex*/)
{
    // We don't care about that.
}

void PatternViewerControllerImpl::onExpressionsInvalidated()
{
    refreshStoredSongDataAndUpdateView(true);        // The expression indexes may have changed. Error may be present (arp/pitch present or not).
}


// InstrumentChangeObserver method implementations.
// ==================================================

void PatternViewerControllerImpl::onInstrumentChanged(const Id& /*instrumentId*/, const unsigned int whatChanged)
{
    // If color changed, we need to refresh the metadata.
    if (NumberUtil::isBitPresent(whatChanged, InstrumentChangeObserver::What::color)) {
        sendDisplayedMetadataToView();
    }
}

void PatternViewerControllerImpl::onInstrumentsInvalidated()
{
    refreshStoredSongDataAndUpdateView(false, true); // The instrument indexes may have changed, so the colors too.
}

void PatternViewerControllerImpl::onPsgInstrumentCellChanged(const Id& /*instrumentId*/, int /*cellIndex*/, bool /*mustRefreshAllAfterToo*/)
{
    // We don't care about internal data being changed.
}


// Timer method implementations.
// ==============================================

void PatternViewerControllerImpl::timerCallback()
{
    // Builds a map to send to the View.
    // Each raw PSG registers must be converted to data the View is interested in.
    std::unordered_map<int, PeriodAndNoiseMeterInput> metersInput;

    for (const auto& entry : psgIndexToPendingRegisters) {
        const auto psgIndex = entry.first;
        const auto& psgRegisters = entry.second;

        for (auto channelIndexInPsg = 0; channelIndexInPsg < PsgValues::channelCountPerPsg; ++channelIndexInPsg) {
            // By default, nothing.
            auto softwarePeriodAndVolumeOptional = OptionalValue<std::pair<int, int>>();
            auto hardwarePeriodOptional = OptionalInt();
            auto noiseAndVolumeOptional = OptionalValue<std::pair<int, int>>();

            // Any software period?
            if (psgRegisters.getMixerSoundState(channelIndexInPsg)) {
                softwarePeriodAndVolumeOptional = OptionalValue(std::make_pair(
                        psgRegisters.getSoftwarePeriod(channelIndexInPsg), psgRegisters.getVolume(channelIndexInPsg)));
            }

            // Any hardware period?
            if (psgRegisters.isHardwareVolume(channelIndexInPsg)) {
                hardwarePeriodOptional = OptionalInt(psgRegisters.getHardwarePeriod());
            }

            // Any noise?
            if (psgRegisters.getMixerNoiseState(channelIndexInPsg)) {
                noiseAndVolumeOptional = OptionalValue(std::make_pair(
                        psgRegisters.getNoise(), psgRegisters.getVolume(channelIndexInPsg)));
            }

            const PeriodAndNoiseMeterInput periodAndNoiseMeterInput(softwarePeriodAndVolumeOptional, hardwarePeriodOptional, noiseAndVolumeOptional);

            // Stores the PSG data for this Channel, in the map.
            auto channelIndexInSong = PsgValues::getChannelIndex(channelIndexInPsg, psgIndex);
            metersInput.insert(std::make_pair(channelIndexInSong, periodAndNoiseMeterInput));
        }
    }

    psgIndexToPendingRegisters.clear();     // No more pending. This also takes care of the going from X PSGs to 1.

    patternViewerView->updateMeters(metersInput);
}


// TrackChangeObserver method implementations.
// ==================================================

void PatternViewerControllerImpl::onTrackDataChanged(const Id& subsongId)
{
    // Only interested in modification in the shown subsong.
    if (subsongId != shownLocation.getSubsongId()) {
        return;
    }

    refreshStoredSongDataAndUpdateView(false);
}

void PatternViewerControllerImpl::onTrackMetaDataChanged(const Id& subsongId)
{
    // Only interested in modification in the shown subsong.
    if (subsongId != shownLocation.getSubsongId()) {
        return;
    }

    channelToLinkedState.clear();

    refreshStoredSongDataAndUpdateView(false);
}


// GeneralDataObservers method implementations.
// ==================================================

void PatternViewerControllerImpl::onGeneralDataChanged(const unsigned int whatChanged)
{
    // Only interested in the double-stop.
    if (!NumberUtil::isBitPresent(whatChanged, GeneralDataObserver::What::doubleStopPlay)) {
        return;
    }

    // Dismisses the Block Selection.
    setBlockSelection(true, true, Selection());
}


// ActionManager::Controller method implementations.
// ====================================================

Selection PatternViewerControllerImpl::getCurrentSelection() const noexcept
{
    return currentSelection;
}

CursorLocation PatternViewerControllerImpl::getCursorLocation() const noexcept
{
    return cursorLocation;
}

Location PatternViewerControllerImpl::getShownLocation() const noexcept
{
    return shownLocation;
}

int PatternViewerControllerImpl::getSubsongLength(const Id& subsongId) const noexcept
{
    auto length = 0;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        length = subsong.getLength();
    });

    return length;
}

Cell PatternViewerControllerImpl::getCell(const int channelIndex, const int lineIndex) const noexcept
{
    const auto iterator = channelToTrack.find(channelIndex);
    if (iterator == channelToTrack.cend()) {
        jassertfalse;       // Asking for a non-existent Track?
        return { };
    }
    const auto& track = channelToTrack.at(channelIndex);
    return track.getCell(lineIndex);
}

SpecialCell PatternViewerControllerImpl::getSpecialCell(const bool isSpeedTrack, const int lineIndex) const noexcept
{
    const auto& specialTrack = isSpeedTrack ? speedTrack : eventTrack;
    return specialTrack.getCell(lineIndex);
}

juce::ApplicationCommandManager& PatternViewerControllerImpl::getCommandManager() const noexcept
{
    return mainController.getCommandManager();
}

SongController& PatternViewerControllerImpl::getSongController() const noexcept
{
    return songController;
}

bool PatternViewerControllerImpl::isRecording() const noexcept
{
    return recording;
}

void PatternViewerControllerImpl::setSelectedInstrument(const int instrumentIndex) noexcept
{
    const auto instrumentId = songController.getInstrumentId(instrumentIndex);
    mainController.setSelectedInstrumentId(instrumentId, false);
}

void PatternViewerControllerImpl::setSelectedEffect(const Effect effect) noexcept
{
    setDefaultEffectAndRefreshUi(effect);
}

void PatternViewerControllerImpl::setSelectedExpression(const bool isArpeggio, const int expressionIndex) noexcept
{
    const auto expressionId = songController.getExpressionId(isArpeggio, expressionIndex);
    mainController.setSelectedExpressionId(isArpeggio, expressionId, false);
}

void PatternViewerControllerImpl::notifyActionFailedBecauseRecordingOff() noexcept
{
    patternViewerView->showWarningBecauseRecordingOff();
}


// ToolboxController::Controller method implementations.
// ==========================================================

SelectedData PatternViewerControllerImpl::getSelectedData()
{
    return actionManager.buildSelectedData();
}

SelectedData PatternViewerControllerImpl::getPatternSelection()
{
    return actionManager.buildSelectedDataAsPattern();
}

SelectedData PatternViewerControllerImpl::getCurrentSubsongSelection()
{
    return actionManager.buildSelectedDataAsCurrentSubsong();
}

std::vector<SelectedData> PatternViewerControllerImpl::getAllSubsongsSelection()
{
    return actionManager.buildSelectedDataWithAllSubsongs();
}


// ====================================================

void PatternViewerControllerImpl::notifyBlockSelectionChange() const noexcept
{
    mainController.observers().getBlockSelectionObservers().applyOnObservers([&](SelectedBlockObserver* provider) {
        // If the selection is empty, uses the shown location line. It is considered empty, but the start value remains, which is very handy
        // because we can make the difference between a Block selection, and a cursor line!
        const auto range = currentBlockSelection.isEmpty()
                ? juce::Range(shownLocation.getLine(), shownLocation.getLine())
                : juce::Range(currentBlockSelection.getTopLine(), currentBlockSelection.getBottomLine() + 1);  // +1 because Range end is exclusive.

        provider->onNewSelectedBlock(range);
    });
}

void PatternViewerControllerImpl::setBlockSelection(const bool refreshUi, const bool notifyChange, const Selection& newSelection) noexcept
{
    currentBlockSelection = newSelection;
    if (refreshUi) {
        refreshStoredSongDataAndUpdateView(false);
    }
    if (notifyChange) {
        notifyBlockSelectionChange();
    }
}

Selection PatternViewerControllerImpl::correctBlock(const Selection& newBlock) const noexcept
{
    // Simple: removes the Block if one of the bound in invalid.
    return newBlock.isValid(height) ? newBlock : Selection();
}

void PatternViewerControllerImpl::onUserWantsToCapture()
{
    actionManager.onUserWantsToCapture();
}

void PatternViewerControllerImpl::onUserWantsToWriteRst()
{
    cursorManager.manageNoteReceived(CellConstants::rstNote, CellConstants::rstInstrument);
}

const EffectContext* PatternViewerControllerImpl::getEffectContext() const noexcept
{
    return mainController.getPlayerController().getEffectContext();
}

CursorLocation PatternViewerControllerImpl::correctCursorLocation(const CursorLocation& inputCursorLocation) const noexcept
{
    const auto locations = cursorManager.buildCursorLocations();
    const auto index = CollectionUtil::findIndexOfItem<CursorLocation>(locations, inputCursorLocation);
    return (index < 0) ? CursorLocation() : inputCursorLocation;
}

}   // namespace arkostracker
