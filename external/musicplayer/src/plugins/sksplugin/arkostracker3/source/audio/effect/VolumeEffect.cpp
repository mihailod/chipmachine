#include "VolumeEffect.h"

namespace arkostracker
{

VolumeEffect::VolumeEffect() noexcept :
        volume(1.0)
{
}

void VolumeEffect::setVolume(const double newVolume) noexcept
{
    volume = newVolume;
}

void VolumeEffect::prepareToPlay(double /*sampleRate*/) noexcept
{
    // Nothing to do.
}

void VolumeEffect::releaseResources()
{
    // Nothing to do.
}

void VolumeEffect::processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, const int afterLastPositionInBuffer) noexcept
{
    // Full volume? Then don't do anything.
    if (volume >= 1.0) {
        return;
    }

    const auto volumeFloat = static_cast<float>(volume);
    const auto channelCount = buffer->getNumChannels();

    for (; positionInBuffer < afterLastPositionInBuffer; ++positionInBuffer) {
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            auto sample = buffer->getSample(channelIndex, positionInBuffer);
            sample *= volumeFloat;
            buffer->setSample(channelIndex, positionInBuffer, sample);
        }
    }
}

}   // namespace arkostracker
