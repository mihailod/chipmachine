#pragma once

#include "../../song/instrument/sample/SamplePart.h"

namespace arkostracker
{

/**
 * This class "resamples" a sample in order to apply a new diginote.
 * This should be used when exporting a sample into a binary data, to have the new frequency applied directly.
 */
class SampleResampler
{
public:
    /** Prevents instantiation. */
    SampleResampler() = delete;

    /**
     * @return a resampled SamplePart from the given one, according to its diginote. The output one will have a default diginote.
     * @param inputSamplePart the sample part.
     * @param pitchCents a possible transposition in semi-tones. May be negative.
     */
    static SamplePart resample(const SamplePart& inputSamplePart, double pitchCents = 0.0) noexcept;

    /**
     * @return a resampled sample, according a pitch. Also returns the sample step.
     * @param inputSample the sample to resample.
     * @param inputSampleSize the sample size to resample.
     * @param note a note, from which to resample. The diginote +1 for example.
     * @param pitchCents how much to pitch according to the original sample, on top of using the note. +1 means one semi-tone. May be negative.
     * This is used on export to match a hardware player that would be too slow/fast.
     */
    static std::pair<juce::MemoryBlock, double> resample(const juce::MemoryBlock& inputSample, int inputSampleSize, int note, double pitchCents) noexcept;

private:
    static constexpr auto referenceFrequency = 440.0F;     // Doesn't matter in this case.

    /**
     * @return a resampled sample, according a pitch. Also returns the sample step.
     * @param inputSample the sample to resample.
     * @param inputSampleSize the sample size to resample.
     * @param step the step to use. Must be positive.
     */
    static std::pair<juce::MemoryBlock, double> resampleWithStep(const juce::MemoryBlock& inputSample, int inputSampleSize, double step) noexcept;
};

}   // namespace arkostracker
