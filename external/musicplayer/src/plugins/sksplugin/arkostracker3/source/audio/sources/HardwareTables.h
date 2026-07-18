#pragma once

#include <array>

namespace arkostracker
{

/**
 * Convenient class to create look-up hardware tables.
 * The hardware tables have 32 steps, to manage both AY (16 steps) and YM (32 steps for hardware envelope).
 */
class HardwareTables
{
public:
    /** Constructor. */
    HardwareTables() noexcept;

    /**
     * @return the volume, from 0 to 31 (to manage YM half-volumes).
     * @param hardwareEnvelope the hardware envelope number.
     * @param hardwareCurveCounter the cycle in the hardware curve.
     */
    unsigned char getVolume(int hardwareEnvelope, int hardwareCurveCounter) const noexcept;

    static constexpr auto hardwareCurvesCycleLength = 32;                // Nb steps a cycle consists of. 32 to manage YM half-volumes.
    static constexpr auto hardwareCurvesLength = hardwareCurvesCycleLength * 4;
    static constexpr auto hardwareCurvesNbCycles = 4;
    static constexpr auto hardwareCurvesLoopTo = hardwareCurvesNbCycles / 2 * hardwareCurvesCycleLength; // We always loop to the 2nd cycle.

private:
    /** Generates the hardware curves. Doing it once is enough. */
    void generateHardwareCurves() noexcept;

    /** Points to an array of volumes, each for one hardware curve. */
    std::array<std::array<unsigned char, hardwareCurvesLength>, hardwareCurvesCycleLength> hardwareCurves;
};

}   // namespace arkostracker
