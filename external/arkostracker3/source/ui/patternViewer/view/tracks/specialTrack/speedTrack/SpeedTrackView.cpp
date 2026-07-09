#include "SpeedTrackView.h"

#include "../../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

SpeedTrackView::SpeedTrackView(SpecialTrackView::Listener& pListener, const int pLine, const int pTrackHeight, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata,
                               juce::Font& pNormalFont, juce::Font& pDoubleHeightFont) noexcept :
        SpecialTrackView(pListener, pLine, pTrackHeight, std::move(pTrackViewMetadata), pNormalFont, pDoubleHeightFont),
        header(*this),
        textColor(juce::Colours::red)       // Temp only.
{
    findAndSetTextColor(juce::LookAndFeel::getDefaultLookAndFeel());

    addAndMakeVisible(header);
}

TrackType SpeedTrackView::getTrackType() const noexcept
{
    return TrackType::speed;
}

void SpeedTrackView::buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& paramLookAndFeel) noexcept
{
    findAndSetTextColor(paramLookAndFeel);
}

void SpeedTrackView::findAndSetTextColor(const juce::LookAndFeel& paramLookAndFeel) noexcept
{
    textColor = paramLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSpeedTrackText));
}

void SpeedTrackView::onResizedAccepted() noexcept
{
    // Sets the header new size.
    header.setBounds(0, 0, getWidth(), headerHeight);
}

juce::Colour SpeedTrackView::getCellTextColor() const noexcept
{
    return textColor;
}

bool SpeedTrackView::isSpeedTrack() const noexcept
{
    return true;
}


// SpecialTrackHeader::Listener method implementations.
// ===================================================

void SpeedTrackView::onUserWantsToNameSpecialTrack(const juce::String& currentName)
{
    listener.onUserWantsToNameSpecialTrack(true, currentName);
}

void SpeedTrackView::onUserWantsToManageSpecialLink()
{
    listener.onUserWantsToManageSpecialLink(true);
}

void SpeedTrackView::setHeaderDisplayedData(const SpecialTrackHeader::DisplayedData& displayedData) noexcept
{
    header.setDisplayedData(displayedData);
}

}   // namespace arkostracker
