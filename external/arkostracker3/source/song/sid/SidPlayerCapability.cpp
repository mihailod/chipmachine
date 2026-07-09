#include "SidPlayerCapability.h"

namespace arkostracker
{

SidPlayerCapability::SidPlayerCapability(const SidPlayerProfile pSidPlayerProfile, const int pFrequencyHz, const int pMinimumPeriod, const int pMaximumPeriod) noexcept :
        sidPlayerProfile(pSidPlayerProfile),
        frequencyHz(pFrequencyHz),
        minimumPeriod(pMinimumPeriod),
        maximumPeriod(pMaximumPeriod)
{
}

SidPlayerCapability SidPlayerCapability::buildDefault() noexcept
{
    return buildForCpc();
}

SidPlayerCapability SidPlayerCapability::buildForCpc() noexcept
{
    return { SidPlayerProfile::cpc, 312 * 50, 2, 255 };
}

SidPlayerCapability SidPlayerCapability::buildForAmstradPlus() noexcept
{
    return { SidPlayerProfile::amstradPlus, 312 * 50, 2, 0xfff };
}

SidPlayerCapability SidPlayerCapability::buildForAtariSt() noexcept
{
    return { SidPlayerProfile::atariSt, -1, 1, 0xfff };
}

SidPlayerCapability SidPlayerCapability::buildForCustom(const int pFrequencyHz, const int pMinimumPeriod, int pMaximumPeriod) noexcept
{
    return { SidPlayerProfile::custom, pFrequencyHz, pMinimumPeriod, pMaximumPeriod };
}

SidPlayerProfile SidPlayerCapability::getSidPlayerProfile() const noexcept
{
   return sidPlayerProfile;
}

int SidPlayerCapability::getFrequencyHz() const noexcept
{
    return frequencyHz;
}

int SidPlayerCapability::getMinimumPeriod() const noexcept
{
    return minimumPeriod;
}

int SidPlayerCapability::getMaximumPeriod() const noexcept
{
    return maximumPeriod;
}

juce::String SidPlayerCapability::toDisplayedString() const noexcept
{
    juce::String baseText;
    switch (sidPlayerProfile) {
        case SidPlayerProfile::cpc:
            baseText = juce::translate("Amstrad CPC");
            break;
        case SidPlayerProfile::amstradPlus:
            baseText = juce::translate("Amstrad Plus");
            break;
        case SidPlayerProfile::atariSt:
            baseText = juce::translate("Atari ST");
            break;
        case SidPlayerProfile::custom:
            baseText = juce::translate("Custom");
            break;
    }

    juce::String frequencyText;
    if (sidPlayerProfile == SidPlayerProfile::atariSt) {
        frequencyText = juce::translate("MFP-bound");
    } else {
        frequencyText = juce::String(frequencyHz) + " Hz";
    }

    return baseText + " (" + frequencyText + ". Periods from " + juce::String(minimumPeriod) + " to " + juce::String(maximumPeriod) + ")";
}

bool SidPlayerCapability::operator==(const SidPlayerCapability& other) const noexcept
{
    return sidPlayerProfile == other.sidPlayerProfile
               && frequencyHz == other.frequencyHz
               && minimumPeriod == other.minimumPeriod
               && maximumPeriod == other.maximumPeriod;
}

bool SidPlayerCapability::operator!=(const SidPlayerCapability& other) const noexcept
{
    return !(*this == other);
}

}   // namespace arkostracker
