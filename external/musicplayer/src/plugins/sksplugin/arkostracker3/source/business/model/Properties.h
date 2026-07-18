#pragma once

#include <juce_core/juce_core.h>

#include "../../song/subsong/Subsong.h"

namespace arkostracker
{

/** Holder of properties in the Subsong Properties screen. */
class Properties
{
public:
    Properties(juce::String name, int initialSpeed, float replayFrequencyHz, int digiChannel, int highlightSpacing, int secondaryHighlight,
        const SidPlayerCapability& sidPlayerCapability) noexcept;
    explicit Properties(const Subsong::Metadata& metadata) noexcept;

    bool operator==(const Properties& rhs) const;
    bool operator!=(const Properties& rhs) const;

    /**
     * Sets the metadata to a Subsong.
     * @param subsong the Subsong to set the metadata to.
     */
    void setMetadataToSubsong(Subsong& subsong) const noexcept;

    const juce::String& getName() const;
    int getInitialSpeed() const;
    float getReplayFrequencyHz() const;
    int getDigiChannel() const;
    int getHighlightSpacing() const;
    int getSecondaryHighlight() const;
    SidPlayerCapability getSidPlayerCapability() const;

private:
    const juce::String name;
    const int initialSpeed;
    const float replayFrequencyHz;
    const int digiChannel;
    const int highlightSpacing;
    const int secondaryHighlight;
    const SidPlayerCapability sidPlayerCapability;
};

}   // namespace arkostracker
