#pragma once

#include <array>
#include <juce_core/juce_core.h>

#include "SpecialEffect.h"

namespace arkostracker 
{

/** The index of each PSG Register, as on the hardware. */
enum class PsgRegistersIndex : uint8_t
{
    softwarePeriodLowChannel1 = 0,
    softwarePeriodHighChannel1,
    softwarePeriodLowChannel2,
    softwarePeriodHighChannel2,
    softwarePeriodLowChannel3,
    softwarePeriodHighChannel3,

    noise,

    mixer,

    volumeChannel1,
    volumeChannel2,
    volumeChannel3,

    hardwarePeriodLow,
    hardwarePeriodHigh,

    hardwareEnvelope
};



/** Immutable holder of a hardware envelope and the retrig state. */
class HardwareEnvelopeAndRetrig
{
public:
    /**
     * Constructor.
     * @param r13 the register 13, from 0 to 15.
     * @param retrig true if Retrig.
     */
    HardwareEnvelopeAndRetrig(int r13, bool retrig) noexcept;

    /** Static constructor to get a default r13 (8), and no retrig. */
    static HardwareEnvelopeAndRetrig buildDefault() noexcept;

    int getEnvelope() const noexcept;
    bool isRetrig() const noexcept;
    void setRetrig(bool retrig) noexcept;

private:
    int envelope;                       // The envelope (register 13), from 0 to 15.
    bool retrig;                        // True if retrig.
};

/**
 * Holds all the PSG registers. This obviously represents the registers of one PSG only, three channels.
 * It also holds "alien" data, like special effects (for SID).
 */
class PsgRegisters
{
public:
    static constexpr auto registerCount = 16;                    // How many registers there are.

    /** Constructor, empty sounds (all registers are set to 0, the mixer has all the channels closed, but the R13 is the 8, the minimum value used in AT). */
    PsgRegisters();

    /**
     * Returns the volume of one channel.
     * @param channelIndex the index of the channel (0-2).
     * @return volume the volume (0-15 or 16).
     */
    int getVolume(int channelIndex) const noexcept;

    /**
     * Returns true if the channel uses hardware volume (bit 5 to 1).
     * @param channelIndex the index of the channel (0-2).
     * @return true if the channel uses hardware volume.
     */
    bool isHardwareVolume(int channelIndex) const noexcept;

    /**
     * Sets the volume of one channel.
     * @param channelIndex the index of the channel (0-2).
     * @param volume the volume (0-15 or 16).
     */
    void setVolume(int channelIndex, int volume) noexcept;

    /**
     * Returns the software period for the given channel.
     * @param channelIndex the index of the channel (0-2).
     * @return the software period (0-0xfff).
     */
    int getSoftwarePeriod(int channelIndex) const noexcept;

    /**
     * Sets the period.
     * @param channelIndex the index of the channel (0-2).
     * @param period from 0 to 0xfff included.
     */
    void setSoftwarePeriod(int channelIndex, int period) noexcept;

    /** Returns the noise (0-31). */
    int getNoise() const noexcept;

    /**
     * Sets the noise. Warning, you should also open the noise mixer channel for it to be useful.
     * @param noise the noise (0-31).
     */
    void setNoise(int noise) noexcept;

    /** @return the mixer value (0-0x3f). */
    int getMixer() const noexcept;

    /**
     * Sets the mixer value.
     * @param mixer the mixer value (0-0x3f).
     */
    void setMixer(int mixer) noexcept;

    /**
     * Sets the noise state for the mixer.
     * @param channelIndex the index of the channel (0-2).
     * @param open true to open the noise (0 in the mixer).
     */
    void setMixerNoiseState(int channelIndex, bool open) noexcept;

    /**
     * Sets the sound state for the mixer.
     * @param channelIndex the index of the channel (0-2).
     * @param open true to open the sound (0 in the mixer).
     */
    void setMixerSoundState(int channelIndex, bool open) noexcept;

    /**
     * Returns the mixer state for the given channel.
     * @param channelIndex the index of the channel (0-2).
     * @returns true if the mixer channel is opened (sound is heard).
     */
    bool getMixerSoundState(int channelIndex) const noexcept;

    /**
     * Returns the mixer noise state for the given channel.
     * @param channelIndex the index of the channel (0-2).
     * @returns true if the noise channel is opened (sound is heard).
     */
    bool getMixerNoiseState(int channelIndex) const noexcept;

    /** @return the hardware period (from 0 to 0xffff). */
    int getHardwarePeriod() const noexcept;

    /**
     * Sets the hardware period.
     * @param hardwarePeriod the hardware period (from 0 to 0xffff).
     */
    void setHardwarePeriod(int hardwarePeriod) noexcept;

    /** @return the hardware envelope, from 0 to 0xf included, plus the retrig state. */
    HardwareEnvelopeAndRetrig getHardwareEnvelopeAndRetrig() const noexcept;

    /**
     * Sets the hardware envelope and the retrig state.
     * Note: the Retrig state should NOT be set to true in case of a value change, because it is more logical this way.
     * However, this also means the client may have to store the previous R13. This can be important for YM generation tools.
     * @param hardwareEnvelope the hardware envelope, from 0 to 0xf included. 255 is NOT accepted (this is only related to the YM format).
     * @param retrig true to indicate a Retrig is forced, OR the value has changed.
    */
    void setHardwareEnvelopeAndRetrig(int hardwareEnvelope, bool retrig) noexcept;

    /**
     * Returns the PSG value from the given register.
     * @param registerIndex the index of the register (0-15).
     * @return the value.
     */
    int getValueFromRegister(int registerIndex) const noexcept;

    /**
     * Sets a value from its register.
     * @param registerIndex the index of the register (0-15).
     * @param value the value to encode.
     */
    void setValue(int registerIndex, int value) noexcept;

    /** @return the possible event number, from 1-255, or 0 if there is none. */
    int getEventNumber() const noexcept;

    /**
     * Sets an Event.
     * @param eventNumber the event number (1-255, 0 is none).
     */
    void setEventNumber(int eventNumber) noexcept;

    /**
     * Sets the Retrig state.
     * @param retrig true if a Retrig is forced.
     */
    void setRetrig(bool retrig);

    /** Indicates whether a Retrig is forced. */
    bool isRetrig() const;

    /**
    * Mutes the channels, if the mute state of the channels are on.
    * @param muteStates bit 0 set if channel A is mute, etc.
    */
    void muteChannelFromMuteStates(unsigned char muteStates) noexcept;

    /**
    * Mutes the channel (volume, mixer, noise).
    * @param channelIndex the channel index (0-2).
    */
    void muteChannel(int channelIndex) noexcept;

    /** Sets to mute, mixer closed, R13 reset to 8. */
    void clear() noexcept;

    /** Corrects the internal values by removing the useless bits (not by checking min/max). */
    void correct() noexcept;

    /** Sets the Special Effects data. */
    void setSpecialEffect(int channelIndex, const SpecialEffect& specialEffects) noexcept;
    /** @return the Special Effects data. */
    const SpecialEffect& getSpecialEffects(int channelIndex) const noexcept;

    bool operator==(const PsgRegisters& rhs) const;
    bool operator!=(const PsgRegisters& rhs) const;

private:
    /**
     * Corrects a stored value by applying a mask.
     * @param psgRegistersIndex the index of the register.
     * @param mask the mask to apply (0b1111 for example).
     */
    void correctValue(PsgRegistersIndex psgRegistersIndex, unsigned int mask) noexcept;

    std::array<int, static_cast<unsigned int>(registerCount)> registers;               // All the registers.
    int eventNumber;                                        // A possible event index (0 if there is none).
    bool retrig;                                            // True if a retrig is forced.

    std::unordered_map<int, SpecialEffect> channelToSpecialEffects;
};

}   // namespace arkostracker
