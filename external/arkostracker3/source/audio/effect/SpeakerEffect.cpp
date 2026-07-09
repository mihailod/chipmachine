#include "SpeakerEffect.h"

namespace arkostracker
{

SpeakerEffect::SpeakerEffect() noexcept :
        vsa(1e-8),

        m_a1(0),
        m_a2(0),
        m_b1(0),
        m_b2(0),
        m_b0(0),

        m_x1_left(0),
        m_x2_left(0),
        m_y1_left(0),
        m_y2_left(0),

        m_x1_right(0),
        m_x2_right(0),
        m_y1_right(0),
        m_y2_right(0)
{
}

void SpeakerEffect::prepareToPlay(const double sampleRate) noexcept
{
    constexpr double cutoffFrequency = 600;     // 700 lacks a bit of bass.
    constexpr auto q = 1.0;

    constexpr auto doublePi = 3.1415926535897932384626433832795028841971;

    const auto w0 = 2 * doublePi * cutoffFrequency / sampleRate;
    const auto cs = cos(w0);
    const auto sn = sin(w0);
    const auto AL = sn / (2 * q);
    const auto b0 = (1 + cs) / 2;
    const auto b1 = -(1 + cs);
    const auto b2 = (1 + cs) / 2;
    const auto a0 = 1 + AL;
    const auto a1 = -2 * cs;
    const auto a2 = 1 - AL;

    m_a1 = a1 / a0;
    m_a2 = a2 / a0;
    m_b0 = b0 / a0;
    m_b1 = b1 / a0;
    m_b2 = b2 / a0;
}

void SpeakerEffect::releaseResources()
{
    // Nothing to do.
}

void SpeakerEffect::processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, const int afterLastPositionInBuffer) noexcept
{
    // Only managing stereo!
    if (buffer->getNumChannels() != 2) {
        jassertfalse;
        return;
    }

    for (; positionInBuffer < afterLastPositionInBuffer; ++positionInBuffer) {
        const auto sampleLeft = buffer->getSample(0, positionInBuffer);
        const auto sampleRight = buffer->getSample(1, positionInBuffer);

        // Left part.
        auto out = m_b0*sampleLeft + m_b1*m_x1_left + m_b2*m_x2_left - m_a1*m_y1_left - m_a2*m_y2_left + vsa;
        m_x2_left = m_x1_left;
        m_y2_left = m_y1_left;
        m_x1_left = sampleLeft;
        m_y1_left = out;
        buffer->setSample(0, positionInBuffer, static_cast<float>(out));

        // Right part.
        out = m_b0*sampleRight + m_b1*m_x1_right + m_b2*m_x2_right - m_a1*m_y1_right - m_a2*m_y2_right + vsa;
        m_x2_right = m_x1_right;
        m_y2_right = m_y1_right;
        m_x1_right = sampleRight;
        m_y1_right = out;
        buffer->setSample(1, positionInBuffer, static_cast<float>(out));
    }
}

}   // namespace arkostracker
