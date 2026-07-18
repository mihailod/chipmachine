#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

class SourceProfile;

/** Class to generate a source with the periods (of a PSG). */
class GeneratePeriods
{
public:
    /** Prevents instantiation. */
    GeneratePeriods() = delete;

    /**
     * @return the source, one word for each period.
     * @param psgFrequencyHz the PSG frequency, in Hz.
     * @param referenceFrequencyHz the reference frequency, in Hz.
     * @param sourceProfile the source profile to use.
     */
    static std::unique_ptr<juce::MemoryOutputStream> generate(int psgFrequencyHz, float referenceFrequencyHz, const SourceProfile& sourceProfile) noexcept;
};

}   // namespace arkostracker
