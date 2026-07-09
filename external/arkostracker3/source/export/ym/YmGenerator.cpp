#include "YmGenerator.h"

#include "../../player/PsgRegistersConverter.h"
#include "../../reader/ym/YmReader.h"
#include "../../utils/StringUtil.h"

namespace arkostracker
{

YmGenerator::YmGenerator(const juce::MemoryBlock& pInputYm, juce::MemoryOutputStream& pOutputStream, const OptionalInt pTargetPsgFrequencyHz) :
        inputYmStream(pInputYm, true),
        outputStream(pOutputStream),
        optionalTargetPsgFrequencyHz(pTargetPsgFrequencyHz)
{
}

bool YmGenerator::convert()
{
    YmReader ymReader(inputYmStream);
    if (!ymReader.checkFormat()) {
        jassertfalse;
        return false;
    }
    if (!ymReader.prepare()) {
        jassertfalse;
        return false;
    }

    constexpr auto psgIndex = 0;

    // This code is more or less a copy/paste from YmExporter, but mixing it would have been a bit messy...

    // Now encodes the header in the "real" buffer.
    // b0 = interleaved? Yes. b1 = signed samples ? --> no. b2 = 4bits? --> no.
    constexpr auto songAttributes = 1;
    const auto digidrumCount = ymReader.getDigidrumCount();
    const auto iterationCount = ymReader.getIterationCount();
    const auto loopCounter = ymReader.getLoopIndex();
    const auto originalPsgFrequency = ymReader.getPsgFrequencyHz(psgIndex);
    const auto replayFrequency = ymReader.getPlayerReplayHz();

    const auto targetPsgFrequencyHz = optionalTargetPsgFrequencyHz.isPresent() ? optionalTargetPsgFrequencyHz.getValue() : originalPsgFrequency;

    outputStream.writeText("YM6!", false, false, nullptr);
    outputStream.writeText("LeOnArD!", false, false, nullptr);
    outputStream.writeIntBigEndian(iterationCount);
    outputStream.writeIntBigEndian(songAttributes);
    outputStream.writeShortBigEndian(static_cast<int16_t>(digidrumCount));
    outputStream.writeIntBigEndian(targetPsgFrequencyHz);
    outputStream.writeShortBigEndian(static_cast<int16_t>(replayFrequency));
    outputStream.writeIntBigEndian(loopCounter);
    outputStream.writeShortBigEndian(0);                        // Padding.

    // Encodes the digidrums.
    encodeDigidrums(ymReader);

    StringUtil::writeNtString(outputStream, ymReader.getTitle());
    StringUtil::writeNtString(outputStream, ymReader.getAuthor());
    StringUtil::writeNtString(outputStream, ymReader.getComments());

    // Encodes the music data.
    auto success = true;
    constexpr auto registerCount = 16;
    auto previousHardwareEnvelope = 16;        // Sentinel value.
    // Multiple passes! This is inefficient because all the operations are performed for all the registers... Who cares?
    for (auto registerIndex = 0; registerIndex < registerCount; ++registerIndex) {
        for (auto iterationIndex = 0; success && (iterationIndex < iterationCount); ++iterationIndex) {
            const auto& [originalPsgRegisters, readSuccess] = ymReader.getRegisters(psgIndex, iterationIndex);
            jassert(readSuccess);
            success = readSuccess;

            // Converts to the target PSG.
            const auto psgRegisters = PsgRegistersConverter::convertPeriods(originalPsgRegisters, originalPsgFrequency, targetPsgFrequencyHz);

            auto value = psgRegisters.getValueFromRegister(registerIndex);
            if (registerIndex == 3) {
                // R3's bit 5/4 indicates on which channel the digidrum is played. Volume regs are not modified, no need to change them.
                // The mask should be %00110000 only, but more bits are kept for TU to work simpler (less changes).
                const auto digidrumBits = static_cast<unsigned int>(originalPsgRegisters.getValueFromRegister(3)) & 0b11110000U;
                value = static_cast<int>((static_cast<unsigned int>(value) & 0b1111U) | digidrumBits);
            } else if (registerIndex == 13) {
                // R13 has a special treatment. Encodes the value if retrig, or different from the previous frame. Else, 255 (=the same).
                const auto encodeValue = (psgRegisters.isRetrig() || (value != previousHardwareEnvelope));
                previousHardwareEnvelope = value;

                value = encodeValue ? value : static_cast<signed char>(255);
            }

            outputStream.writeByte(static_cast<char>(value));
        }
    }

    // Encodes the tail.
    outputStream.writeText("End!", false, false, nullptr);

    // Finish!
    outputStream.flush();

    return true;
}

void YmGenerator::encodeDigidrums(const YmReader& ymReader) const noexcept
{
    for (const auto& digidrum : ymReader.getDigidrums()) {
        // Encodes the header.
        const auto size = digidrum->getLength();
        const auto& sampleData = digidrum->getData();
        outputStream.writeIntBigEndian(size);

        // Encodes the sample data.
        for (auto index = 0; index < size; ++index) {
            const auto sample = sampleData[static_cast<size_t>(index)];
            outputStream.writeByte(sample);
        }
    }
}

}   // namespace arkostracker
