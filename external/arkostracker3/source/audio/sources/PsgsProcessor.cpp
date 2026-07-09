#include "PsgsProcessor.h"

namespace arkostracker
{

PsgsProcessor::PsgsProcessor() noexcept :
        effectProcessors()
{
}

void PsgsProcessor::prepareToPlay(const int samplesPerBlockExpected, const double sampleRate)
{
    // Asks the parent to do its job.
    MixerAudioSource::prepareToPlay(samplesPerBlockExpected, sampleRate);

    effectProcessors.prepareToPlay(sampleRate);
}

void PsgsProcessor::getNextAudioBlock(const juce::AudioSourceChannelInfo& audioSourceChannelInfo)
{
    // Audio thread
    // ===============================================

    // Asks the parent to do its job (mixing all sources).
    MixerAudioSource::getNextAudioBlock(audioSourceChannelInfo);

    effectProcessors.processBuffer(audioSourceChannelInfo);
}

void PsgsProcessor::setOutputMix(const OutputMix& outputMix) noexcept
{
    effectProcessors.setOutputMix(outputMix);
}

}   // namespace arkostracker
