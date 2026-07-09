#include "Properties.h"

namespace arkostracker
{
Properties::Properties(juce::String pName, const int pInitialSpeed, const float pReplayFrequencyHz, const int pDigiChannel,
    const int pHighlightSpacing, const int pSecondaryHighlight, const SidPlayerCapability& pSidPlayerCapability) noexcept :
            name(std::move(pName)),
            initialSpeed(pInitialSpeed),
            replayFrequencyHz(pReplayFrequencyHz),
            digiChannel(pDigiChannel),
            highlightSpacing(pHighlightSpacing),
            secondaryHighlight(pSecondaryHighlight),
            sidPlayerCapability(pSidPlayerCapability)
{
}

Properties::Properties(const Subsong::Metadata& metadata) noexcept :
        name(metadata.getName()),
        initialSpeed(metadata.getInitialSpeed()),
        replayFrequencyHz(metadata.getReplayFrequencyHz()),
        digiChannel(metadata.getDigiChannel()),
        highlightSpacing(metadata.getHighlightSpacing()),
        secondaryHighlight(metadata.getSecondaryHighlight()),
        sidPlayerCapability(metadata.getSidPlayerCapability())
{
}

bool Properties::operator==(const Properties& rhs) const
{
    return name == rhs.name &&
           initialSpeed == rhs.initialSpeed &&
           juce::exactlyEqual(replayFrequencyHz, rhs.replayFrequencyHz) &&
           digiChannel == rhs.digiChannel &&
           highlightSpacing == rhs.highlightSpacing &&
           secondaryHighlight == rhs.secondaryHighlight &&
           sidPlayerCapability == rhs.sidPlayerCapability;
}

bool Properties::operator!=(const Properties& rhs) const
{
    return !(rhs == *this);
}

const juce::String& Properties::getName() const
{
    return name;
}

int Properties::getInitialSpeed() const
{
    return initialSpeed;
}

float Properties::getReplayFrequencyHz() const
{
    return replayFrequencyHz;
}

int Properties::getDigiChannel() const
{
    return digiChannel;
}

int Properties::getHighlightSpacing() const
{
    return highlightSpacing;
}

int Properties::getSecondaryHighlight() const
{
    return secondaryHighlight;
}

SidPlayerCapability Properties::getSidPlayerCapability() const
{
    return sidPlayerCapability;
}

void Properties::setMetadataToSubsong(Subsong& subsong) const noexcept
{
    subsong.setName(name);
    subsong.setInitialSpeed(initialSpeed);
    subsong.setReplayFrequency(replayFrequencyHz);
    subsong.setDigiChannel(digiChannel);
    subsong.setHighlightSpacings(highlightSpacing, secondaryHighlight);
    subsong.setSidPlayerCapability(sidPlayerCapability);
}

}   // namespace arkostracker
