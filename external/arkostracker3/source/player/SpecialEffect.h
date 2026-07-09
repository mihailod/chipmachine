#pragma once

#include <cstdint>
#include <cstdlib>

namespace arkostracker
{

/** Holds special effect data (sid, sync buzzer...), as low-level for direct use (not linked to the UI). Immutable. */
class SpecialEffect
{
public:
    /** Default constructor, without effect. */
    SpecialEffect() noexcept;

    /** Generic constructor. */
    SpecialEffect(bool isSidActivated, int highShelfPeriod, int lowShelfPeriod, int lowShelfVolume) noexcept;

    /** @return true if the no effect is used. */
    bool isDeactivated() const noexcept;
    /** @return true if the SID is the effect used. */
    bool isSidActivated() const noexcept;
    /** @return the high-shelf period. */
    int getHighShelfPeriod() const noexcept;
    /** @return the low-shelf period. */
    int getLowShelfPeriod() const noexcept;
    /** @return the low-shelf volume, from 0 to 15 (0 is default). */
    int getLowShelfVolume() const noexcept;

    bool operator==(const SpecialEffect& rhs) const noexcept;
    bool operator!=(const SpecialEffect& rhs) const noexcept;

    /** Calculates the hashcode and stores it. */
    void calculateHashcode() noexcept;

    /** @return the hash code.*/
    size_t getHashcode() const noexcept;

private:
    bool sidActivated;
    int highShelfPeriod;
    int lowShelfPeriod;
    int lowShelfVolume;                 // From 0 to 15.

    size_t hashcode;                    // The hashcode, calculated once and for all.
};

}   // namespace arkostracker
