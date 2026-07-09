#include "SpeedTrackHeader.h"

#include <BinaryData.h>

#include "../../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

SpeedTrackHeader::SpeedTrackHeader(Listener& pListener) noexcept :
        SpecialTrackHeader(pListener),
        image(juce::ImageFileFormat::loadFrom(BinaryData::TrackHeaderSpeed_png, BinaryData::TrackHeaderSpeed_pngSize))
{
}

juce::Colour SpeedTrackHeader::getBackgroundColor() const noexcept
{
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    return localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSpeedTrackHeaderBackground));
}

const juce::Image& SpeedTrackHeader::getImage() const noexcept
{
    return image;
}

}   // namespace arkostracker
