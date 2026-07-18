#include "EventTrackView.h"

#include "../../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

EventTrackView::EventTrackView(SpecialTrackView::Listener& pListener, int pLine, int pTrackHeight, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata,
                               juce::Font& pNormalFont, juce::Font& pDoubleHeightFont) noexcept :
        SpecialTrackView(pListener, pLine, pTrackHeight, std::move(pTrackViewMetadata), pNormalFont, pDoubleHeightFont),
        header(*this),
        textColor(juce::Colours::red)       // Temp only.
{
    findAndSetTextColor(juce::LookAndFeel::getDefaultLookAndFeel());

    addAndMakeVisible(header);
}

TrackType EventTrackView::getTrackType() const noexcept
{
    return TrackType::event;
}

void EventTrackView::onResizedAccepted() noexcept
{
    // Sets the header new size.
    header.setBounds(0, 0, getWidth(), headerHeight);
}

void EventTrackView::buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& paramLookAndFeel) noexcept
{
    findAndSetTextColor(paramLookAndFeel);
}

void EventTrackView::findAndSetTextColor(const juce::LookAndFeel& paramLookAndFeel) noexcept
{
    textColor = paramLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerEventTrackText));
}

juce::Colour EventTrackView::getCellTextColor() const noexcept
{
    return textColor;
}

bool EventTrackView::isSpeedTrack() const noexcept
{
    return false;
}


// SpecialTrackHeader::Listener method implementations.
// ===================================================

void EventTrackView::onUserWantsToNameSpecialTrack(const juce::String& currentName)
{
    listener.onUserWantsToNameSpecialTrack(false, currentName);
}

void EventTrackView::onUserWantsToManageSpecialLink()
{
    listener.onUserWantsToManageSpecialLink(false);
}

void EventTrackView::setHeaderDisplayedData(const SpecialTrackHeader::DisplayedData& displayedData) noexcept
{
    header.setDisplayedData(displayedData);
}

}   // namespace arkostracker
