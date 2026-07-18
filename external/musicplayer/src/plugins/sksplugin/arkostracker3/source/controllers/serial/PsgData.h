#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Holds the psg data. */
class PsgData
{
public:
    /**
     * Constructor.
     * @param psgFrequency the PSG frequency (1000000hz for a CPC), in Hz.
     * @param replayFrequency the replay frequency, in Hz (50, 25, etc.).
     */
    PsgData(unsigned int psgFrequency, float replayFrequency) noexcept;

    /** @return the PSG frequency (1000000hz for a CPC), in Hz. */
    unsigned int getPsgFrequency() const noexcept;
    /** @return the replay frequency, in Hz (50, 25, etc.). */
    float getReplayFrequency() const noexcept;

private:
    unsigned int psgFrequency;                  // The PSG frequency (1000000hz for a CPC), in Hz.
    float replayFrequency;                      // The replay frequency, in Hz (50, 25, etc.).
};

}   // namespace arkostracker
