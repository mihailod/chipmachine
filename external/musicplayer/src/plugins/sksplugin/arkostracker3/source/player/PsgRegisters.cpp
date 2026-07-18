#include "PsgRegisters.h"

#include <bitset>

#include <juce_core/juce_core.h>

#include "../utils/PsgValues.h"

namespace arkostracker 
{

HardwareEnvelopeAndRetrig::HardwareEnvelopeAndRetrig(const int r13, const bool newRetrig) noexcept :
        envelope(r13),
        retrig(newRetrig)
{
}

HardwareEnvelopeAndRetrig HardwareEnvelopeAndRetrig::buildDefault() noexcept
{
    return { 8, false };
}

int HardwareEnvelopeAndRetrig::getEnvelope() const noexcept
{
    return envelope;
}

bool HardwareEnvelopeAndRetrig::isRetrig() const noexcept
{
    return retrig;
}

void HardwareEnvelopeAndRetrig::setRetrig(const bool pRetrig) noexcept
{
    retrig = pRetrig;
}


// ===========================================================================

PsgRegisters::PsgRegisters() :
        registers(),
        eventNumber(0),
        retrig(false),
        channelToSpecialEffects()
{
    registers[static_cast<int>(PsgRegistersIndex::mixer)] = 0b111111;       // Closes the sound and noise.
    registers[static_cast<int>(PsgRegistersIndex::hardwareEnvelope)] = PsgValues::minimumHardwareEnvelope;
}

int PsgRegisters::getVolume(const int channelIndex) const noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    return registers[static_cast<size_t>(PsgRegistersIndex::volumeChannel1) + static_cast<size_t>(channelIndex)];
}

bool PsgRegisters::isHardwareVolume(const int channelIndex) const noexcept
{
    const auto volume = getVolume(channelIndex);
    jassert(volume >= 0);
    return ((static_cast<unsigned int>(volume) & 0b10000U) != 0);
}

void PsgRegisters::setVolume(const int channelIndex, const int volume) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));
    jassert((volume >= PsgValues::minimumVolume) && (volume <= PsgValues::maximumVolumeIncludeHardwareVolume));

    registers[static_cast<size_t>(PsgRegistersIndex::volumeChannel1) + static_cast<size_t>(channelIndex)] = static_cast<unsigned char>(volume);
}

int PsgRegisters::getSoftwarePeriod(const int channelIndex) const noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    const auto baseRegister = static_cast<size_t>(PsgRegistersIndex::softwarePeriodLowChannel1) + (static_cast<size_t>(channelIndex) * 2);
    // The useless bits of the MSB are NOT removed: useful for YM.
    return (registers[baseRegister] + (registers[baseRegister + 1] << 8U));
}

void PsgRegisters::setSoftwarePeriod(const int channelIndex, const int period) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));
    jassert((period >= PsgValues::minimumPeriod) && (period <= PsgValues::maximumSoftwarePeriod));      // On CPC, we manage 16 bits numbers, so we won't bother testing for 0xfff which should be the normal value.

    auto baseRegister = static_cast<size_t>(PsgRegistersIndex::softwarePeriodLowChannel1) + (static_cast<size_t>(channelIndex) * 2);
    registers[baseRegister++] = static_cast<unsigned char>(static_cast<unsigned int>(period) & 0xffU);
    // The useless bits of the MSB are NOT removed: useful for YM.
    registers[baseRegister] = static_cast<unsigned char>((static_cast<unsigned int>(period) >> 8U) & 0xffU);
}

int PsgRegisters::getNoise() const noexcept
{
    return registers[static_cast<int>(PsgRegistersIndex::noise)];
}

void PsgRegisters::setNoise(const int noise) noexcept
{
    jassert((noise >= PsgValues::minimumNoise) && (noise <= PsgValues::maximumNoise));

    registers[static_cast<int>(PsgRegistersIndex::noise)] = static_cast<unsigned char>(noise);
}

int PsgRegisters::getMixer() const noexcept
{
    return registers[static_cast<int>(PsgRegistersIndex::mixer)];
}

void PsgRegisters::setMixer(const int mixer) noexcept
{
    jassert((mixer >= PsgValues::minimumMixer) && (mixer <= PsgValues::maximumMixer));

    registers[static_cast<int>(PsgRegistersIndex::mixer)] = static_cast<unsigned char>(mixer);
}

void PsgRegisters::setMixerNoiseState(const int channelIndex, const bool open) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    auto mixer = registers[static_cast<int>(PsgRegistersIndex::mixer)];
    const auto bitRank = channelIndex + 3;

    if (open) {
        // Must reset the bit.
        mixer &= ~(1 << bitRank);
    } else {
        // Must set the bit.
        mixer |= (1 << bitRank);
    }

    registers[static_cast<int>(PsgRegistersIndex::mixer)] = static_cast<unsigned char>(mixer);
}

void PsgRegisters::setMixerSoundState(const int channelIndex, const bool open) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    auto mixer = registers[static_cast<int>(PsgRegistersIndex::mixer)];
    const auto bitRank = channelIndex;

    if (open) {
        // Must reset the bit.
        mixer &= ~(1 << bitRank);
    } else {
        // Must set the bit.
        mixer |= (1 << bitRank);
    }

    registers[static_cast<int>(PsgRegistersIndex::mixer)] = static_cast<unsigned char>(mixer);
}

bool PsgRegisters::getMixerSoundState(const int channelIndex) const noexcept
{
    const auto mixer = getMixer();
    jassert(mixer >= 0);
    const std::bitset<8> bits(static_cast<unsigned int>(mixer));
    return !bits.test(static_cast<size_t>(channelIndex));
}

bool PsgRegisters::getMixerNoiseState(const int channelIndex) const noexcept
{
    const auto mixer = getMixer();
    jassert(mixer >= 0);
    const std::bitset<8> bits(static_cast<unsigned int>(mixer));
    return !bits.test(static_cast<size_t>(channelIndex) + 3);        // Skips the channel bits.
}

int PsgRegisters::getHardwarePeriod() const noexcept
{
    return registers[static_cast<size_t>(PsgRegistersIndex::hardwarePeriodLow)] + (registers[static_cast<size_t>(PsgRegistersIndex::hardwarePeriodHigh)] << 8);
}

void PsgRegisters::setHardwarePeriod(const int hardwarePeriod) noexcept
{
    jassert((hardwarePeriod >= PsgValues::minimumPeriod) && (hardwarePeriod <= PsgValues::maximumHardwarePeriod));

    registers[static_cast<int>(PsgRegistersIndex::hardwarePeriodLow)] = static_cast<int>(static_cast<unsigned int>(hardwarePeriod) & 0xffU);
    registers[static_cast<int>(PsgRegistersIndex::hardwarePeriodHigh)] = static_cast<int>((static_cast<unsigned int>(hardwarePeriod) >> 8U) & 0xffU);
}

HardwareEnvelopeAndRetrig PsgRegisters::getHardwareEnvelopeAndRetrig() const noexcept
{
    return { registers[static_cast<int>(PsgRegistersIndex::hardwareEnvelope)], retrig };
}

void PsgRegisters::setHardwareEnvelopeAndRetrig(const int hardwareEnvelope, const bool newRetrig) noexcept
{
    // YM related value is not accepted (255)!.
    jassert(((hardwareEnvelope >= PsgValues::minimumHardwareEnvelope) && (hardwareEnvelope <= PsgValues::maximumHardwareEnvelope)));

    registers[static_cast<int>(PsgRegistersIndex::hardwareEnvelope)] = static_cast<unsigned char>(hardwareEnvelope);
    retrig = newRetrig;
}

int PsgRegisters::getValueFromRegister(const int registerIndex) const noexcept
{
    jassert((registerIndex >= 0) && (registerIndex < registerCount));
    return registers.at(static_cast<size_t>(registerIndex));
}

void PsgRegisters::setValue(const int registerIndex, const int value) noexcept
{
    jassert((registerIndex >= 0) && (registerIndex < registerCount));
    registers.at(static_cast<size_t>(registerIndex)) = value;
}

void PsgRegisters::setEventNumber(const int newEventNumber) noexcept
{
    eventNumber = newEventNumber;
}

int PsgRegisters::getEventNumber() const noexcept
{
    return eventNumber;
}

bool PsgRegisters::isRetrig() const
{
    return retrig;
}

void PsgRegisters::setRetrig(const bool newRetrig)
{
    retrig = newRetrig;
}

void PsgRegisters::muteChannelFromMuteStates(const unsigned char muteStates) noexcept
{
    if ((muteStates & 0b001U) != 0) {
        muteChannel(0);
    }
    if ((muteStates & 0b010U) != 0) {
        muteChannel(1);
    }
    if ((muteStates & 0b100U) != 0) {
        muteChannel(2);
    }
}

void PsgRegisters::muteChannel(const int channelIndex) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    setVolume(channelIndex, 0);
    setMixerNoiseState(channelIndex, false);
    setMixerSoundState(channelIndex, false);
}

void PsgRegisters::clear() noexcept
{
    // Simple to copy the values from another instance.
    const auto emptyRegisters = PsgRegisters();

    registers = emptyRegisters.registers;
    eventNumber = emptyRegisters.eventNumber;
    retrig = emptyRegisters.retrig;
}

void PsgRegisters::correct() noexcept
{
    // MSB of software periods.
    correctValue(PsgRegistersIndex::softwarePeriodHighChannel1, PsgValues::maskSoftwarePeriodMSB);
    correctValue(PsgRegistersIndex::softwarePeriodHighChannel2, PsgValues::maskSoftwarePeriodMSB);
    correctValue(PsgRegistersIndex::softwarePeriodHighChannel3, PsgValues::maskSoftwarePeriodMSB);

    correctValue(PsgRegistersIndex::noise, PsgValues::maskNoise);
}

void PsgRegisters::setSpecialEffect(const int channelIndex, const SpecialEffect& newSpecialEffects) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));
    channelToSpecialEffects[channelIndex] = newSpecialEffects;
}

const SpecialEffect& PsgRegisters::getSpecialEffects(const int channelIndex) const noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < PsgValues::channelCountPerPsg));

    static const auto none = SpecialEffect();

    if (const auto it = channelToSpecialEffects.find(channelIndex); it != channelToSpecialEffects.cend()) {
        return it->second;
    }

    return none;
}

void PsgRegisters::correctValue(const PsgRegistersIndex psgRegistersIndex, const unsigned int mask) noexcept
{
    const auto reg = static_cast<int>(psgRegistersIndex);

    const auto value = static_cast<unsigned int>(getValueFromRegister(reg));
    setValue(reg, static_cast<int>(value & mask));
}

bool PsgRegisters::operator==(const PsgRegisters& rhs) const
{
    return registers == rhs.registers
           && eventNumber == rhs.eventNumber
           && retrig == rhs.retrig
           && channelToSpecialEffects == rhs.channelToSpecialEffects
    ;
}

bool PsgRegisters::operator!=(const PsgRegisters& rhs) const
{
    return !(*this == rhs);

}

}   // namespace arkostracker
