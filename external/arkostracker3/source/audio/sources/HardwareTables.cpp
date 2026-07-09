#include "HardwareTables.h"

#include <juce_core/juce_core.h>

namespace arkostracker
{

HardwareTables::HardwareTables() noexcept :
        hardwareCurves()
{
    generateHardwareCurves();
}

unsigned char HardwareTables::getVolume(const int hardwareEnvelope, const int hardwareCurveCounter) const noexcept
{
    return hardwareCurves[static_cast<size_t>(hardwareEnvelope)][static_cast<size_t>(hardwareCurveCounter)];
}

void HardwareTables::generateHardwareCurves() noexcept
{
    for (unsigned int i = 0; i < hardwareCurvesCycleLength; ++i) {
        std::array<unsigned char, hardwareCurvesLength> curve;  // No need to initialize it. NOLINT(cppcoreguidelines-pro-type-member-init)

        const auto hold = ((i & (unsigned int)1) != 0);
        const auto alternate = ((i & (unsigned int)2) != 0);
        auto attack = ((i & (unsigned int)4) != 0);
        const auto cont = ((i & (unsigned int)8) != 0);

        auto x = 0;          // Position in the table. Always increases.
        auto direction = 1;
        auto currentVolume = 0;

        for (auto cycle = 0; cycle < hardwareCurvesNbCycles; ++cycle) {
            if (direction != 0) {
                if (attack) {
                    currentVolume = 0;
                    direction = 1;
                } else {
                    currentVolume = 31;
                    direction = -1;
                }
            }

            // Performs one cycle.
            for (int cx = 0; cx < hardwareCurvesCycleLength; ++cx) {
                jassert((currentVolume >= 0) && (currentVolume <= 255));
                curve[(size_t)x++] = (unsigned char)currentVolume;
                currentVolume += direction;
                if (currentVolume < 0) {
                    currentVolume = 0;
                } else if (currentVolume > 31) {          // 16*2-1 volumes to manage YM half-steps.
                    currentVolume = 31;
                }
            }

            // If direction at 0, we don't do anything more, the curve is stuck forever.
            if (direction != 0) {
                // If NOT Continue, the generator stays/jump at 0 forever.
                if (!cont) {
                    currentVolume = 0;
                    direction = 0;
                } else {
                    // Continue. If Hold, direction is forced to 0 forever.
                    if (hold) {
                        direction = 0;
                        if (alternate) {
                            currentVolume ^= 0x1f;       // If Hold and Alternate, inverts the Volume and lets it this way forever.
                        }
                    } else {
                        // No Hold. Alternate the curve if needed.
                        if (alternate) {
                            attack = !attack;
                        }
                    }
                }
            }
        }

        hardwareCurves[i] = curve;
    }
}

}   // namespace arkostracker
