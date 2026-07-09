#include "TrackViewImpl.h"

#include "../../../../../song/cells/CellConstants.h"
#include "../../../../../utils/CollectionUtil.h"
#include "../../../../../utils/NumberUtil.h"
#include "../../../../lookAndFeel/CustomLookAndFeel.h"
#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const float TrackViewImpl::effectContextFontHeight = 13.0F;
const int TrackViewImpl::effectContextBackgroundHeight = static_cast<int>(effectContextFontHeight) + 4;

TrackViewImpl::TrackViewImpl(TrackView::Listener& pListener, const int pChannelIndex, juce::Font& pNormalFont, juce::Font& pDoubleHeightFont,
                             std::shared_ptr<TrackViewMetadata> pTrackViewMetadata,
                             const TrackView::TransmittedDisplayedData& pTransmittedDisplayedData) noexcept :
        TrackView(pTransmittedDisplayedData.line, std::move(pTrackViewMetadata)),
        listener(pListener),
        channelIndex(pChannelIndex),
        normalFont(pNormalFont),
        doubleHeightFont(pDoubleHeightFont),
        outOfBoundsDisplayedData(),

        trackHeader(*this),

        linkState(pTransmittedDisplayedData.linkState),
        muted(pTransmittedDisplayedData.muted),
        minimized(pTransmittedDisplayedData.minimized),
        displayedTrack(pTransmittedDisplayedData.track == nullptr ? Track() : *pTransmittedDisplayedData.track),
        trackHeight(64),
        playedLocationLine(pTransmittedDisplayedData.playedLocationLine),
        selection(),

        cells(),
        effectContextText(),
        effectContextFont()
{
    addAndMakeVisible(trackHeader);

    // Sets the effect context font once and for all.
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
    effectContextFont = currentLookAndFeel.getBaseFont();
    effectContextFont.setHeight(effectContextFontHeight);
}


// TrackView method implementations.
// ======================================

void TrackViewImpl::setDisplayedDataAndRefresh(const TransmittedDisplayedData& transmittedDisplayedData) noexcept
{
    auto readOnlyChanged = false;

    // Link changed?
    const auto linkChanged = (linkState != transmittedDisplayedData.linkState);
    linkState = transmittedDisplayedData.linkState;

    auto mustRefresh = linkChanged;

    // Current values.
    auto trackName(displayedTrack.getName());
    auto trackColor(displayedTrack.getColor());

    // Is the track different?
    if (transmittedDisplayedData.track != nullptr) {
        readOnlyChanged = (displayedTrack.isReadOnly() != transmittedDisplayedData.track->isReadOnly());

        displayedTrack = *transmittedDisplayedData.track;
        trackName = displayedTrack.getName();
        trackColor = displayedTrack.getColor();
        mustRefresh = true;
    }
    // New height?
    const auto newTrackHeight = transmittedDisplayedData.trackHeight;
    if (trackHeight != newTrackHeight) {
        trackHeight = newTrackHeight;
        mustRefresh = true;
    }
    // New cursor?
    auto newCursorLocation = OptionalValue(transmittedDisplayedData.cursorLocation);        // Wraps into an optional to fit the interface here.
    newCursorLocation = applyCursorLocationOnTrack(newCursorLocation);
    if (currentCursorLocation != newCursorLocation) {
        currentCursorLocation = newCursorLocation;
        mustRefresh = true;
    }
    // New played line?
    const auto newPlayedLocationLine = transmittedDisplayedData.playedLocationLine;
    if (playedLocationLine != newPlayedLocationLine) {
        playedLocationLine = newPlayedLocationLine;
        mustRefresh = true;
    }
    // A new selection?
    const auto& newSelection = transmittedDisplayedData.selection;
    if ((selection != newSelection)) {
        selection = newSelection;
        mustRefresh = true;
    }

    // Is the new line different?
    const auto newLine = NumberUtil::correctNumber(transmittedDisplayedData.line, 0, TrackConstants::lastPossibleIndex);
    mustRefresh = mustRefresh || (shownLine != newLine);

    // Minimized has changed?
    const auto newMuted = transmittedDisplayedData.muted;
    const auto newTransposition = transmittedDisplayedData.transposition;
    const auto newMinimized = transmittedDisplayedData.minimized;
    mustRefresh = mustRefresh || (minimized != newMinimized);

    // The header will take care of knowing what has changed.
    trackHeader.setDisplayedData({ !newMuted, channelIndex + 1, newTransposition, trackName,
        trackColor.isPresent() ? juce::Colour(trackColor.getValue()) : OptionalValue<juce::Colour>(),
        linkState });

    // Must refresh the Cells?
    if (mustRefresh) {
        shownLine = newLine;
        minimized = newMinimized;

        refreshCells();
    }

    // Mute state changed? The mute "veil" must be refreshed. Also in case of read-only/link change.
    if ((muted != newMuted) || linkChanged || readOnlyChanged) {
        muted = newMuted;
        repaint();
    }
}


// CellView::Listener method implementations.
// ============================================

void TrackViewImpl::onUserClickedOnRank(const int viewRank, const CellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    // The controller does not know where is the centered rank. Makes sure the center in 0.
    const auto rankFromCenterAsOrigin = determineRankFromCenterAsOrigin(viewRank);
    listener.onUserClickedOnRankOfNormalTrack(channelIndex, rankFromCenterAsOrigin, cursorRank, leftButton, shift);
}

void TrackViewImpl::onUserClickIsUp(const bool leftButton)
{
    listener.onUserClickIsUpFromNormalTrack(leftButton);
}

void TrackViewImpl::onUserDraggedCursor(const juce::MouseEvent& event)
{
    listener.onUserDraggedCursorFromNormalTrack(event);
}


// =========================================================

void TrackViewImpl::postPaintOverChildren(juce::Graphics& g, const int trackY, const int trackHeightOnScreen) noexcept
{
    const auto width = getWidth();
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());

    // Muted veil.
    if (muted) {
        // The header will take of its own dimming.
        const auto filterColor = currentLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerBackground))
                .withAlpha(0.6F);

        g.setColour(filterColor);
        g.fillRect(0, trackY, width, trackHeightOnScreen);
    } else if (effectContextText.isNotEmpty()) {
        // Displays the effect context, with a small background.
        constexpr auto marginLeft = 4;

        const auto textWidth = static_cast<int>(static_cast<float>(effectContextFont.getStringWidth(effectContextText)) * 1.2F);
        const auto fontHeight = static_cast<int>(effectContextFontHeight);

        const auto textBackgroundHeight = effectContextBackgroundHeight;

        const auto textColor = currentLookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.8F);
        const auto textBackgroundColor = textColor.contrasting().withAlpha(0.5F);

        // A black border behind the text.
        g.setColour(textBackgroundColor);
        g.fillRect(width - textWidth - 1, trackY, textWidth, textBackgroundHeight);

        g.setColour(textColor);
        g.setFont(effectContextFont);
        g.drawSingleLineText(effectContextText, width - marginLeft, trackY + fontHeight, juce::Justification::right);
    }

    // Draws the Lock/Link.
    displayReadOnlyLock(g, displayedTrack.isReadOnly());
}


// =========================================================

void TrackViewImpl::refreshCell(const int cellIndex, const int cellRank, const bool somethingToShow, const bool isMiddleLine, const OptionalBool primaryHighlight,
                                const bool isPlayedLine) noexcept
{
    auto& cellView = cells.at(static_cast<size_t>(cellRank));

    if (!somethingToShow) {
        // Nothing to show.
        cellView->updateDisplayedData(outOfBoundsDisplayedData);
        return;
    }

    // Something to show.
    const auto& cell = displayedTrack.getCell(cellIndex);

    // There is something to show.
    const auto& note = cell.getNote();
    const auto& instrument = cell.getInstrument();

    // Finds the instrument color, if the instrument is valid!
    auto instrumentColor = trackColors->noNoteColor;           // Same color as the no-note.
    if (instrument.isPresent()) {
        instrumentColor = getInstrumentColor(instrument.getValue());
    } else if (note.isPresent()) {
        // The instrument is not present, but there is a note: legato (or glide). We still want to show a color if possible.
        // Goes up till we find an instrument.
        auto browsedCellIndex = cellIndex - 1;
        while (browsedCellIndex >= 0) {
            const auto& browsedCell = displayedTrack.getCell(browsedCellIndex);
            const auto& browsedNote = browsedCell.getNote();
            if (const auto& browsedInstrument = browsedCell.getInstrument(); browsedNote.isPresent() && browsedInstrument.isPresent()) {
                instrumentColor = getInstrumentColor(browsedInstrument.getValue());
                break;
            }

            --browsedCellIndex;
        }
    }

    // The note has the same color as the instrument.
    const auto noteColor = instrumentColor;

    // Is the cursor present? Only present on the "middle line" of course.
    const auto cursorRank = (isMiddleLine && currentCursorLocation.isPresent())
                                ? static_cast<CellCursorRank>(currentCursorLocation.getValue().getRank())
                                : CellCursorRank::notPresent;

    // Selection?
    auto leftSelectionRank = CellCursorRank::notPresent;
    auto rightSelectionRank = CellCursorRank::notPresent;
    if (selection.containsLine(cellIndex)) {
        leftSelectionRank = selection.getLeftRankAsCellCursorRank();
        rightSelectionRank = selection.getRightRankAsCellCursorRank();
    }

    const auto& cellEffects = cell.getEffects();
    const auto indexToEffects = cellEffects.getExistingEffects();
    std::unordered_map<int, CellView::DisplayedEffect> indexToDisplayedEffect;

    const auto isNote = cell.isNote();
    const auto isInstrument = cell.isInstrument();

    // Converts each effect to displayable effect.
    for (const auto& [effectIndex, cellEffect] : indexToEffects) {
        const auto effectEnum = cellEffect.getEffect();

        // What is the letter to display?
        jassert(cellEffect.isPresent());            // Abnormal, we requested only present effects!
        auto effectToCharIterator = trackViewMetadata->effectToChar.find(effectEnum);
        juce::juce_wchar effectLetter;      // NOLINT(*-init-variables)
        if (effectToCharIterator == trackViewMetadata->effectToChar.cend()) {
            jassertfalse;           // Abnormal!
            effectLetter = '*';
        } else {
            effectLetter = effectToCharIterator->second;
        }
        const auto effectValue = cellEffect.getEffectRawValue();
        const auto valueDigitCount = EffectUtil::getDigitCount(effectEnum);

        // What color for the effect?
        auto effectColor = trackColors->errorColor;
        // Maybe the effect is in error! Also checks if the possible effect has invalid values (arp/pitch not existing for example).
        const auto effectErrorOptional = cellEffects.getError(effectIndex, isNote, isInstrument);
        if (effectErrorOptional.isAbsent() && !isErrorInEffectValue(cellEffect)) {
            // No error. Gets the effect color.
            const auto& effectToColor = trackViewMetadata->effectToColor;
            auto effectToColorIterator = effectToColor.find(effectEnum);
            if (effectToColorIterator == effectToColor.cend()) {
                jassertfalse;           // Abnormal!
            } else {
                effectColor = effectToColorIterator->second;
            }
        }

        auto displayedEffect = CellView::DisplayedEffect(effectLetter, effectValue, valueDigitCount, effectColor);
        indexToDisplayedEffect.insert(std::make_pair(effectIndex, displayedEffect));
    }

    const CellView::DisplayedData cellDisplayedData(note, instrument, noteColor, instrumentColor, indexToDisplayedEffect, isMiddleLine, cursorRank,
                                                    leftSelectionRank, rightSelectionRank, minimized, primaryHighlight, isPlayedLine);

    cellView->updateDisplayedData(cellDisplayedData);
}

juce::Colour TrackViewImpl::getInstrumentColor(const int instrumentIndex) const noexcept
{
    const auto instrumentColorIterator = trackViewMetadata->instrumentToColorArgb.find(instrumentIndex);
    if (instrumentColorIterator == trackViewMetadata->instrumentToColorArgb.cend()) {
        return trackColors->errorColor;           // Not found!
    }

    return juce::Colour(instrumentColorIterator->second);
}

bool TrackViewImpl::isErrorInEffectValue(const CellEffect& cellEffect) const noexcept
{
    if (!cellEffect.isPresent()) {
        return false;
    }
    const auto readEffect = cellEffect.getEffect();
    if (readEffect == Effect::arpeggioTable) {
        return cellEffect.getEffectLogicalValue() >= trackViewMetadata->arpeggioExpressionCount;
    }
    if (readEffect == Effect::pitchTable) {
        return cellEffect.getEffectLogicalValue() >= trackViewMetadata->pitchExpressionCount;
    }

    return false;
}

int TrackViewImpl::calculateTrackWidth(const bool minimizedParam) const noexcept
{
    // What is the new width?
    const auto minimizedEffectCount = minimizedParam ? CellConstants::effectCount : 0;       // All or nothing.
    return CellView::calculateWidth(minimizedEffectCount, normalFont);                       // Any font will do, they have the same width.
}

TrackType TrackViewImpl::getTrackType() const noexcept
{
    return TrackType::normal;
}

int TrackViewImpl::getChannelIndex() const noexcept
{
    return channelIndex;
}

void TrackViewImpl::updateMeterAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept
{
    trackHeader.updateMeterAndRefresh(periodAndNoiseMeterInput);
}

void TrackViewImpl::setEffectContextText(const juce::String& text) noexcept
{
    if (effectContextText == text) {
        return;
    }
    effectContextText = text;

    // Strangely enough, needed on Linux but not Mac, else when no text is displayed, it is not removed.
    repaint(0, headerHeight, getWidth(), effectContextBackgroundHeight);
}

void TrackViewImpl::onResizedAccepted() noexcept
{
    const auto width = getWidth();
    constexpr auto trackHeaderHeight = headerHeight;
    trackHeader.setBounds(0, 0, width, trackHeaderHeight);
}

juce::Font& TrackViewImpl::getNormalFont() noexcept
{
    return normalFont;
}

juce::Font& TrackViewImpl::getDoubleHeightFont() noexcept
{
    return doubleHeightFont;
}

bool TrackViewImpl::isMinimized() const noexcept
{
    return minimized;
}

int TrackViewImpl::getTrackHeight() const noexcept
{
    return trackHeight;
}

std::vector<AbstractCellView*> TrackViewImpl::getCellViews() const noexcept
{
    return CollectionUtil::toPointerCollection<CellView, AbstractCellView>(cells);
}

void TrackViewImpl::resizeCellViewCount(const size_t newCount) noexcept
{
    jassert(newCount <= cells.size());
    cells.resize(newCount);
}

int TrackViewImpl::getCellCount() const noexcept
{
    return static_cast<int>(cells.size());
}

AbstractCellView& TrackViewImpl::createAndStorePrototypeCell() noexcept
{
    const auto viewRank = static_cast<int>(cells.size());

    // Creates only empty Cells for now. They are updated at the end.
    const std::unordered_map<int, CellView::DisplayedEffect> indexToEffects;
    const auto emptyDisplayedData = CellView::DisplayedData();

    auto cell = std::make_unique<CellView>(*this, viewRank, normalFont, doubleHeightFont, emptyDisplayedData, trackColors->cellColors);
    cells.push_back(std::move(cell));

    // Returns the last one.
    return cells.rbegin()->operator*();
}

OptionalValue<CursorLocation> TrackViewImpl::applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept
{
    // Not present? Can't do anything then.
    if (cursorLocationToTest.isAbsent()) {
        return cursorLocationToTest;
    }

    const auto cursorLocationValueToTest = cursorLocationToTest.getValue();
    // Must be on a normal Track, and on the right one.
    if ((cursorLocationValueToTest.getTrackType() != TrackType::normal) || cursorLocationValueToTest.getChannelIndex() != channelIndex) {
        return { };
    }

    // A valid rank?
    const auto rank = cursorLocationValueToTest.getRank();
    if ((rank < 0) || (rank >= static_cast<int>(CellCursorRank::count))) {
        return { };
    }

    // Valid!
    return cursorLocationToTest;
}

OptionalInt TrackViewImpl::getPlayedLocationLine() const noexcept
{
    return playedLocationLine;
}


// TrackHeader::Listener method implementations.
// ===============================================

void TrackViewImpl::onUserLeftClickedOnChannelNumber()
{
    listener.onUserLeftClickedOnChannelNumber(channelIndex);
}

void TrackViewImpl::onUserRightClickedOnChannelNumber()
{
    listener.onUserRightClickedOnChannelNumber(channelIndex);
}

void TrackViewImpl::onUserWantsToChangeTransposition()
{
    listener.onUserWantsToChangeTransposition(channelIndex);
}

void TrackViewImpl::onUserWantsToNameTrack(const juce::String& currentName, const OptionalValue<juce::Colour> currentColor)
{
    listener.onUserWantsToSetTrackNameAndColor(channelIndex, currentName, currentColor);
}

void TrackViewImpl::onUserWantsToManageLink()
{
    listener.onUserWantsToManageLink(channelIndex);
}

}   // namespace arkostracker
