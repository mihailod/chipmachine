#pragma once

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** A simple holder of data to send to the meter input. */
class PeriodAndNoiseMeterInput
{
public:

    /**
     * Empty constructor, with no data (no data was received). Useful for decreasing only the bars).
     */
    PeriodAndNoiseMeterInput() :
            softwarePeriodAndVolumeOptional(),
            hardwarePeriodOptional(),
            noiseAndVolumeOptional()
    {
    }

    /**
     * Constructor.
     * @param pSoftwarePeriodAndVolumeOptional if present, a software period (0-0xfff) and volume (0-15 or 16).
     * @param pHardwarePeriodOptional if present, a hardware period (0-0xffff).
     * @param pNoiseAndVolumeOptional if present, a noise (0-31) and volume (0-15 or 16).
     */
    PeriodAndNoiseMeterInput(const OptionalValue<std::pair<int, int>>& pSoftwarePeriodAndVolumeOptional, const OptionalInt& pHardwarePeriodOptional,
                             const OptionalValue<std::pair<int, int>>& pNoiseAndVolumeOptional) :
        softwarePeriodAndVolumeOptional(pSoftwarePeriodAndVolumeOptional),
        hardwarePeriodOptional(pHardwarePeriodOptional),
        noiseAndVolumeOptional(pNoiseAndVolumeOptional)
    {
    }

    const OptionalValue<std::pair<int, int>>& getSoftwarePeriodAndVolumeOptional() const
    {
        return softwarePeriodAndVolumeOptional;
    }

    const OptionalInt& getHardwarePeriodOptional() const
    {
        return hardwarePeriodOptional;
    }

    const OptionalValue<std::pair<int, int>>& getNoiseAndVolumeOptional() const
    {
        return noiseAndVolumeOptional;
    }

private:
    OptionalValue<std::pair<int, int>> softwarePeriodAndVolumeOptional;
    OptionalInt hardwarePeriodOptional;
    OptionalValue<std::pair<int, int>> noiseAndVolumeOptional;
};

}   // namespace arkostracker

