#pragma once

#include "EffectsProcessor.h"

namespace arkostracker
{

class OutputMix;

/**
 * Custom MixerAudioSource that mixes the Audio Source of each PSG (for example), and
 * applies effects on the mixed signal.
 */
class PsgsProcessor final : public juce::MixerAudioSource
{
public:
    PsgsProcessor() noexcept;

    /**
     * Sets the output mix. This must be called from the UI thread.
     * @param outputMix the output mix.
     */
    void setOutputMix(const OutputMix& outputMix) noexcept;

    // MixerAudioSource method implementations.
    // =============================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) override;

private:
    EffectsProcessor effectProcessors;                // Manages all the effects.
};

}   // namespace arkostracker
