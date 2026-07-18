#include "ChannelOutputRegisters.h"

#include <juce_core/juce_core.h>

#include "../../utils/PsgValues.h"
#include "../PsgRegisters.h"

namespace arkostracker 
{

ChannelOutputRegisters::ChannelOutputRegisters(const int pVolume, const int pNoise, const bool pSound, const int pSoftwarePeriod,
                                               const int pHardwarePeriod, const int pHardwareEnvelope, const bool pRetrig,
                                               const SpecialEffect& pSpecialEffect) noexcept :
        volume(pVolume),
        noise(pNoise),
        sound(pSound),
        softwarePeriod(pSoftwarePeriod),
        hardwarePeriod(pHardwarePeriod),
        hardwareEnvelope(pHardwareEnvelope),
        retrig(pRetrig),
        soundType(findSoundType(sound, volume)),
        specialEffect(pSpecialEffect)
{
}

ChannelOutputRegisters::ChannelOutputRegisters() :
        volume(0),
        noise(0),
        sound(),
        softwarePeriod(0),
        hardwarePeriod(0),
        hardwareEnvelope(0),
        retrig(false),
        soundType(SoundType::noSoftwareNoHardware),
        specialEffect()
{
}

ChannelOutputRegisters::ChannelOutputRegisters(const int channelIndex, const PsgRegisters& source) noexcept :
        volume(source.getVolume(channelIndex)),
        noise(source.getMixerNoiseState(channelIndex) ? source.getNoise() : 0),     // Don't encode the noise of the mixer is off!
        sound(source.getMixerSoundState(channelIndex)),
        softwarePeriod(source.getSoftwarePeriod(channelIndex)),
        hardwarePeriod(source.getHardwarePeriod()),
        hardwareEnvelope(source.getHardwareEnvelopeAndRetrig().getEnvelope()),
        retrig(source.isRetrig()),
        soundType(findSoundType(sound, volume)),
        specialEffect(source.getSpecialEffects(channelIndex))
{
    jassert((channelIndex >= 0) && (channelIndex <= 2));
}

SoundType ChannelOutputRegisters::getSoundType() const noexcept
{
    return soundType;
}

bool ChannelOutputRegisters::isNoise() const noexcept
{
    return (noise > 0);
}

int ChannelOutputRegisters::getVolume() const noexcept
{
    return volume;
}

int ChannelOutputRegisters::getNoise() const noexcept
{
    return noise;
}

int ChannelOutputRegisters::getSoftwarePeriod() const noexcept
{
    return softwarePeriod;
}
int ChannelOutputRegisters::getHardwarePeriod() const noexcept
{
    return hardwarePeriod;
}
int ChannelOutputRegisters::getHardwareEnvelope() const noexcept
{
    return hardwareEnvelope;
}
bool ChannelOutputRegisters::isRetrig() const noexcept
{
    return retrig;
}

bool ChannelOutputRegisters::isSilent() const noexcept
{
    if (volume == 0) {
        return true;
    }

    // There is a volume... But if it is a "no software no hardware" sound and if there is no noise, it means silence.
    return ((soundType == SoundType::noSoftwareNoHardware) && !isNoise());
}

bool ChannelOutputRegisters::operator==(const ChannelOutputRegisters& other) const noexcept
{
    // The volume is 0 to 16, so hardware sounds still use it. It can be compared in all cases.
    if ((soundType != other.soundType) || (noise != other.noise) || (volume != other.volume)) {
        return false;
    }

    auto equal = true;

    // Only compares the fields relevant to the sound type.

    // Software period.
    if ((soundType == SoundType::softwareOnly) || (soundType == SoundType::softwareAndHardware)) {
        equal = (softwarePeriod == other.softwarePeriod);
    }

    // Hardware.
    if ((soundType == SoundType::hardwareOnly) || (soundType == SoundType::softwareAndHardware)) {
        equal = equal && ((hardwarePeriod == other.hardwarePeriod)
                  && (hardwareEnvelope == other.hardwareEnvelope)
                  && (retrig == other.retrig)
        );
    }

    if (equal) {
        // SID.
        if (specialEffect != other.specialEffect) {
            return false;
        }
    }

    return equal;
}

bool ChannelOutputRegisters::operator!=(const ChannelOutputRegisters& other) const noexcept
{
    return !operator==(other);
}

SoundType ChannelOutputRegisters::findSoundType(const bool sound, const int volume) noexcept
{
    // Hardware?
    if (volume < PsgValues::hardwareVolumeValue) {
        // Not a hardware sound. Software only or not software either.
        return sound ? SoundType::softwareOnly : SoundType::noSoftwareNoHardware;
    }
    // Hardware.
    return sound ? SoundType::softwareAndHardware : SoundType::hardwareOnly;
}

PsgRegisters ChannelOutputRegisters::toPsgRegisters(const int channelIndex) const noexcept
{
    PsgRegisters psgRegisters;

    psgRegisters.setMixerNoiseState(channelIndex, isNoise());
    psgRegisters.setSoftwarePeriod(channelIndex, softwarePeriod);
    psgRegisters.setVolume(channelIndex, volume);
    psgRegisters.setHardwareEnvelopeAndRetrig(hardwareEnvelope, retrig);
    psgRegisters.setHardwarePeriod(hardwarePeriod);

    auto isSound = false;
    switch (soundType) {
        case SoundType::noSoftwareNoHardware: [[fallthrough]];
        case SoundType::hardwareOnly:
            isSound = false;
            break;
        case SoundType::softwareOnly: [[fallthrough]];
        case SoundType::softwareAndHardware:
            isSound = true;
            break;
    }
    psgRegisters.setMixerSoundState(channelIndex, isSound);

    // SID.
    psgRegisters.setSpecialEffect(channelIndex, specialEffect);

    return psgRegisters;
}

SpecialEffect ChannelOutputRegisters::getSpecialEffect() const noexcept
{
    return specialEffect;
}

}   // namespace arkostracker
