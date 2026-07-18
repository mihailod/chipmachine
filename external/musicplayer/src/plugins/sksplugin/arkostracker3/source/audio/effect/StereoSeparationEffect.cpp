#include "StereoSeparationEffect.h"

namespace arkostracker
{

StereoSeparationEffect::StereoSeparationEffect() noexcept :
        separation(1.0)
{
}

void StereoSeparationEffect::setSeparation(double pSeparation) noexcept
{
    separation = pSeparation;
}

void StereoSeparationEffect::prepareToPlay(double /*sampleRate*/) noexcept
{
    // Nothing to do.
}

void StereoSeparationEffect::releaseResources()
{
    // Nothing to do.
}

void StereoSeparationEffect::processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, int afterLastPositionInBuffer) noexcept
{
    // Full stereo? Then don't do anything.
    if (separation >= 1.0) {
        return;
    }

    // Only managing stereo!
    if (buffer->getNumChannels() != 2) {
        jassertfalse;
        return;
    }

    // Makes the separation ranges from 0.5 to 1.0.
    const auto ratio = separation * 0.5 + 0.5;

    for (; positionInBuffer < afterLastPositionInBuffer; ++positionInBuffer) {
        auto sampleLeft = buffer->getSample(0, positionInBuffer);
        auto sampleRight = buffer->getSample(1, positionInBuffer);

        // Left.
        {
            const auto ratioedLeft = static_cast<double>(sampleLeft) * ratio;
            const auto ratioedRight = static_cast<double>(sampleRight) * (1.0 - ratio);

            const auto output = ratioedLeft + ratioedRight;
            buffer->setSample(0, positionInBuffer, static_cast<float>(output));
        }

        // Right.
        {
            const auto ratioedLeft = static_cast<double>(sampleLeft) * (1.0 - ratio);
            const auto ratioedRight = static_cast<double>(sampleRight) * ratio;

            const auto output = ratioedLeft + ratioedRight;
            buffer->setSample(1, positionInBuffer, static_cast<float>(output));
        }
    }
}

}   // namespace arkostracker
