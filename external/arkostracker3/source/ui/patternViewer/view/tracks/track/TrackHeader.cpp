#include "TrackHeader.h"

#include <BinaryData.h>

#include "../../../../../utils/NumberUtil.h"
#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const float TrackHeader::cornerSize = 8.0F;
const float TrackHeader::channelPathWidth = 30.0F;
const float TrackHeader::channelFontHeight = 16.0F;
const float TrackHeader::pathSeparation = 3.0F;
const float TrackHeader::displayedDataX = channelPathWidth + pathSeparation;
const float TrackHeader::alphaButtonsUseless = 0.1F;
const int TrackHeader::trackColorHeight = 2;


// TrackHeader::DisplayedData
// ============================================================

TrackHeader::DisplayedData::DisplayedData(const bool pChannelOn, const int pChannelNumber, const int pTransposition,
    juce::String pTrackName, const OptionalValue<juce::Colour> pTrackColor, const LinkState pLinkState) noexcept :
        channelOn(pChannelOn),
        channelNumber(pChannelNumber),
        transposition(pTransposition),
        trackName(std::move(pTrackName)),
        trackColor(pTrackColor),
        linkState(pLinkState)
{
}

bool TrackHeader::DisplayedData::isChannelOn() const noexcept
{
    return channelOn;
}

int TrackHeader::DisplayedData::getChannelNumber() const noexcept
{
    return channelNumber;
}

int TrackHeader::DisplayedData::getTransposition() const noexcept
{
    return transposition;
}

const juce::String& TrackHeader::DisplayedData::getTrackName() const noexcept
{
    return trackName;
}

OptionalValue<juce::Colour> TrackHeader::DisplayedData::getTrackColor() const noexcept
{
    return trackColor;
}

LinkState TrackHeader::DisplayedData::getLinkState() const noexcept
{
    return linkState;
}

bool TrackHeader::DisplayedData::operator==(const TrackHeader::DisplayedData& rhs) const
{
    return channelOn == rhs.channelOn &&
           channelNumber == rhs.channelNumber &&
           transposition == rhs.transposition &&
           linkState == rhs.linkState &&
           trackName == rhs.trackName &&
           trackColor == rhs.trackColor;
}

bool TrackHeader::DisplayedData::operator!=(const TrackHeader::DisplayedData& rhs) const
{
    return !(rhs == *this);
}


// ============================================================

TrackHeader::TrackHeader(Listener& pListener) noexcept :
        listener(pListener),
        channelBackgroundPath(),
        corneredBackgroundPath(),
        trackNameLabel(juce::String(), [&] { listener.onUserWantsToNameTrack(
            displayedData.getTrackName(), displayedData.getTrackColor()
        ); }),
        transpositionButton(juce::String(), juce::translate("The transposition for the channel. Click to modify.")),
        linkButton(
            // Off: Link. On: Linked to.
            BinaryData::IconLink_png, static_cast<size_t>(BinaryData::IconLink_pngSize),
            BinaryData::IconLinkedTo_png, static_cast<size_t>(BinaryData::IconLinkedTo_pngSize),
            juce::translate("Link/unlink this track"), [&](int, bool, bool) {
            onLinkButtonClicked();
        }),
        channelFont(juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMonoB_ttf, BinaryData::UbuntuMonoB_ttfSize)),
        displayedData(true, 0, 0, juce::String(), { }, LinkState::none),      // Unreachable value to force refresh.
        meter()
{
    setOpaque(false);           // Because rounded corner.

    // Removes the focus, else pressing Return will give focus to it.
    transpositionButton.setWantsKeyboardFocus(false);
    transpositionButton.setMouseClickGrabsKeyboardFocus(false);
    transpositionButton.onClick = [&] { onTranspositionButtonClicked(); };
    channelFont.setHeight(channelFontHeight);
    trackNameLabel.setAlpha(0.5F);

    addAndMakeVisible(transpositionButton);
    addAndMakeVisible(meter);
    addAndMakeVisible(trackNameLabel);
    addAndMakeVisible(linkButton);
}

void TrackHeader::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto widthFloat = static_cast<float>(getWidth());
    const auto heightFloat = static_cast<float>(height);
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::smallMargins;
    const auto backgroundX = displayedDataX;

    // Refreshes the channel path.
    channelBackgroundPath.clear();
    channelBackgroundPath.addRoundedRectangle(0.0F, 0.0F, channelPathWidth, heightFloat, cornerSize, cornerSize,
                                              true, false, false, false);

    // Refreshes the background path.
    const auto backgroundWidth = widthFloat - backgroundX;
    corneredBackgroundPath.clear();
    corneredBackgroundPath.addRoundedRectangle(backgroundX, 0.0F, backgroundWidth, heightFloat, cornerSize, cornerSize,
                                               false, true, false, false);

    // The transposition to the right.
    const auto transpositionButtonHeight = height - 4;
    const auto transpositionButtonY = static_cast<int>(std::round(static_cast<float>(height - transpositionButtonHeight) / 2.0F));
    constexpr auto transpositionButtonWidth = 42;
    const auto linkButtonWidth = LookAndFeelConstants::iconButtonsWidth;
    transpositionButton.setBounds(width - margins - transpositionButtonWidth, transpositionButtonY, transpositionButtonWidth, transpositionButtonHeight);

    // The Link button to its right.
    linkButton.setBounds(transpositionButton.getX() - linkButtonWidth - smallMargins, transpositionButton.getY(), linkButtonWidth, transpositionButtonHeight);

    // The meter in the middle.
    constexpr auto meterMargins = 5;
    const auto meterX = static_cast<int>(backgroundX) + meterMargins;
    const auto meterWidth = linkButton.getX() - meterX - meterMargins;
    meter.setBounds(meterX, meterMargins, meterWidth, height - (2 * meterMargins));

    // The track name in the middle too.
    const auto trackNameLabelX = static_cast<int>(backgroundX);
    constexpr auto trackNameLabelVerticalMargins = 5;
    trackNameLabel.setBounds(trackNameLabelX, trackNameLabelVerticalMargins,
        linkButton.getX() - trackNameLabelX, height - (2 * trackNameLabelVerticalMargins));
}

void TrackHeader::paint(juce::Graphics& g)
{
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    const auto height = getHeight();

    const auto channelOn = displayedData.isChannelOn();

    // The channel on the left.
    const auto channelBackgroundColor = localLookAndFeel.findColour(static_cast<int>(channelOn
                                                                                     ? LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOn
                                                                                     : LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOff));
    g.setColour(channelBackgroundColor);
    jassert(!channelBackgroundPath.isEmpty());         // Path not ready yet?!
    g.fillPath(channelBackgroundPath);

    // The rest of the background.
    const auto backgroundColor = localLookAndFeel.findColour(static_cast<int>(channelOn
                                                                              ? LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOn
                                                                              : LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOff));
    g.setColour(backgroundColor);
    jassert(!corneredBackgroundPath.isEmpty());         // Path not ready yet?!
    g.fillPath(corneredBackgroundPath);

    // Displays the channel number.
    g.setColour(localLookAndFeel.findColour(juce::Label::ColourIds::textColourId));
    g.setFont(channelFont);
    g.drawFittedText(juce::String(displayedData.getChannelNumber()),
                     0, 0, static_cast<int>(channelPathWidth), height,
                     juce::Justification::centred, 1, 1.0F);

    // The possible track color at the bottom.
    if (const auto& trackColor = displayedData.getTrackColor(); trackColor.isPresent()) {
        g.setColour(trackColor.getValueRef());
        const auto trackColorX = static_cast<int>(displayedDataX);
        g.fillRect(trackColorX, height - trackColorHeight, getWidth() - trackColorX, trackColorHeight);
    }
}

void TrackHeader::paintOverChildren(juce::Graphics& g)
{
    if (!displayedData.isChannelOn()) {
        // The header will take of its own dimming.
        const auto filterColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerBackground))
                .withAlpha(0.4F);
        g.setColour(filterColor);
        g.fillPath(channelBackgroundPath);
        g.fillPath(corneredBackgroundPath);
    }
}

void TrackHeader::mouseDown(const juce::MouseEvent& event)
{
    // Click on the Channel part?
    if (static_cast<float>(event.getMouseDownPosition().getX()) < channelPathWidth) {
        if (event.mods.isLeftButtonDown()) {
            listener.onUserLeftClickedOnChannelNumber();
        } else {
            listener.onUserRightClickedOnChannelNumber();
        }
    }
}

void TrackHeader::setDisplayedData(TrackHeader::DisplayedData newDisplayedData) noexcept
{
    // Any change? If not, don't do anything.
    if (displayedData == newDisplayedData) {
        return;
    }
    displayedData = std::move(newDisplayedData);

    // Not managed via Paint method.
    const auto transposition = displayedData.getTransposition();
    transpositionButton.setButtonText(NumberUtil::signedHexToStringWithPrefix(transposition, juce::String(), true));
    transpositionButton.setAlpha(transposition == 0 ? alphaButtonsUseless : 1.0F);

    linkButton.setState(displayedData.getLinkState() == LinkState::linkedTo);
    linkButton.setAlpha(displayedData.getLinkState() == LinkState::none ? alphaButtonsUseless : 1.0F);

    trackNameLabel.setText(displayedData.getTrackName(), juce::NotificationType::dontSendNotification);
    repaint();
}

void TrackHeader::updateMeterAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept
{
    meter.updateAndRefresh(periodAndNoiseMeterInput);
}

void TrackHeader::onTranspositionButtonClicked() const noexcept
{
    listener.onUserWantsToChangeTransposition();
}

void TrackHeader::onLinkButtonClicked() const noexcept
{
    listener.onUserWantsToManageLink();
}

}   // namespace arkostracker
