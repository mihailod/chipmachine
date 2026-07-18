#include "BaseEffect.h"

namespace arkostracker
{

BaseEffect::BaseEffect() :
        enabled(false)
{
}

void BaseEffect::setEnabled(bool newEnabled) noexcept
{
    enabled.store(newEnabled);
}

void BaseEffect::processBuffer(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) noexcept
{
    // If disabled, don't do anything.
    if (!enabled) {
        return;
    }

    // Extracts the metadata of the audio source.
    juce::AudioSampleBuffer* buffer = audioSourceChannelInfo.buffer;
    const auto bufferSize = audioSourceChannelInfo.numSamples;
    auto positionInBuffer = audioSourceChannelInfo.startSample;
    const auto afterLastPositionInBuffer = positionInBuffer + bufferSize;

    // Asks the subclass to process the buffer.
    processBuffer(buffer, positionInBuffer, afterLastPositionInBuffer);
}

}   // namespace arkostracker
