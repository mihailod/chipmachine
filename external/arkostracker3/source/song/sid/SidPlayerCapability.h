#pragma once

#include <juce_core/juce_core.h>

#include "SidPlayerProfile.h"

namespace arkostracker
{

/** Represents the capabilities of a SID player. Some may be specific, without frequency (Atari ST). */
class SidPlayerCapability
{
public:
    /** @return a default profile (CPC). */
    static SidPlayerCapability buildDefault() noexcept;

    /** @return a profile for CPC. */
    static SidPlayerCapability buildForCpc() noexcept;
    /** @return a profile for Amstrad Plus. */
    static SidPlayerCapability buildForAmstradPlus() noexcept;
    /** @return a profile for CPC. */
    static SidPlayerCapability buildForAtariSt() noexcept;
    /** @return a custom profile. */
    static SidPlayerCapability buildForCustom(int frequencyHz, int minimumPeriod, int maximumPeriod) noexcept;

    /** @return the profile. */
    SidPlayerProfile getSidPlayerProfile() const noexcept;

    /** @return the player frequency, in Hz. May not be relevant (-1 if not). */
    int getFrequencyHz() const noexcept;

    /** @return the minimum period. */
    int getMinimumPeriod() const noexcept;
    /** @return the maximum period. */
    int getMaximumPeriod() const noexcept;

    /** @return a String for display. */
    juce::String toDisplayedString() const noexcept;

    bool operator==(const SidPlayerCapability& other) const noexcept;
    bool operator!=(const SidPlayerCapability& other) const noexcept;

private:
    /**
     * Constructor.
     * @param sidPlayerProfile The player profile.
     * @param frequencyHz the frequency in Hz. -1 if not relevant.
     * @param minimumPeriod the minimum period.
     * @param maximumPeriod the maximum period.
     */
    SidPlayerCapability(SidPlayerProfile sidPlayerProfile, int frequencyHz, int minimumPeriod, int maximumPeriod) noexcept;

    SidPlayerProfile sidPlayerProfile;
    int frequencyHz;
    int minimumPeriod;
    int maximumPeriod;
};

}   // namespace arkostracker
