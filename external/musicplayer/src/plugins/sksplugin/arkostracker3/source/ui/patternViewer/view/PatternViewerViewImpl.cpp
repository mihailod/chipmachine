#include "PatternViewerViewImpl.h"

#include <BinaryData.h>

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../song/subsong/Position.h"
#include "../../../utils/CollectionUtil.h"
#include "../../components/dialogs/TextFieldDialog.h"
#include "../../keyboard/CommandIds.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/KeyboardFocusTraverserCustom.h"
#include "../controller/PatternViewerController.h"
#include "dialog/LinkDialog.h"
#include "dialog/SpecialLinkDialog.h"
#include "dialog/TrackHeaderDialog.h"
#include "tracks/lineTrack/LineTrackViewImpl.h"
#include "tracks/track/TrackViewImpl.h"

namespace arkostracker 
{

const int PatternViewerViewImpl::renameDialogWidth = 300;
const int PatternViewerViewImpl::spaceBetweenTracks = 6;
const int PatternViewerViewImpl::offsetEffectsComboBoxId = 1;

PatternViewerViewImpl::PatternViewerViewImpl(PatternViewerController& pPatternViewerController, DisplayedMetadata pDisplayedMetadata, const int pInitialStep,
                                             const bool pWriteInstrument, const bool pRecordState, const Follow pFollow) noexcept :
        patternViewerCommands(pPatternViewerController),
        virtualKeyboardCommands(PreferencesManager::getInstance().getVirtualKeyboardLayout()),
        patternViewerController(pPatternViewerController),
        displayedMetadata(std::move(pDisplayedMetadata)),
        font(std::make_unique<juce::Font>(juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMonoR_ttf,
                                                                                  static_cast<size_t>(BinaryData::UbuntuMonoR_ttfSize)))),
        doubleFont(std::make_unique<juce::Font>(juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMonoB_ttf,
                                                                                        static_cast<size_t>(BinaryData::UbuntuMonoB_ttfSize)))),
        recordButton(BinaryData::IconRecordOff_png, static_cast<size_t>(BinaryData::IconRecordOff_pngSize),
                     BinaryData::IconRecordOn_png, static_cast<size_t>(BinaryData::IconRecordOn_pngSize),
                     juce::translate("Record"), [this](int, bool, bool) { onRecordButtonClicked(); }, 0,
                     true, false),   // Record ON must not have its color changed!
        followButton([&](int) { onFollowButtonClicked(); }),
        stepSlider(*this, static_cast<const void*>(BinaryData::IconStep_png),
                   static_cast<size_t>(BinaryData::IconStep_pngSize)),        // Cast to avoid ambiguity.
        writeInstrumentButton(BinaryData::IconWriteInstrumentOff_png, static_cast<size_t>(BinaryData::IconWriteInstrumentOff_pngSize),
                              BinaryData::IconWriteInstrumentOn_png, static_cast<size_t>(BinaryData::IconWriteInstrumentOn_pngSize),
                              juce::translate("When disabled, the instrument is not written when you write a note"),
                              [this](int, bool, bool) { onWriteInstrumentButtonClicked(); }),
        minimizeButton(BinaryData::IconCollapsed1234_png, static_cast<size_t>(BinaryData::IconCollapsed1234_pngSize), juce::translate("Minimize"),
                       [this](int, bool, bool) { onMinimizeButtonClicked(); }),
        effectsComboBox(),
        lockEffectButton(BinaryData::IconWriteNoOverwrite_png, static_cast<size_t>(BinaryData::IconWriteNoOverwrite_pngSize),
                         BinaryData::IconWriteOverwrite_png, static_cast<size_t>(BinaryData::IconWriteOverwrite_pngSize),
                         std::vector {
                             juce::translate("The selected effect number is written if there is no effect already under the cursor."),
                             juce::translate("The selected effect number is written, possibly overwriting the effect already under the cursor."),
                         },
                         [this](int, bool, bool) { onLockEffectButtonClicked(); }),
        toolboxButton(BinaryData::IconToolboxOff_png, static_cast<size_t>(BinaryData::IconToolboxOff_pngSize),
                        BinaryData::IconToolboxOn_png, static_cast<size_t>(BinaryData::IconToolboxOn_pngSize),
                        juce::translate("Opens or close a toolbox."),
                        [this](int, bool, bool) { onToolboxButtonClicked(); }),
        viewport(*this),
        holderInViewport(),
        trackViews(),           // Set below.
        lineTrackView(),        // Set below.
        speedTrackView(),       // Set below.
        eventTrackView(),       // Set below.
        verticalScrollBar(true),
        storedTrackHeight(0),
        storedChannelToTransposition(),
        isToolboxOpened(false),
        modalDialog(),
        setHeightDialogListener(*this),
        setTranspositionDialogListener(*this),
        cachedEffectToChar()
{
    setFontNewHeight(displayedMetadata.getFontSize());

    recordButton.setState(pRecordState);

    // Sets up the "follow" icon and state.
    const auto imageColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId);
    followButton.addImage(std::make_unique<ColoredImage>(
                                  BinaryData::IconFollowOffIfRecord_png, static_cast<size_t>(BinaryData::IconFollowOffIfRecord_pngSize), imageColor),
                          juce::translate("Follow the playing if Record is off"));
    followButton.addImage(std::make_unique<ColoredImage>(
                                  BinaryData::IconFollowOn_png, static_cast<size_t>(BinaryData::IconFollowOn_pngSize), imageColor),
                          juce::translate("Follow the playing"));
    followButton.addImage(std::make_unique<ColoredImage>(
                                  BinaryData::IconFollowOff_png, static_cast<size_t>(BinaryData::IconFollowOff_pngSize), imageColor),
                          juce::translate("Don't follow the playing"));
    PatternViewerViewImpl::setFollowMode(pFollow);

    // Sets up the Viewport and the unique Component inside, which will hold the Tracks.
    viewport.setScrollBarsShown(false, true, false, true);
    viewport.setViewedComponent(&holderInViewport, false);
    verticalScrollBar.setAutoHide(false);
    verticalScrollBar.setSingleStepSize(1.0);
    verticalScrollBar.addListener(this);
    createOrDeleteTrackViewsIfNeeded(displayedMetadata.getChannelCount());

    effectsComboBox.onChange = [this] { onDefaultEffectChanged(); };
    effectsComboBox.setWantsKeyboardFocus(false);        // Else, the PV loses focus when the ComboBox is used.

    stepSlider.setShownValue(static_cast<double>(pInitialStep));
    writeInstrumentButton.setState(pWriteInstrument);

    addAndMakeVisible(recordButton);
    addAndMakeVisible(followButton);
    addAndMakeVisible(stepSlider);
    addAndMakeVisible(minimizeButton);
    addAndMakeVisible(effectsComboBox);
    addAndMakeVisible(lockEffectButton);
    addAndMakeVisible(toolboxButton);
    addAndMakeVisible(informationView);
    addAndMakeVisible(writeInstrumentButton);
    addAndMakeVisible(viewport);
    addAndMakeVisible(verticalScrollBar);

    fillEffectComboBox();
}

void PatternViewerViewImpl::resized()
{
    const auto width = getWidth();

    const auto margins = LookAndFeelConstants::margins;
    const auto largeMargins = margins * 3;
    constexpr auto smallMargins = 2;
    constexpr auto iconsY = 5;
    const auto iconsX = margins;
    const auto iconsHeight = LookAndFeelConstants::buttonsHeight;
    const auto buttonImagesWidth = LookAndFeelConstants::buttonImagesWidth;
    const auto toolboxButtonWidth = buttonImagesWidth;

    // The icons at the top.
    recordButton.setBounds(iconsX, iconsY, buttonImagesWidth, iconsHeight);
    followButton.setBounds(recordButton.getRight() + margins, iconsY, buttonImagesWidth, iconsHeight);
    stepSlider.setTopLeftPosition(followButton.getRight() + largeMargins, iconsY);
    writeInstrumentButton.setBounds(stepSlider.getRight() + largeMargins, iconsY, buttonImagesWidth, iconsHeight);
    minimizeButton.setBounds(writeInstrumentButton.getRight() + smallMargins, iconsY, buttonImagesWidth, iconsHeight);
    effectsComboBox.setBounds(minimizeButton.getRight() + largeMargins, iconsY, 160, iconsHeight);
    lockEffectButton.setBounds(effectsComboBox.getRight() + smallMargins, iconsY, buttonImagesWidth, iconsHeight);

    constexpr auto minimumInformationViewWidth = 80;
    constexpr auto widthBetweenInformationViewAndToolboxButton = smallMargins;
    const auto informationViewX = lockEffectButton.getRight() + largeMargins;
    const auto informationViewWidth = std::max(minimumInformationViewWidth, width - informationViewX - widthBetweenInformationViewAndToolboxButton - toolboxButtonWidth - margins);
    informationView.setBounds(informationViewX, iconsY, informationViewWidth, iconsHeight);
    toolboxButton.setBounds(informationView.getRight() + widthBetweenInformationViewAndToolboxButton, iconsY, toolboxButtonWidth, iconsHeight);
    patternViewerController.locateToolbox(toolboxButton.getX(), toolboxButton.getY());

    relocateAndResizeAllTrackViews();

    // A bit hackish, but couldn't find a way to focus on the bars on init.
    if (isShowing() || isOnDesktop()) {     // To prevent JUCE assertion.
        viewport.grabKeyboardFocus();
    }
}

void PatternViewerViewImpl::relocateAndResizeAllTrackViews()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto margins = LookAndFeelConstants::margins;

    const auto patternViewerX = margins;
    const auto patternViewerY = recordButton.getBottom() + 6;
    const auto scrollBarSize = juce::LookAndFeel::getDefaultLookAndFeel().getDefaultScrollbarWidth();
    const auto scrollBarX = width - scrollBarSize;
    const auto patternViewerWidth = width - scrollBarSize - 1;
    const auto patternViewerHeight = height - patternViewerY;

    // Note that a Font Size change will provoke a refresh of the views because the bounds change, as a side-effect.

    // The Line Track on the left.
    const auto lineTrackWidth = lineTrackView->calculateTrackWidth(false);            // Never minimized.
    lineTrackView->setBounds(patternViewerX, patternViewerY, lineTrackWidth, patternViewerHeight);

    // The Viewport with all the Tracks, on its right.
    const auto viewPortX = lineTrackView->getRight() + spaceBetweenTracks;
    const auto viewPortWidth = patternViewerWidth - viewPortX;
    viewport.setBounds(viewPortX, patternViewerY, viewPortWidth, patternViewerHeight);
    verticalScrollBar.setBounds(scrollBarX, patternViewerY, scrollBarSize, patternViewerHeight - scrollBarSize);    // Height a bit less, looks better because of the bottom bar.

    // Finally, relocates the Tracks inside the Viewport.
    relocateAndResizeAllTrackViewsInViewport();
}

void PatternViewerViewImpl::lookAndFeelChanged()
{
    applyMetadataToTracks();
}

std::unique_ptr<juce::ComponentTraverser> PatternViewerViewImpl::createKeyboardFocusTraverser()
{
    return std::make_unique<KeyboardFocusTraverserCustom>(viewport);      // Make sure the PV itself always get the focus.
}

void PatternViewerViewImpl::applyMetadataToTracks() const noexcept
{
    // Updates the trackViewMetadata. As it is shared with the TrackView, no need to pass it.
    const auto trackViewMetadata = buildInternalTrackViewMetadata();

    for (auto* track : getAbstractTrackViews()) {
        track->onLookAndFeelChanged(trackViewMetadata);
    }
}

std::shared_ptr<TrackViewMetadata> PatternViewerViewImpl::buildInternalTrackViewMetadata() const noexcept
{
    // These come from the Look'n'feel.
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    auto volumeEffectColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerVolumeEffect));
    auto arpeggioEffectColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerArpeggioEffect));
    auto pitchEffectColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerPitchEffect));
    auto resetEffectColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerResetEffect));
    auto instrumentSpeedEffectColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerInstrumentSpeedEffect));

    const std::unordered_map<Effect, juce::Colour> effectToColor = {
            { Effect::volume, volumeEffectColor },
            { Effect::volumeIn, volumeEffectColor },
            { Effect::volumeOut, volumeEffectColor },
            { Effect::arpeggioTable, arpeggioEffectColor },
            { Effect::arpeggio3Notes, arpeggioEffectColor },
            { Effect::arpeggio4Notes, arpeggioEffectColor },
            { Effect::forceArpeggioSpeed, arpeggioEffectColor },
            { Effect::pitchUp, pitchEffectColor },
            { Effect::pitchDown, pitchEffectColor },
            { Effect::fastPitchUp, pitchEffectColor },
            { Effect::fastPitchDown, pitchEffectColor },
            { Effect::pitchGlide, pitchEffectColor },
            { Effect::pitchTable, pitchEffectColor },
            { Effect::forcePitchTableSpeed, pitchEffectColor },
            { Effect::reset, resetEffectColor },
            { Effect::forceInstrumentSpeed, instrumentSpeedEffectColor },
    };
    jassert(effectToColor.size() == static_cast<size_t>(Effect::lastEffect2_0) - 1U);         // Missing an effect above?

    return std::make_shared<TrackViewMetadata>(displayedMetadata.getHighlightStep(), displayedMetadata.getSecondaryHighlight(),
                                               displayedMetadata.mustZoomMiddleLine(),
                                               displayedMetadata.getEffectToChar(), effectToColor, displayedMetadata.getInstrumentToColorArgb(),
                                               displayedMetadata.getArpeggioCount(), displayedMetadata.getPitchCount()
    );
}


// TrackView::Listener method implementations.
// =============================================
void PatternViewerViewImpl::onUserLeftClickedOnChannelNumber(const int channelIndex)
{
    patternViewerController.onUserLeftClickedOnChannelNumber(channelIndex);
}

void PatternViewerViewImpl::onUserRightClickedOnChannelNumber(const int channelIndex)
{
    patternViewerController.onUserRightClickedOnChannelNumber(channelIndex);
}

void PatternViewerViewImpl::onUserWantsToScrollVerticallyWithMouseWheelOnTrack(const bool scrollToBottom)
{
    patternViewerController.onUserWantsToScrollVerticallyWithMouseWheel(scrollToBottom);
}

void PatternViewerViewImpl::onUserClickedOnRankOfNormalTrack(const int channelIndex, const int rankFromCenterAsOrigin, const CellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    patternViewerController.onUserClickedOnRankOfNormalTrack(channelIndex, rankFromCenterAsOrigin, cursorRank, leftButton, shift);
}

void PatternViewerViewImpl::onUserClickIsUpFromNormalTrack(const bool leftButton)
{
    patternViewerController.onUserClickIsUpFromNormalTrack(leftButton);
}

void PatternViewerViewImpl::onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event)
{
    patternViewerController.onUserDraggedCursorFromNormalTrack(event);
}


// SpecialTrackView::Listener method implementations.
// =====================================================

void PatternViewerViewImpl::onUserClickedOnRankOfSpecialTrack(const bool isSpeedTrack, const int rankFromCenterAsOrigin, const SpecialCellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    patternViewerController.onUserClickedOnRankOfSpecialTrack(isSpeedTrack, rankFromCenterAsOrigin,  cursorRank, leftButton, shift);
}

void PatternViewerViewImpl::onUserClickIsUpFromSpecialTrack(const bool leftButton)
{
    patternViewerController.onUserClickIsUpFromSpecialTrack(leftButton);
}

void PatternViewerViewImpl::onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event)
{
    patternViewerController.onUserDraggedCursorFromSpecialTrack(event);
}

void PatternViewerViewImpl::onUserWantsToNameSpecialTrack(const bool isSpeedTrack, const juce::String& currentName)
{
    onUserWantsToSetSpecialTrackName(isSpeedTrack, currentName);
}

void PatternViewerViewImpl::onUserWantsToManageSpecialLink(const bool isSpeedTrack)
{
    promptToSetSpecialLink(isSpeedTrack);
}


// PatternViewerViewImpl method implementations.
// ==============================================

void PatternViewerViewImpl::setDisplayedMetadata(const DisplayedMetadata& newDisplayedMetadata,
                                                 const DisplayedData& displayedData) noexcept
{
    // Any difference?
    if (displayedMetadata == newDisplayedMetadata) {
        return;
    }
    displayedMetadata = newDisplayedMetadata;

    // The font height and "must zoom middle line" may have changed.
    const auto fontHeight = displayedMetadata.getFontSize();
    setFontNewHeight(fontHeight);

    // Maybe the track count has changed!
    createOrDeleteTrackViewsIfNeeded(displayedMetadata.getChannelCount());

    // Performs a "set displayed data" with either non-changed values, or stored one.
    setDisplayedData(displayedData);

    // Makes sure the metadata are applied. This is useful when switching Subsong.
    applyMetadataToTracks();

    // Resizes the Tracks according to the minimized state and the Hex line number.
    relocateAndResizeAllTrackViews();

    // The effect names may have changed.
    fillEffectComboBox();
}

void PatternViewerViewImpl::relocateAndResizeAllTrackViewsInViewport() noexcept
{
    const auto trackHeight = viewport.getHeight();
    auto x = 0;
    constexpr auto separator = spaceBetweenTracks;

    auto channelIndex = 0;
    auto isFirst = true;
    for (auto* trackView : getAbstractTrackViews()) {
        // The first Track MUST be the LineTrack, which is OUTSIDE the Viewport, skips it, it has already been treated.
        if (isFirst) {
            jassert(trackView->getTrackType() == TrackType::line);           // MUST BE a LineTrack!!
            isFirst = false;
            continue;
        }

        // A normal or special Track, in the Viewport.
        jassert(trackView->getTrackType() != TrackType::line);           // MUST NOT BE a LineTrack!!

        // New width? Depends on the minimize state and the font size.
        const auto minimized = isTrackMinimized(channelIndex);
        const auto newWidth = changeTrackWidthFromMinimizeState(*trackView, minimized);

        constexpr auto y = 0;
        trackView->setBounds(x, y, newWidth, trackHeight);

        x += newWidth + separator;

        ++channelIndex;
    }

    // Sets the size of the viewport holder.
    const auto holderWidth = (x - separator);     // Removes the last separator.
    const auto holderHeight = trackHeight;
    holderInViewport.setSize(holderWidth, holderHeight);
}

bool PatternViewerViewImpl::isTrackMinimized(const int channelIndex) const noexcept
{
    const auto& minimizedChannelIndexes = displayedMetadata.getMinimizedChannelIndexes();
    return (minimizedChannelIndexes.find(channelIndex) != minimizedChannelIndexes.cend());
}

void PatternViewerViewImpl::setShownLineAndCursorLocation(const int newLine, const OptionalValue<CursorLocation> cursorLocation) noexcept
{
    for (auto* track : getAbstractTrackViews()) {
        track->setShownLineAndCursorPositionAndRefreshIfDifferent(newLine, cursorLocation);
    }
    verticalScrollBar.setCurrentRangeStart(static_cast<double>(newLine), juce::NotificationType::dontSendNotification);

    // Makes sure the track where the cursor is, is visible.
    if (cursorLocation.isPresent()) {
        adjustXScrollingToCursor(cursorLocation.getValueRef());
    }
}

void PatternViewerViewImpl::adjustXScrollingToCursor(const CursorLocation& cursorLocation) noexcept
{
    // Gets the X start/end of the target Track.
    int trackViewXStart;    // NOLINT(*-init-variables)
    int trackViewXEnd;      // NOLINT(*-init-variables)
    switch (cursorLocation.getTrackType()) {
        default:
            [[fallthrough]];
        case TrackType::line:
            // Don't try to do anything.
            return;
        case TrackType::normal:
        {
            const auto channelIndex = cursorLocation.getChannelIndex();
            const auto& trackView = *trackViews.at(static_cast<size_t>(channelIndex));
            trackViewXStart = trackView.getX();
            trackViewXEnd = trackViewXStart + trackView.getWidth();
            break;
        }
        case TrackType::speed:
        {
            trackViewXStart = speedTrackView->getX();
            trackViewXEnd = trackViewXStart + speedTrackView->getWidth();
            break;
        }
        case TrackType::event:
        {
            trackViewXStart = eventTrackView->getX();
            trackViewXEnd = trackViewXStart + eventTrackView->getWidth();
            break;
        }
    }

    // Is it inside the Viewport? If not, scrolls to the beginning of the track X.
    const auto viewPortXStart = viewport.getViewPositionX();
    const auto viewPortXEnd = viewPortXStart + viewport.getWidth();

    if ((trackViewXStart < viewPortXStart) || (trackViewXEnd > viewPortXEnd)) {
        viewport.setViewPosition(trackViewXStart, viewport.getViewPositionY());
    }
}

void PatternViewerViewImpl::setDisplayedData(const DisplayedData& displayedData) noexcept
{
    // (Almost) nothing is stored here. Directly transmits the data to the Tracks.
    const auto line = displayedData.getLine();
    const auto height = displayedData.getHeight();
    storedTrackHeight = height;         // The exceptions are here...
    storedChannelToTransposition = displayedData.getChannelToTransposition();

    // Updates the vertical bar.
    verticalScrollBar.setRangeLimits(0.0, static_cast<double>(height));      // Maximum is never reached, so no need to decrease of 1.
    verticalScrollBar.setCurrentRangeStart(static_cast<double>(line), juce::NotificationType::dontSendNotification);

    const auto playedLocationLine = displayedData.getPlayedLocationLine();

    // Updates the Line Track.
    const auto hexadecimal = displayedMetadata.areShowLineNumbersInHexadecimal();
    lineTrackView->setDisplayedData(LineTrackView::DisplayedData(line, height, hexadecimal, playedLocationLine, displayedData.getLineSelection()));

    const auto& cursorPosition = displayedData.getCursorLocation();
    const auto& selection = displayedData.getSelection();

    // Updates the Tracks.
    auto channelIndex = 0;
    for (const auto& trackView : trackViews) {
        const auto& mutedChannelIndexes = displayedMetadata.getMutedChannelIndexes();
        const auto& channelIndexToLinkState = displayedData.getChannelIndexToLinkState();

        const auto minimized = isTrackMinimized(channelIndex);
        const auto muted = (mutedChannelIndexes.find(channelIndex) != mutedChannelIndexes.cend());
        auto linkState = LinkState::none;
        if (auto it = channelIndexToLinkState.find(channelIndex); it != channelIndexToLinkState.cend()) {
            linkState = it->second;
        } else {
            // Bothersome assertion on loading song, removed.
            // jassertfalse;       // All link state should be present. However, can assert when changing Song, but the real values will be sent afterward.
        }
        const auto transposition = CollectionUtil::findAndGet(displayedData.getChannelToTransposition(), channelIndex, 0);

        // Any Track? If yes, makes a copy.
        const auto* track = displayedData.getChangedTrackFromChannel(channelIndex);

        // Selection for this track?
        const auto selectionForThisTrack = selection.extractForTrack(channelIndex);

        const TrackView::TransmittedDisplayedData trackDisplayedData(line, cursorPosition, height, linkState, muted, minimized, transposition, track,
                                                                     playedLocationLine, selectionForThisTrack);
        trackView->setDisplayedDataAndRefresh(trackDisplayedData);

        ++channelIndex;
    }

    // Updates the Special Tracks. Present only if changed.
    const auto selectionOnSpeedTrack = selection.extractForSpecialTrack(true);
    const auto selectionOnEventTrack = selection.extractForSpecialTrack(false);
    speedTrackView->setDisplayedDataAndRefreshIfNeeded(SpecialTrackView::TransmittedDisplayedData(line, height, displayedData.getSpeedTrack(),
                                                                                                  playedLocationLine, selectionOnSpeedTrack,
                                                                                                  displayedData.getSpeedTrackLinkState()));
    eventTrackView->setDisplayedDataAndRefreshIfNeeded(SpecialTrackView::TransmittedDisplayedData(line, height, displayedData.getEventTrack(),
                                                                                                  playedLocationLine, selectionOnEventTrack,
                                                                                                  displayedData.getEventTrackLinkState()));
}

int PatternViewerViewImpl::getViewportX() const noexcept
{
    return viewport.getViewPositionX();
}

void PatternViewerViewImpl::setViewportX(const int x) noexcept
{
    viewport.setViewPosition(x, viewport.getViewPositionY());
}

void PatternViewerViewImpl::updateMeters(std::unordered_map<int, PeriodAndNoiseMeterInput> channelIndexToPeriodAndNoiseMeterInput) noexcept
{
    // The input map is not browsed: the tracks are, and data is picked if present. If not present, simply send empty data (this will decrease the bars).
    // Thus, out-of-bounds data is ignored.
    auto channelIndex = 0;
    for (const auto& trackView : trackViews) {
        // Any data from the input map for this Track?
        const auto iterator = channelIndexToPeriodAndNoiseMeterInput.find(channelIndex);
        if (iterator != channelIndexToPeriodAndNoiseMeterInput.cend()) {
            // There is data.
            trackView->updateMeterAndRefresh(iterator->second);
        } else {
            // No data.
            static const PeriodAndNoiseMeterInput emptyInput;
            trackView->updateMeterAndRefresh(emptyInput);
        }

        ++channelIndex;
    }
}


// LineTrackView::Listener method implementations.
// ===================================================

void PatternViewerViewImpl::onUserWantsToScrollVerticallyWithMouseWheelOnLineTrack(const bool scrollToBottom)
{
    // Note: only the Line Track manages its vertical mouse wheel, the other tracks uses the viewport mouse wheel handling to allow
    // Alt+wheel to move horizontally. Shouldn't be problem if the line Track doesn't handle it...
    patternViewerController.onUserWantsToScrollVerticallyWithMouseWheel(scrollToBottom);
}

void PatternViewerViewImpl::onUserClickedOnTrackHeight()
{
    // We manage the dialog here, without notifying the Controller. It would simply add more complexity.
    jassert(modalDialog == nullptr);            // Dialog already present?
    const auto minimumHeight = Position::minimumPositionHeight;
    const auto maximumHeight = Position::maximumPositionHeight;
    const auto showInHex = displayedMetadata.areShowLineNumbersInHexadecimal();
    const auto topText = juce::translate("From "
        + (showInHex ? juce::String::toHexString(minimumHeight) : juce::String(minimumHeight))
        + juce::translate(" to ")
        + (showInHex ? juce::String::toHexString(maximumHeight) : juce::String(maximumHeight))
        + (showInHex ? " (hex)." : " (decimal)."));
    modalDialog = std::make_unique<SetValueDialog>(setHeightDialogListener, storedTrackHeight, showInHex,
                                                   minimumHeight, maximumHeight,
                                                   juce::translate("Set height"), juce::translate("Height"),
                                                   topText);
}

void PatternViewerViewImpl::onUserWantsToModifyTrackHeightWithOffset(const int offset)
{
    patternViewerController.onUserWantsToModifyTrackHeightWithOffset(offset);
}

void PatternViewerViewImpl::onUserClickedOnLineTrack(const int rankFromCenterAsOrigin, const bool leftButton)
{
    patternViewerController.onUserClickedOnRankOfLineTrack(rankFromCenterAsOrigin, leftButton);
}

void PatternViewerViewImpl::onUserClickIsUpFromLineTrack(const bool leftButton)
{
    patternViewerController.onUserClickIsUpFromLineTrack(leftButton);
}

void PatternViewerViewImpl::onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event)
{
    patternViewerController.onUserDraggedCursorFromLineTrack(event);
}


// =====================================================

int PatternViewerViewImpl::changeTrackWidthFromMinimizeState(AbstractTrackView& trackView, const bool minimized) noexcept
{
    const auto newWidth = trackView.calculateTrackWidth(minimized);
    trackView.setSize(newWidth, trackView.getHeight());

    return newWidth;
}

void PatternViewerViewImpl::createOrDeleteTrackViewsIfNeeded(const int originalExpectedTrackCount) noexcept
{
    const auto expectedTrackCount = static_cast<size_t>(originalExpectedTrackCount);
    // Anything to do?
    const auto currentTrackCount = trackViews.size();
    if ((lineTrackView != nullptr) && (expectedTrackCount == currentTrackCount)) {
        return;         // Already the right number of Tracks.
    }

    auto trackViewMetadata = buildInternalTrackViewMetadata();

    constexpr auto defaultLine = 0;
    constexpr auto defaultHeight = 64;
    const auto defaultCursorLocation = CursorLocation::onTrack(0, CellCursorRank::first);

    // Need to create the LineTrackView?
    if (lineTrackView == nullptr) {
        // Mostly junk data for now.
        auto displayedData = LineTrackView::DisplayedData(defaultLine, defaultHeight, displayedMetadata.areShowLineNumbersInHexadecimal(),
                                                          OptionalInt(), { });

        lineTrackView = std::make_unique<LineTrackViewImpl>(*this, defaultLine, trackViewMetadata, displayedData, *font, *doubleFont);

        addAndMakeVisible(*lineTrackView);          // Not put in the Viewport.
    }

    // Any "music" Track to create or delete?
    if (expectedTrackCount < currentTrackCount) {
        // Too many Tracks. Deletes the overflow.
        trackViews.resize(expectedTrackCount);
    } else if (expectedTrackCount > currentTrackCount) {
        // Need to create more Tracks.
        // Note that they are NOT sized and positioned.
        const auto trackToCreateCount = expectedTrackCount - currentTrackCount;
        for (size_t i = 0U; i < trackToCreateCount; ++i) {
            const auto channelIndex = static_cast<int>(trackViews.size());

            // Junk data for now.
            const TrackView::TransmittedDisplayedData displayedData(defaultLine, defaultCursorLocation, defaultHeight, LinkState::none,
                                                                    false, false,
                                                              0, nullptr, OptionalInt(), Selection());

            auto trackView = std::make_unique<TrackViewImpl>(*this, channelIndex, *font, *doubleFont, trackViewMetadata, displayedData);
            holderInViewport.addAndMakeVisible(*trackView);

            trackViews.emplace_back(std::move(trackView));
        }
    }

    // The Special Tracks, inside the Viewport.
    if (speedTrackView == nullptr) {
        speedTrackView = std::make_unique<SpeedTrackView>(*this, defaultLine, defaultHeight, trackViewMetadata, *font, *doubleFont);

        holderInViewport.addAndMakeVisible(*speedTrackView);
    }
    if (eventTrackView == nullptr) {
        eventTrackView = std::make_unique<EventTrackView>(*this, defaultLine, defaultHeight, trackViewMetadata, *font, *doubleFont);

        holderInViewport.addAndMakeVisible(*eventTrackView);
    }
}

std::vector<AbstractTrackView*> PatternViewerViewImpl::getAbstractTrackViews() const noexcept
{
    std::vector<AbstractTrackView*> tracks;

    // Adds the LineTrack first.
    tracks.push_back(lineTrackView.get());

    // Then adds the "music" Track.
    for (const auto& track : trackViews) {
        tracks.push_back(track.get());
    }
    // Add the special Tracks too.
    tracks.push_back(speedTrackView.get());
    tracks.push_back(eventTrackView.get());

    return tracks;
}

void PatternViewerViewImpl::setFontNewHeight(const int fontHeight) const noexcept
{
    const auto fontHeightFloat = static_cast<float>(fontHeight);
    const auto doubleFontHeight = fontHeightFloat * 1.6F;
    constexpr auto horizontalScale = 1.2F;              // Looks better.

    // Creates the "normal" font.
    font->setHeight(fontHeightFloat);
    font->setHorizontalScale(horizontalScale);
    // Creates the "double height" font.
    doubleFont->setHeight(fontHeightFloat);
    doubleFont->setHorizontalScale(horizontalScale);
    doubleFont->setHeightWithoutChangingWidth(doubleFontHeight);        // "Highlight" font is taller, but not larger.
}

void PatternViewerViewImpl::onMinimizeButtonClicked() const noexcept
{
    patternViewerController.onUserWantsToToggleMinimize();
}

void PatternViewerViewImpl::onRecordButtonClicked() const noexcept
{
    patternViewerController.onUserWantsToToggleRecord();
}

void PatternViewerViewImpl::onFollowButtonClicked() const noexcept
{
    patternViewerController.onUserWantsToToggleFollow();
}

void PatternViewerViewImpl::onWriteInstrumentButtonClicked() const noexcept
{
    patternViewerController.onUserWantsToToggleWriteInstrument();
}

void PatternViewerViewImpl::onLockEffectButtonClicked() const noexcept
{
    patternViewerController.onUserWantsToToggleOverwriteDefaultEffect();
}

void PatternViewerViewImpl::onToolboxButtonClicked() noexcept
{
    isToolboxOpened = !isToolboxOpened;
    toolboxButton.setState(isToolboxOpened);

    // Shows or hides the Toolbox.
    patternViewerController.showOrHideToolbox(isToolboxOpened, *this, toolboxButton.getX(), toolboxButton.getY());
}

void PatternViewerViewImpl::setRecordingState(const bool newRecordState) noexcept
{
    recordButton.setState(newRecordState);
}

void PatternViewerViewImpl::setFollowMode(const Follow newFollowMode) noexcept
{
    int state;              // NOLINT(*-init-variables)
    switch (newFollowMode) {
        case Follow::followIfRecordOff:
            state = 0;
            break;
        case Follow::followOff:
            state = 2;
            break;
        default:
            jassertfalse;
        case Follow::followOn:
            state = 1;
            break;
    }
    followButton.setStateInt(state);
}

void PatternViewerViewImpl::setWriteInstrumentState(const bool writeInstrumentOnWriteNote) noexcept
{
    writeInstrumentButton.setState(writeInstrumentOnWriteNote);
}

void PatternViewerViewImpl::setOverwriteDefaultEffect(const bool locked) noexcept
{
    lockEffectButton.setState(locked);
}

void PatternViewerViewImpl::setDefaultEffect(const Effect newEffect) noexcept
{
    const auto selectedEffectId = static_cast<int>(newEffect) + offsetEffectsComboBoxId;
    effectsComboBox.setSelectedId(selectedEffectId, juce::NotificationType::dontSendNotification);
}

OptionalBool PatternViewerViewImpl::determineOutOfBoundsDirectionFromMouseEvent(const juce::MouseEvent& mouseEvent) noexcept
{
    const auto relativeEvent = mouseEvent.getEventRelativeTo(&viewport);
    const auto relativePositionX = static_cast<int>(relativeEvent.position.getX());
    // Simple case: left.
    if (relativePositionX < 0) {
        return true;
    }
    if (relativePositionX >= viewport.getWidth()) {
        return false;
    }

    // Not out of bounds.
    return { };
}

void PatternViewerViewImpl::setInformationText(const juce::String& text, const bool isError) noexcept
{
    informationView.displayMessage(text, isError);
}

void PatternViewerViewImpl::setEffectContextText(const int channelIndex, const juce::String& text) noexcept
{
    auto trackIndex = 0;
    for (const auto& trackView : trackViews) {
        // Makes sure the other channel texts are removed.
        trackView->setEffectContextText((trackIndex == channelIndex) ? text : juce::String());
        ++trackIndex;
    }
}

void PatternViewerViewImpl::onUserWantsToSetMusicTrackNameAndColor(const int channelIndex, const juce::String& currentName, const OptionalValue<juce::Colour> currentColor) noexcept
{
    onUserWantsToSetTrackNameAndColor(channelIndex, currentName, currentColor);
}

void PatternViewerViewImpl::onUserWantsToSetSpecialTrackName(const bool isSpeedTrack, const juce::String& currentName) noexcept
{
    promptToSetSpecialTrackName(isSpeedTrack, currentName);
}

void PatternViewerViewImpl::onUserWantsToSetLink(const int channelIndex) noexcept
{
    onUserWantsToManageLink(channelIndex);
}

void PatternViewerViewImpl::onUserWantsToSetSpecialLink(const bool isSpeedTrack) noexcept
{
    onUserWantsToManageSpecialLink(isSpeedTrack);
}

void PatternViewerViewImpl::showWarningBecauseRecordingOff() noexcept
{
    recordButton.animateForError();
}


// SetHeightDialogListener method implementations.
// ===================================================

PatternViewerViewImpl::SetHeightDialogListener::SetHeightDialogListener(PatternViewerViewImpl& parent) :
        WithParent(parent)
{
}

void PatternViewerViewImpl::SetHeightDialogListener::onWantToCancelSetValue()
{
    parentObject.modalDialog.reset();
}

void PatternViewerViewImpl::SetHeightDialogListener::onWantToSetValue(const int newHeight)
{
    parentObject.modalDialog.reset();
    parentObject.patternViewerController.onUserWantsToModifyTrackHeight(newHeight);
}


// ApplicationCommandTarget method implementations.
// ==================================================

juce::ApplicationCommandTarget* PatternViewerViewImpl::getNextCommandTarget()
{
    return findFirstTargetParentComponent();        // Command not found here, goes up the UI hierarchy.
}

void PatternViewerViewImpl::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    patternViewerCommands.getAllCommands(commands);
    virtualKeyboardCommands.getAllCommands(commands);
}

void PatternViewerViewImpl::getCommandInfo(const juce::CommandID commandId, juce::ApplicationCommandInfo& result)
{
    patternViewerCommands.getCommandInfo(commandId, result);
    virtualKeyboardCommands.getCommandInfo(commandId, result);
}

bool PatternViewerViewImpl::perform(const InvocationInfo& info)
{
    // First, tests the PV commands (NOT the virtual KB).
    auto success = true;
    switch (info.commandID) {
        case patternViewerMoveDown:
            patternViewerController.onUserWantsToMoveVertically(true, false, false);
            break;
        case patternViewerMoveDownEnlargeSelection:
            patternViewerController.onUserWantsToMoveVertically(true, false, true);
            break;
        case patternViewerMoveUp:
            patternViewerController.onUserWantsToMoveVertically(false, false, false);
            break;
        case patternViewerMoveUpEnlargeSelection:
            patternViewerController.onUserWantsToMoveVertically(false, false, true);
            break;
        case patternViewerMoveFastDown:
            patternViewerController.onUserWantsToMoveVertically(true, true, false);
            break;
        case patternViewerMoveFastDownEnlargeSelection:
            patternViewerController.onUserWantsToMoveVertically(true, true, true);
            break;
        case patternViewerMoveFastUp:
            patternViewerController.onUserWantsToMoveVertically(false, true, false);
            break;
        case patternViewerMoveFastUpEnlargeSelection:
            patternViewerController.onUserWantsToMoveVertically(false, true, true);
            break;
        case patternViewerMoveTop:
            patternViewerController.onUserWantsToMoveVerticallyTo(0, false);
            break;
        case patternViewerMoveTopEnlargeSelection:
            patternViewerController.onUserWantsToMoveVerticallyTo(0, true);
            break;
        case patternViewerMoveBottom:
            patternViewerController.onUserWantsToMoveVerticallyTo(127, false);     // Will be corrected, we don't care about the real value. Simpler...
            break;
        case patternViewerMoveBottomEnlargeSelection:
            patternViewerController.onUserWantsToMoveVerticallyTo(127, true);
            break;

        case patternViewerMoveLeft:
            patternViewerController.onUserWantsToMoveHorizontally(false, false);
            break;
        case patternViewerMoveLeftEnlargeSelection:
            patternViewerController.onUserWantsToMoveHorizontally(false, true);
            break;
        case patternViewerMoveRight:
            patternViewerController.onUserWantsToMoveHorizontally(true, false);
            break;
        case patternViewerMoveRightEnlargeSelection:
            patternViewerController.onUserWantsToMoveHorizontally(true, true);
            break;
        case patternViewerMoveToNextChannel:
            patternViewerController.onUserWantsToMoveToNextChannel(true);
            break;
        case patternViewerMoveToPreviousChannel:
            patternViewerController.onUserWantsToMoveToNextChannel(false);
            break;
        case patternViewerMoveToNextPosition:
            patternViewerController.onUserWantsToMoveToNextPosition(true);
            break;
        case patternViewerMoveToPreviousPosition:
            patternViewerController.onUserWantsToMoveToNextPosition(false);
            break;
        case patternViewerToggleRecord:
            patternViewerController.onUserWantsToToggleRecord();
            break;
        case patternViewerToggleFollow:
            patternViewerController.onUserWantsToToggleFollow();
            break;
        case patternViewerDelete:
            patternViewerController.onUserWantsToDelete(false);
            break;
        case patternViewerDeleteWholeCell:
            patternViewerController.onUserWantsToDelete(true);
            break;
        case patternViewerPlayLine:
            patternViewerController.onUserWantsToPlayLine();
            break;
        case patternViewerSetBlockTogglePlayPatternFromBlock:
            patternViewerController.onUserWantsToTogglePlayPatternFromCursorOrBlock(true);
            break;
        case patternViewerPlayPatternFromStartIgnoreBlock:
            patternViewerController.onUserWantsToTogglePlayPatternFromStart();
            break;

        case patternViewerToggleOverwriteDefaultEffect:
            patternViewerController.onUserWantsToToggleOverwriteDefaultEffect();
            break;
        case patternViewerNextDefaultEffect:
            patternViewerController.onUserWantsToChangeDefaultEffect(true);
            break;
        case patternViewerPreviousDefaultEffect:
            patternViewerController.onUserWantsToChangeDefaultEffect(false);
            break;

        case juce::StandardApplicationCommandIDs::copy:
            patternViewerController.onUserWantsToCopy();
            break;
        case juce::StandardApplicationCommandIDs::cut:
            patternViewerController.onUserWantsToCut();
            break;
        case juce::StandardApplicationCommandIDs::paste:
            patternViewerController.onUserWantsToPaste(false, false);
            break;
        case patternViewerPasteAndMoveBelow:
            patternViewerController.onUserWantsToPaste(true, false);
            break;
        case patternViewerPasteContinuously:
            patternViewerController.onUserWantsToPaste(false, true);
            break;
        case patternViewerPasteMix:
            patternViewerController.onUserWantsToPasteMix();
            break;
        case patternViewerClearSelection:
            patternViewerController.onUserWantsToClearSelection();
            break;
        case patternViewerTransposePlusOne:
            patternViewerController.onUserWantsToTransposeSelection(TransposeRate::upNormal);
            break;
        case patternViewerTransposeMinusOne:
            patternViewerController.onUserWantsToTransposeSelection(TransposeRate::downNormal);
            break;
        case patternViewerTransposePlusOneOctave:
            patternViewerController.onUserWantsToTransposeSelection(TransposeRate::upFast);
            break;
        case patternViewerTransposeMinusOneOctave:
            patternViewerController.onUserWantsToTransposeSelection(TransposeRate::downFast);
            break;
        case patternViewerToggleReadOnly:
            patternViewerController.onUserWantsToToggleReadOnly();
            break;
        case patternViewerGenerateArpeggio:
            patternViewerController.onUserWantsToGenerateArpeggio();
            break;
        case patternViewerGenerateInstrument:
            patternViewerController.onUserWantsToGenerateInstrument();
            break;
        case patternViewerSetTrackNameAndColor:
            patternViewerController.onUserWantsToSetTrackNameAndColor();
            break;
        case patternViewerLinkTrack:
            patternViewerController.onUserWantsToLinkTrack();
            break;

        case patternViewerInsert:
            patternViewerController.onUserWantsToInsert();
            break;
        case patternViewerRemove:
            patternViewerController.onUserWantsToRemove();
            break;

        case patternViewerIncreaseEditStep:
            patternViewerController.onUserWantsToIncreaseEditStep(1);
            break;
        case patternViewerDecreaseEditStep:
            patternViewerController.onUserWantsToIncreaseEditStep(-1);
            break;

        case patternViewerToggleSolo:
            patternViewerController.onUserWantsToToggleSoloTrack();
            break;
        case patternViewerToggleMute:
            patternViewerController.onUserWantsToToggleMuteTrack();
            break;

        case patternViewerSelectTrack:
            patternViewerController.onUserWantsToSelectTrack();
            break;
        case patternViewerSelectAllTracks:
            patternViewerController.onUserWantsToSelectAllTracks();
            break;
        case patternViewerCapture:
            patternViewerController.onUserWantsToCapture();
            break;
        case patternViewerRst:
            patternViewerController.onUserWantsToWriteRst();
            break;

        default:
            success = false;
            break;
    }

    // If command not found, maybe it is a virtual keyboard key?
    if (!success) {
        const auto note = virtualKeyboardCommands.performForNote(info);
        if (note >= 0) {
            success = true;
            patternViewerController.onKeyboardNote(note);
        }
    }

    if (!success) {
        jassertfalse;       // The command isn't known: not normal (oversight?).
    }

    return success;
}

bool PatternViewerViewImpl::keyPressed(const juce::KeyPress& key)
{
    // Asks the controller if this key must be used, which will return true or false.
    return patternViewerController.onKeyPressed(key);
}


// juce::ScrollBar::Listener method implementations.
// ====================================================

void PatternViewerViewImpl::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, const double newRangeStart)
{
    if (scrollBarThatHasMoved != &verticalScrollBar) {
        jassertfalse;
        return;
    }
    const auto newLine = static_cast<int>(newRangeStart);
    patternViewerController.onUserWantsToMoveVerticallyTo(newLine, false);
}


// SliderIncDec::Listener method implementations.
// =================================================

void PatternViewerViewImpl::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    if (&slider == &stepSlider) {
        patternViewerController.onUserWantsToSetEditStep(static_cast<int>(value));
    } else {
        jassertfalse;       // Slider not managed.
    }
}

void PatternViewerViewImpl::setSteps(const int steps) noexcept
{
    stepSlider.setShownValue(static_cast<double>(steps));
}


// =================================================

void PatternViewerViewImpl::fillEffectComboBox() noexcept
{
    // Did the effects to Char changed? If not, don't do anything.
    if (displayedMetadata.getEffectToChar() == cachedEffectToChar) {
        return;
    }
    cachedEffectToChar = displayedMetadata.getEffectToChar();

    effectsComboBox.clear(juce::NotificationType::dontSendNotification);

    const auto& effects = patternViewerController.getDefaultEffects();
    static const std::unordered_map<Effect, juce::String> effectToString = {
        {Effect::reset, juce::translate("reset + inv. volume")},
        {Effect::volume, juce::translate("volume")},
        {Effect::volumeIn, juce::translate("volume up")},
        {Effect::volumeOut, juce::translate("volume down")},
        {Effect::arpeggioTable, juce::translate("arpeggio table")},
        {Effect::pitchTable, juce::translate("pitch table")},
        {Effect::arpeggio3Notes, juce::translate("arpeggio 3 notes")},
        {Effect::arpeggio4Notes, juce::translate("arpeggio 4 notes")},
        {Effect::pitchUp, juce::translate("pitch up")},
        {Effect::pitchDown, juce::translate("pitch down")},
        {Effect::pitchGlide, juce::translate("glide to note")},
        {Effect::fastPitchUp, juce::translate("fast pitch up")},
        {Effect::fastPitchDown, juce::translate("fast pitch down")},
        {Effect::forceInstrumentSpeed, juce::translate("set instr. speed")},
        {Effect::forceArpeggioSpeed, juce::translate("set arpeggio speed")},
        {Effect::forcePitchTableSpeed, juce::translate("set pitch table speed")},
    };
    jassert(effectToString.size() == effects.size());

    const auto& effectToChar = cachedEffectToChar;
    for (const auto& lEffect : effects) {
        const auto& effectText = effectToString.find(lEffect)->second;

        // Finds the effect char to display for this effect.
        juce::juce_wchar effectChar = ' ';      // Fallback.
        if (lEffect != Effect::noEffect) {
            if (auto iterator = effectToChar.find(lEffect); iterator != effectToChar.cend()) {
                effectChar = iterator->second;
            } else {
                jassertfalse;               // Shouldn't happen!
            }
        }

        const auto text = (juce::String::charToString(effectChar) + ": " + effectText);
        effectsComboBox.addItem(text, static_cast<const int>(lEffect) + offsetEffectsComboBoxId);            // 0 is reserved by JUCE.
    }

    // Selects the default effect from the controller.
    const auto selectedEffectId = static_cast<int>(patternViewerController.getDefaultEffect()) + offsetEffectsComboBoxId;
    effectsComboBox.setSelectedId(selectedEffectId, juce::NotificationType::dontSendNotification);
}

void PatternViewerViewImpl::onDefaultEffectChanged() const noexcept
{
    const auto selectedEffect = static_cast<Effect>(effectsComboBox.getSelectedId() - offsetEffectsComboBoxId);
    patternViewerController.onUserSelectedLockedEffect(selectedEffect);
}

void PatternViewerViewImpl::onUserWantsToChangeTransposition(const int channelIndex)
{
    // Shows a panel to set the transposition.
    // We manage the dialog here, without notifying the Controller. It would simply add more complexity.
    jassert(modalDialog == nullptr);            // Dialog already present?
    const auto transposition = CollectionUtil::findAndGet(storedChannelToTransposition, channelIndex, 0);
    const auto minimumTransposition = Position::minimumTransposition;
    const auto maximumTransposition = Position::maximumTransposition;
    setTranspositionDialogListener.setChannelIndex(channelIndex);     // Harmless hack to store the channel on return.
    modalDialog = std::make_unique<SetValueDialog>(setTranspositionDialogListener, transposition, true, minimumTransposition,
                                                   maximumTransposition, juce::translate("Set transposition"),
                                                   juce::translate("Transposition"));
}

void PatternViewerViewImpl::onUserWantsToSetTrackNameAndColor(const int channelIndex, const juce::String& currentName, const OptionalValue<juce::Colour> currentColor)
{
    // Shows a panel to set the name.
    // We manage the dialog here, without notifying the Controller. It would simply add more complexity.
    jassert(modalDialog == nullptr); // Dialog already present?
    modalDialog = std::make_unique<TrackHeaderDialog>(currentName, currentColor,
        [&, channelIndex] (const TrackHeaderDialog::Result& result) {
            onSetTrackNameAndColorValidated(channelIndex, result.newName, result.newColor);
        },
        [&] { closeDialog(); }
        );
}

void PatternViewerViewImpl::promptToSetSpecialTrackName(const bool isSpeedTrack, const juce::String& currentName) noexcept
{
    // Shows a panel to set the name.
    jassert(modalDialog == nullptr); // Dialog already present?
    modalDialog = std::make_unique<TextFieldDialog>(juce::translate("Name the track"), juce::translate("Enter the new name:"),
                                                    currentName,
                                                    [&, isSpeedTrack](const juce::String& newText) { onRenameSpecialTrackNameValidated(isSpeedTrack, newText); },
                                                    [&] { closeDialog(); },
                                                    true, renameDialogWidth
    );
}

void PatternViewerViewImpl::onUserWantsToManageLink(const int channelIndex)
{
    jassert(modalDialog == nullptr);        // Dialog already present? Same remark above.

    const auto positionIndex = patternViewerController.getPositionIndex();
    const auto subsongId = patternViewerController.getSubsongId();
    modalDialog = std::make_unique<LinkDialog>(patternViewerController.getSongController(), subsongId, positionIndex, channelIndex,
        [&, positionIndex, channelIndex] (const int clickedPositionIndex, const int clickedChannelIndex) {
            performLink(positionIndex, channelIndex, clickedPositionIndex, clickedChannelIndex);
        },
        [&] (const int clickedPositionIndex, const int clickedChannelIndex) {
            performGoto(clickedPositionIndex, clickedChannelIndex);
        },
        [&] (const int positionIndexToUnlink, const int clickedChannelIndexToUnlink) {
            performUnlink(positionIndexToUnlink, clickedChannelIndexToUnlink);
        },
        [&] { closeDialog(); }
        );
}

void PatternViewerViewImpl::promptToSetSpecialLink(const bool isSpeedTrack) noexcept
{
    jassert(modalDialog == nullptr);        // Dialog already present? Same remark above.

    const auto positionIndex = patternViewerController.getPositionIndex();
    const auto subsongId = patternViewerController.getSubsongId();
    modalDialog = std::make_unique<SpecialLinkDialog>(patternViewerController.getSongController(), subsongId, positionIndex, isSpeedTrack,
        [&, positionIndex] (const int clickedPositionIndex, const bool clickedIsSpeedTrack) {
            performSpecialLink(clickedIsSpeedTrack, positionIndex, clickedPositionIndex);
        },
        [&] (const int clickedPositionIndex, const bool clickedIsSpeedTrack) {
            performSpecialGoto(clickedIsSpeedTrack, clickedPositionIndex);
        },
        [&] (const int positionIndexToUnlink, const bool clickedIsSpeedTrackToUnlink) {
            performSpecialUnlink(clickedIsSpeedTrackToUnlink, positionIndexToUnlink);
        },
        [&] { closeDialog(); }
        );
}


// ===================================================

void PatternViewerViewImpl::closeDialog() noexcept
{
    modalDialog.reset();
}

void PatternViewerViewImpl::onSetTrackNameAndColorValidated(const int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToSetTrackNameAndColor(channelIndex, newName, newColor);
}

void PatternViewerViewImpl::onRenameSpecialTrackNameValidated(const bool isSpeedTrack, const juce::String& newName) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToRenameSpecialTrack(isSpeedTrack, newName);
}

void PatternViewerViewImpl::performLink(const int sourcePositionIndex, const int sourceChannelIndex, const int targetPositionIndex, const int targetChannelIndex) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToCreateLink(sourcePositionIndex, sourceChannelIndex, targetPositionIndex, targetChannelIndex);
}

void PatternViewerViewImpl::performUnlink(const int positionIndexToUnlink, const int channelIndexToUnlink) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToUnlink(positionIndexToUnlink, channelIndexToUnlink);
}

void PatternViewerViewImpl::performSpecialLink(const bool isSpeedTrack, const int sourcePositionIndex, const int targetPositionIndex) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToCreateSpecialLink(isSpeedTrack, sourcePositionIndex, targetPositionIndex);
}

void PatternViewerViewImpl::performSpecialUnlink(const bool isSpeedTrack, const int positionIndexToUnlink) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToUnlinkSpecial(isSpeedTrack, positionIndexToUnlink);
}

void PatternViewerViewImpl::performGoto(const int positionIndexToGoTo, const int channelIndexToGoTo) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToGoTo(positionIndexToGoTo, channelIndexToGoTo);
}

void PatternViewerViewImpl::performSpecialGoto(const bool isSpeedTrack, const int positionIndexToGoTo) noexcept
{
    closeDialog();
    patternViewerController.onUserWantsToGoToSpecialTrack(isSpeedTrack, positionIndexToGoTo);
}


// SetTranspositionDialogListener::Listener method implementations.
// ===================================================

PatternViewerViewImpl::SetTranspositionDialogListener::SetTranspositionDialogListener(PatternViewerViewImpl& parent) :
        WithParent(parent),
        channelIndex(0)
{
}

void PatternViewerViewImpl::SetTranspositionDialogListener::onWantToCancelSetValue()
{
    parentObject.modalDialog.reset();
}

void PatternViewerViewImpl::SetTranspositionDialogListener::onWantToSetValue(const int newTransposition)
{
    parentObject.modalDialog.reset();
    parentObject.patternViewerController.onUserWantsToModifyTransposition(channelIndex, newTransposition);
}

void PatternViewerViewImpl::SetTranspositionDialogListener::setChannelIndex(const int pChannelIndex)
{
    channelIndex = pChannelIndex;
}

}   // namespace arkostracker

