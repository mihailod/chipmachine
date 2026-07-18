#include "EventTrackHeader.h"

#include <BinaryData.h>

#include "../../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

EventTrackHeader::EventTrackHeader(Listener& pListener) noexcept :
        SpecialTrackHeader(pListener),
        image(juce::ImageFileFormat::loadFrom(BinaryData::TrackHeaderEvent_png, BinaryData::TrackHeaderEvent_pngSize))
{
}

juce::Colour EventTrackHeader::getBackgroundColor() const noexcept
{
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    return localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerEventTrackHeaderBackground));
}

const juce::Image& EventTrackHeader::getImage() const noexcept
{
    return image;
}


}   // namespace arkostracker

