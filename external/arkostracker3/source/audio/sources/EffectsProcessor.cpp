#include "EffectsProcessor.h"

namespace arkostracker
{

EffectsProcessor::EffectsProcessor() noexcept :
        currentOutputMix(OutputMix::buildDefault()),
        speakerEffect(),
        stereoSeparationEffect(),
        volumeEffect()
{
    stereoSeparationEffect.setEnabled(true);
    volumeEffect.setEnabled(true);
}

void EffectsProcessor::setOutputMix(const OutputMix& newOutputMix) noexcept
{
    // No change? Then don't do anything.
    if (currentOutputMix == newOutputMix) {
        return;
    }
    currentOutputMix = newOutputMix;

    // Applies the new values. The channel volume is NOT used here.
    speakerEffect.setEnabled(newOutputMix.isSpeakerEmulated());
    stereoSeparationEffect.setSeparation(newOutputMix.getStereoSeparation() / 100.0);
    volumeEffect.setVolume(newOutputMix.getGlobalVolume() / 100.0);
}

void EffectsProcessor::prepareToPlay(const double sampleRate) noexcept
{
    for (auto* effect : getEffects()) {
        effect->prepareToPlay(sampleRate);
    }
}

void EffectsProcessor::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill) noexcept
{
    for (auto* effect : getEffects()) {
        effect->processBuffer(bufferToFill);
    }
}

std::vector<BaseEffect*> EffectsProcessor::getEffects() noexcept
{
    return { &speakerEffect, &stereoSeparationEffect, &volumeEffect };
}

}   // namespace arkostracker
