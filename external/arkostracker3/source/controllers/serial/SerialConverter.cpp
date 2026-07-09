#include "SerialConverter.h"

#include "../../utils/BitNumber.h"

namespace arkostracker
{

const unsigned char SerialConverter::initializationTag = 0xffU;          // Tag to declare an Initialization.
const unsigned char SerialConverter::frameTag = 0xfeU;                   // Tag to declare a Frame.
const unsigned char SerialConverter::stopTag = 0xfdU;                    // Tag to declare a Stop Sound.

std::vector<unsigned char> SerialConverter::encodeFrame(const std::vector<PsgRegisters>& psgRegisters) noexcept
{
    std::vector<unsigned char> output;

    // Don't encode any bytes if not needed.
    if (psgRegisters.empty()) {
        return output;
    }

    output.emplace_back(frameTag);

    // Encodes each PSG Register.
    unsigned char psgIndex = 0;
    for (const auto& psgRegister : psgRegisters) {
        // Psg index.
        output.emplace_back(psgIndex);

        // The software LSB period first.
        output.emplace_back(static_cast<unsigned char>(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodLowChannel1))));
        output.emplace_back(static_cast<unsigned char>(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodLowChannel2))));
        output.emplace_back(static_cast<unsigned char>(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodLowChannel3))));

        // Hardware period.
        output.emplace_back(static_cast<unsigned char>(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::hardwarePeriodLow))));
        output.emplace_back(static_cast<unsigned char>(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::hardwarePeriodHigh))));

        // Software period MSB 1 & 2, mixed.
        {
            BitNumber bitNumber;
            bitNumber.injectNumber(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodHighChannel1)), 4);
            bitNumber.injectNumber(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodHighChannel2)), 4);
            output.emplace_back(static_cast<unsigned char>(bitNumber.get()));
        }

        // Volume 1 (4bits only) + Software period MSB 3.
        {
            BitNumber bitNumber;
            bitNumber.injectNumber(psgRegister.getVolume(0), 4);
            bitNumber.injectNumber(psgRegister.getValueFromRegister(static_cast<int>(PsgRegistersIndex::softwarePeriodHighChannel3)), 4);
            output.emplace_back(static_cast<unsigned char>(bitNumber.get()));
        }

        // Volume 2 & 3 (4bits only).
        {
            BitNumber bitNumber;
            bitNumber.injectNumber(psgRegister.getVolume(1), 4);
            bitNumber.injectNumber(psgRegister.getVolume(2), 4);
            output.emplace_back(static_cast<unsigned char>(bitNumber.get()));
        }

        // Noise + hardware volume bits.
        {
            BitNumber bitNumber;
            bitNumber.injectNumber(psgRegister.getNoise(), 5);
            bitNumber.injectBool(psgRegister.isHardwareVolume(0));
            bitNumber.injectBool(psgRegister.isHardwareVolume(1));
            bitNumber.injectBool(psgRegister.isHardwareVolume(2));
            output.emplace_back(static_cast<unsigned char>(bitNumber.get()));
        }

        // Mixer (6b) + retrig?
        {
            BitNumber bitNumber;
            bitNumber.injectNumber(psgRegister.getMixer(), 6);
            bitNumber.injectBool(psgRegister.isRetrig());
            output.emplace_back(static_cast<unsigned char>(bitNumber.get()));
        }

        // R13.
        output.emplace_back(static_cast<unsigned char>(psgRegister.getHardwareEnvelopeAndRetrig().getEnvelope()));

        ++psgIndex;
    }

    // Psg index marking the end.
    output.emplace_back(static_cast<unsigned char>(0xffU));

    return output;
}

std::vector<unsigned char> SerialConverter::encodeStopSound() noexcept
{
    return { stopTag };
}

std::vector<unsigned char> SerialConverter::encodeInitialization(const std::vector<PsgData>& psgData) noexcept
{
    std::vector<unsigned char> output;

    // Don't encode any bytes if not needed.
    if (psgData.empty()) {
        return output;
    }

    output.emplace_back(initializationTag);

    // Encodes the replay frequency, from the first PSG. They should all be the same.
    const auto replayFrequency = static_cast<unsigned int>(psgData.at(0).getReplayFrequency());
    output.emplace_back(static_cast<unsigned char>((replayFrequency >> 0U) & 0xffU));
    output.emplace_back(static_cast<unsigned char>((replayFrequency >> 8U) & 0xffU));

    // How many PSGs?
    output.emplace_back(static_cast<unsigned char>(psgData.size()));

    // Encodes each PSG data.
    for (const auto& psg : psgData) {
        // Encodes the PSG frequency.
        const auto psgFrequency = psg.getPsgFrequency();
        jassert(psgFrequency <= 0xffffffff);
        output.emplace_back(static_cast<unsigned char>((psgFrequency >> 0U) & 0xffU));
        output.emplace_back(static_cast<unsigned char>((psgFrequency >> 8U) & 0xffU));
        output.emplace_back(static_cast<unsigned char>((psgFrequency >> 16U) & 0xffU));
        output.emplace_back(static_cast<unsigned char>((psgFrequency >> 24U) & 0xffU));
    }

    return output;
}

}   // namespace arkostracker
