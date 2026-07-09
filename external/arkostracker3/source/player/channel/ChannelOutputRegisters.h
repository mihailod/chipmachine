#pragma once

#include "../PsgRegisters.h"

namespace arkostracker
{

class PsgRegisters;

/** The type of the sound (soft, hard, etc.). */
enum class SoundType : uint8_t
{
    noSoftwareNoHardware,
    softwareOnly,
    hardwareOnly,
    softwareAndHardware
};

/**
 * Registers used for one channel (volume, noise, sound?, software period, hardware period, hardware envelope).
 * The retrig is also encoded.
 */
class ChannelOutputRegisters
{
public:
    /**
     * Constructor.
     * @param volume the volume (0-16).
     * @param noise the noise (0-31).
     * @param isSound true if sound is on (heard).
     * @param softwarePeriod the software period. May not be relevant if the type of the sound doesn't depend on it.
     * @param hardwarePeriod the hardware period. May not be relevant if the type of the sound doesn't depend on it.
     * @param hardwareEnvelope the hardware envelope. May not be relevant if the type of the sound doesn't depend on it.
     * @param retrig true to retrig. May not be relevant if the type of the sound doesn't depend on it.
     * @param specialEffect the special effect.
     */
    ChannelOutputRegisters(int volume, int noise, bool isSound, int softwarePeriod,
                           int hardwarePeriod, int hardwareEnvelope, bool retrig,
                           const SpecialEffect& specialEffect) noexcept;

    /** Constructor for a no-sound. */
    ChannelOutputRegisters();

    /**
     * Constructor, from a PSG Registers.
     * @param channelIndex the index of the channel (0-2).
     * @param source the PSG Registers to get the data from.
     */
    ChannelOutputRegisters(int channelIndex, const PsgRegisters& source) noexcept;

    /**
     * Builds a PsgRegisters (with only one channel set).
     * @param channelIndex the index of the channel (0-2).
     */
    PsgRegisters toPsgRegisters(int channelIndex) const noexcept;

    /** @return the type of the sound (soft, hard, etc.). */
    SoundType getSoundType() const noexcept;

    /** Indicates whether the noise if used. */
    bool isNoise() const noexcept;

    /** @return the volume, from 0 to 16. */
    int getVolume() const noexcept;
    /** @return the noise, from 0 (= unused) to 31. */
    int getNoise() const noexcept;
    /** @return the software period. May not be relevant if the type of the sound doesn't depend on it. */
    int getSoftwarePeriod() const noexcept;
    /** @return the hardware period. May not be relevant if the type of the sound doesn't depend on it. */
    int getHardwarePeriod() const noexcept;
    /** @return the hardware envelope. May not be relevant if the type of the sound doesn't depend on it. */
    int getHardwareEnvelope() const noexcept;

    /** @return whether the hardware retrig is used. May not be relevant if the type of the sound doesn't depend on it. */
    bool isRetrig() const noexcept;

    /** @return whether the registers form a "silent" sound (volume = 0, or no Software no Hardware and no noise). */
    bool isSilent() const noexcept;

    /** @return the possible special effect. */
    SpecialEffect getSpecialEffect() const noexcept;

    /** Equality operator. */
    bool operator==(const ChannelOutputRegisters& other) const noexcept;
    /** Not equal operator. */
    bool operator!=(const ChannelOutputRegisters& other) const noexcept;

private:
    /**
     * Determines the SoundType from the given sound flag and volume.
     * @param sound true if there is sound (sound channel opened).
     * @param volume the volume, from 0 to 16.
     */
    static SoundType findSoundType(bool sound, int volume) noexcept;

    int volume; // The volume, from 0 to 16.
    int noise; // The noise, or 0 if not.
    bool sound; // True if sound is on (heard).
    int softwarePeriod; // The software period. May not be relevant if the type of the sound doesn't depend on it.
    int hardwarePeriod; // The hardware period. May not be relevant if the type of the sound doesn't depend on it.
    int hardwareEnvelope; // The hardware envelope. May not be relevant if the type of the sound doesn't depend on it.
    bool retrig; // True to retrig. May not be relevant if the type of the sound doesn't depend on it.
    SoundType soundType; // The type of the sound (soft, hard, etc.).

    SpecialEffect specialEffect;
};

}   // namespace arkostracker
