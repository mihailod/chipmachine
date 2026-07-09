#include "SpecialEffect.h"

#include "../utils/Id.h"
#include "../utils/PsgValues.h"

namespace arkostracker
{

SpecialEffect::SpecialEffect() noexcept :
        sidActivated(false),
        highShelfPeriod(0),
        lowShelfPeriod(0),
        lowShelfVolume(PsgValues::defaultSidLowShelfVolume),
        hashcode()  // Calculated below.
{
    calculateHashcode();
}

SpecialEffect::SpecialEffect(const bool pIsSidActivated, const int pHighShelfPeriod, const int pLowShelfPeriod, const int pLowShelfVolume) noexcept :
        sidActivated(pIsSidActivated),
        highShelfPeriod(pHighShelfPeriod),
        lowShelfPeriod(pLowShelfPeriod),
        lowShelfVolume(pLowShelfVolume),
        hashcode()  // Calculated below.
{
    jassert((pLowShelfVolume >= PsgValues::minimumVolume) && (pLowShelfVolume <= PsgValues::maximumVolumeNoHard));

    calculateHashcode();
}

bool SpecialEffect::isDeactivated() const noexcept
{
    return !sidActivated;
}

bool SpecialEffect::isSidActivated() const noexcept
{
    return sidActivated;
}

int SpecialEffect::getHighShelfPeriod() const noexcept
{
    return highShelfPeriod;
}

int SpecialEffect::getLowShelfVolume() const noexcept
{
    return lowShelfVolume;
}

int SpecialEffect::getLowShelfPeriod() const noexcept
{
    return lowShelfPeriod;
}

bool SpecialEffect::operator==(const SpecialEffect& rhs) const noexcept
{
    // If both deactivated, they are equal.
    if (!sidActivated && !rhs.sidActivated) {
        return true;
    }

    return sidActivated == rhs.sidActivated
           && highShelfPeriod == rhs.highShelfPeriod
           && lowShelfPeriod == rhs.lowShelfPeriod
           && lowShelfVolume == rhs.lowShelfVolume;
}

bool SpecialEffect::operator!=(const SpecialEffect& rhs) const noexcept
{
    return !(*this == rhs);
}

size_t SpecialEffect::getHashcode() const noexcept
{
    return hashcode;
}

void SpecialEffect::calculateHashcode() noexcept
{
    size_t result = 17;

    result = (31U * result) + sidActivated ? 1U : 0U;
    result = (31U * result) + static_cast<unsigned int>(highShelfPeriod);
    result = (31U * result) + static_cast<unsigned int>(lowShelfPeriod);
    result = (31U * result) + static_cast<unsigned int>(lowShelfVolume);

    hashcode = result;
}

}   // namespace arkostracker
