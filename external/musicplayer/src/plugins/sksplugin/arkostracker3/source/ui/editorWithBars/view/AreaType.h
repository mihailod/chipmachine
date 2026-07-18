#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** The various area where bars are displayed. This is NOT the order in which they will be displayed. */
enum class AreaType : int       // Negative values can be used along with AreaTypes values.
{
    soundType,          // Must be >=0.
    envelope,
    noise,

    primaryPeriod,
    primaryArpeggioNoteInOctave,
    primaryArpeggioOctave,
    primaryPitch,

    secondaryPeriod,
    secondaryArpeggioNoteInOctave,
    secondaryArpeggioOctave,
    secondaryPitch,

    sidIsActivatedAndRatio,
    sidBalancePercent,
    sidLowShelfVolume,
    sidArpeggio,
    sidPitch,

    last = sidPitch,
};

class AreaTypeHelper
{
public:
    /** @return all the area types, in order they are displayed. */
    static const std::vector<AreaType>& getAllAreaTypes() noexcept
    {
        static const std::vector areaTypes = {
                AreaType::soundType, AreaType::envelope, AreaType::noise,
                AreaType::primaryPeriod, AreaType::primaryArpeggioNoteInOctave, AreaType::primaryArpeggioOctave, AreaType::primaryPitch,
                AreaType::secondaryPeriod, AreaType::secondaryArpeggioNoteInOctave, AreaType::secondaryArpeggioOctave,  AreaType::secondaryPitch,
                AreaType::sidIsActivatedAndRatio, AreaType::sidArpeggio, AreaType::sidPitch, AreaType::sidBalancePercent, AreaType::sidLowShelfVolume,
        };
        jassert(areaTypes.size() == (static_cast<size_t>(AreaType::last) + 1U));              // Missing area?

        static_assert(static_cast<int>(AreaType::soundType) >= 0, "Must be 0, as stated in the documentation.");

        return areaTypes;
    }
};

}   // namespace arkostracker

