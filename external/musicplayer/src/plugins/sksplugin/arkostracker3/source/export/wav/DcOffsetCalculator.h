#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "../../utils/TotalAndWeight.h"

namespace arkostracker
{

/** Calculates the DC offset from a buffer.*/
class DcOffsetCalculator
{
public:
    /**
     * Constructor.
     * @param channelCount how many channels are to be expected (2 for stereo).
     */
    explicit DcOffsetCalculator(int channelCount) noexcept;

    /**
     * Reads the given audio source to calculate the DC offset for each channel.
     * @param audioSourceChannelInfo the audio source.
     */
    void readAudioSource(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) noexcept;

    /**
     * Applies the previously calculated DC offset to the audio sources. The channel number must fit!
     * @param audioSourceChannelInfo the audio source.
     */
    void applyDcOffset(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) const noexcept;

    /**
     * @return the DC offset of the channel which index is given. The result evolves as audio sources are read.
     * @param channelIndex the index of the channel the DC offset must be read from.
     */
    float getDcOffset(int channelIndex) const noexcept;

private:
    int channelCount;                                                   // How many channels from the buffer.
    std::vector<TotalAndWeight> totalAndWeightPerChannel;               // The total value, plus how many there are, per channel. This evolves.
};

}   // namespace arkostracker
