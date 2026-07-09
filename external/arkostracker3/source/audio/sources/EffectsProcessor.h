#pragma once

#include "../effect/SpeakerEffect.h"
#include "../effect/StereoSeparationEffect.h"
#include "../../controllers/model/OutputMix.h"
#include "../effect/VolumeEffect.h"

namespace arkostracker
{

/** Manages effects to be applied on a given buffer. */
class EffectsProcessor
{
public:
    /** Constructor. */
    EffectsProcessor() noexcept;

    /**
     * Sets the output mix. This must be called from the UI thread.
     * @param outputMix the output mix.
     */
    void setOutputMix(const OutputMix& outputMix) noexcept;

    /**
     * Called when the sample rate is known. The implementation can start making some pre-calculations.
     * @param sampleRate the sample rate (44100, for example).
     */
    void prepareToPlay(double sampleRate) noexcept;

    /**
     * Processes the buffer with all the enabled effects.
     * @param bufferToFill the buffer to fill, if needed.
     */
    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill) noexcept;

private:
    /** @return the list of all the effects. */
    std::vector<BaseEffect*> getEffects() noexcept;

    OutputMix currentOutputMix;

    SpeakerEffect speakerEffect;
    StereoSeparationEffect stereoSeparationEffect;
    VolumeEffect volumeEffect;
};

}   // namespace arkostracker
