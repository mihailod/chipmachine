#include "VgmReader.h"

#include "../../song/psg/PsgFrequency.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StreamUtil.h"

namespace arkostracker 
{

const int VgmReader::maximumPsgCount = 2;           // VGM limit.

VgmReader::VgmReader(juce::InputStream& pInputStream) noexcept:
        StreamedMusicReader(pInputStream),
        psgCount(),
        psgFrequencyHz(),
        psgType(PsgType::ay),
        playerReplayHz(PsgFrequency::defaultReplayFrequencyHz),
        loopIndex(),
        title(),
        author(),

        vgmDataPosition(),
        vgmLoopPosition(),
        currentFramePsgRegisters(),
        waitSampleToOccurrence(),

        psgToPsgRegisters(),
        psgToPreviousHardwareEnvelope()
{
}


// StreamedMusicReader method implementations.
// ===================================================

bool VgmReader::checkFormat() noexcept
{
    // Reads the tag.
    auto& stream = getInputStream();
    auto success = stream.setPosition(0);
    success = success && StreamUtil::readAndCompareString(stream, "Vgm ");
    if (!success) {
        return false;
    }

    // Version? 1.51 is required, it contains the AY handling.
    if (const auto version = readVersion(stream); version < 151) {
        jassertfalse;
        return false;
    }

    // Any YM/AY chip?
    success = stream.setPosition(0x74);
    success = success && (stream.readInt() > 0);

    return success;
}

int VgmReader::readVersion(juce::InputStream& inputStream) noexcept
{
    auto success = inputStream.setPosition(0x8);
    if (!success) {
        return 0;
    }

    auto readBcd = inputStream.readInt();

    auto result = 0;
    auto digitMultiplier = 1;
    while (readBcd != 0) {
        auto digit = readBcd & 0b1111;
        result += digit * digitMultiplier;

        readBcd >>= 4U;
        digitMultiplier *= 10;
    }

    return result;
}

bool VgmReader::prepare() noexcept
{
    auto& stream = getInputStream();

    // Reads the offset to the VGM data. This exists only for >= v1.50, else the offset is 0x40.
    auto success = stream.setPosition(0x34);
    vgmDataPosition = stream.readInt() + 0x34;
    if (!success || vgmDataPosition == 0) {
        jassertfalse;
        return false;
    }

    success = stream.setPosition(0x1c);
    vgmLoopPosition = stream.readInt() + 0x1c;

    // Gets the YM/AY PSG frequency.
    success = success && stream.setPosition(0x74);
    const auto rawPsgFrequencies = stream.readInt();          // May be dual of bit 30 is set.
    psgCount = (rawPsgFrequencies & 0x40000000) > 0 ? 2 : 1;
    psgFrequencyHz = rawPsgFrequencies & (0x40000000 - 1);
    success = success && (psgFrequencyHz > 0);

    const auto chipType = stream.readByte();        // 0x78.
    psgType = (chipType <= 0x03) ? PsgType::ay : PsgType::ym;

    psgToPsgRegisters.clear();
    psgToPreviousHardwareEnvelope.clear();
    currentFramePsgRegisters.clear();
    PsgRegisters defaultPsgRegisters;
    defaultPsgRegisters.setHardwareEnvelopeAndRetrig(PsgValues::minimumHardwareEnvelope, false);    // To avoid assertions.
    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        psgToPsgRegisters.emplace_back();
        psgToPreviousHardwareEnvelope.emplace_back(static_cast<unsigned char>(PsgValues::minimumHardwareEnvelope));
        // Creates default PSG registers. Each frame will modify it, as diffs are encoded in VGM (potentially, especially for R13).
        currentFramePsgRegisters.push_back(defaultPsgRegisters);
    }

    parseGd3();

    // Parses the whole VGM data and build the PSG registers.
    return success && parseVgmData();
}

int VgmReader::getPsgCount() const noexcept
{
    jassert(psgCount > 0);
    return psgCount;
}

PsgType VgmReader::getPsgType(const int psgIndex) const noexcept
{
    jassert(psgIndex < maximumPsgCount); (void)psgIndex;
    return psgType;
}

int VgmReader::getIterationCount() const noexcept
{
    jassert(!psgToPsgRegisters.empty());
    return static_cast<int>(psgToPsgRegisters.at(0U).size());
}

int VgmReader::getLoopIndex() const noexcept
{
    return loopIndex;
}

int VgmReader::getPsgFrequencyHz(const int psgIndex) const
{
    jassert(psgIndex < maximumPsgCount); (void)psgIndex;
    return psgFrequencyHz;
}

float VgmReader::getPlayerReplayHz() const
{
    return playerReplayHz;
}

juce::String VgmReader::getTitle() const
{
    return title;
}

juce::String VgmReader::getAuthor() const
{
    return author;
}

SamplePlayInfo VgmReader::getSamplePlayInfo(int /*sampleIndex*/, int /*sampleReplayFrequencyHz*/) const
{
    return { };
}

std::pair<PsgRegisters, bool> VgmReader::getRegisters(const int psgIndex, const int iterationIndex) const noexcept
{
    if (psgIndex >= psgCount) {
        jassertfalse;           // Invalid PSG index!
        return { };
    }
    if (iterationIndex >= getIterationCount()) {
        jassertfalse;           // Invalid PSG index!
        return { };
    }

    return { psgToPsgRegisters.at(static_cast<size_t>(psgIndex)).at(static_cast<size_t>(iterationIndex)), true};
}

bool VgmReader::areDigidrumsAllowed() const
{
    return false;
}

int VgmReader::getDigidrumCount() const
{
    return 0;
}

juce::String VgmReader::getComments() const
{
    return juce::String();
}

std::vector<std::shared_ptr<Sample>> VgmReader::getDigidrums() const noexcept
{
    return { };
}


// ======================================================

bool VgmReader::parseVgmData() noexcept // NOLINT(*-function-cognitive-complexity)
{
    static const auto oneOperandIgnoredCommand = std::unordered_set{
            0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
            0x4f,
            0x50,
    };
    static const auto twoOperandsIgnoredCommand = std::unordered_set{
            0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e,
            0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
            0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
            0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbe, 0xbf,
    };

    static const auto threeOperandsIgnoredCommand = std::unordered_set{
            0xc0, 0xc1, 0xc2, 0xc3, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
            0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    };

    static const auto fourOperandsIgnoredCommand = std::unordered_set{
            0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
            0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
            0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    };

    auto& stream = getInputStream();

    jassert(vgmDataPosition > 0);
    auto success = stream.setPosition(vgmDataPosition);

    auto bytesToSkip = 0;       // Default. This does NOT include the command, which has already been read!

    auto mustContinue = true;

    while (mustContinue && !stream.isExhausted()) {
        // Reached the loop position?
        if (stream.getPosition() == vgmLoopPosition) {
            loopIndex = getIterationCount();
        }

        // Most commands will be skipped.
        const auto commandByte = static_cast<unsigned char>(stream.readByte());

        // Any command that is ignored and which operands must be skipped?
        if (oneOperandIgnoredCommand.find(commandByte) != oneOperandIgnoredCommand.cend()) {
            bytesToSkip = 1;
        } else if (twoOperandsIgnoredCommand.find(commandByte) != twoOperandsIgnoredCommand.cend()) {
            bytesToSkip = 2;
        } else if (threeOperandsIgnoredCommand.find(commandByte) != threeOperandsIgnoredCommand.cend()) {
            bytesToSkip = 3;
        } else if (fourOperandsIgnoredCommand.find(commandByte) != fourOperandsIgnoredCommand.cend()) {
            bytesToSkip = 4;
        }

        // Skips bytes and continues to the next command if they are present.
        if (bytesToSkip > 0) {
            stream.setPosition(stream.getPosition() + bytesToSkip);
            continue;
        }

        switch (commandByte) {
            case 0x61:          //  Wait NN NN.
                onWaitFound(static_cast<unsigned int>(stream.readShort()));
                break;
            case 0x62:          //  Wait 60th of second.
                onWaitFound(735U);
                break;
            case 0x63:          //  Wait 50th of second.
                onWaitFound(882U);
                break;
            case 0x70: [[fallthrough]];     // Wait 0x7x, ignored, too small.
            case 0x71: [[fallthrough]];
            case 0x72: [[fallthrough]];
            case 0x73: [[fallthrough]];
            case 0x74: [[fallthrough]];
            case 0x75: [[fallthrough]];
            case 0x76: [[fallthrough]];
            case 0x77: [[fallthrough]];
            case 0x78: [[fallthrough]];
            case 0x79: [[fallthrough]];
            case 0x7a: [[fallthrough]];
            case 0x7b: [[fallthrough]];
            case 0x7c: [[fallthrough]];
            case 0x7d: [[fallthrough]];
            case 0x7e: [[fallthrough]];
            case 0x7f:
                break;
            case 0x66:          // End of sound!
                mustContinue = false;
                break;
            case 0x90: [[fallthrough]];          // DAC stream control write.
            case 0x91: [[fallthrough]];
            case 0x92: [[fallthrough]];
            case 0x93: [[fallthrough]];
            case 0x94: [[fallthrough]];
            case 0x95:
                jassertfalse;
                break;
            case 0xa0: {
                const auto rawRegister = static_cast<unsigned char>(stream.readByte());
                const auto reg = rawRegister & 0x7f;        // Bit 7 is set for the second PSG.
                const auto isFirstPsg = (rawRegister & 0x80) == 0;
                const auto value = static_cast<unsigned char>(stream.readByte());

                onPsgRegisterFound(reg, value, isFirstPsg);
                break;
            }
            default:
                DBG("Unknown command: " + juce::String(commandByte));
                jassertfalse;
                break;
        }
    }

    jassert(!mustContinue);         // Exhausted before the end command is read?

    // Tries to get the "best" replay in hz.
    const auto sortedItemsToOccurrence = waitSampleToOccurrence.generateSortedMostUsedItemsAndOccurrence();
    auto foundValid = false;
    auto foundPlayerReplayHz = PsgFrequency::defaultReplayFrequencyHz;
    auto it = sortedItemsToOccurrence.cbegin();
    while (!foundValid && (it != sortedItemsToOccurrence.cend())) {
        const auto waitSample = it->first;
        foundPlayerReplayHz = 44100.0F / static_cast<float>(waitSample);

        // Many VGM are managed like "samples", so the found replay are actually way to high. Cap these, but this is not ideal.
        foundValid = (foundPlayerReplayHz >= 12.0F) && (foundPlayerReplayHz <= 200.0F);

        ++it;
    }

    jassert(foundValid);
    playerReplayHz = foundPlayerReplayHz;

    return success;
}

void VgmReader::onPsgRegisterFound(const int reg, const unsigned char value, const bool isFirstPsg) noexcept
{
    jassert(psgCount > 0);

    const auto psgIndex = static_cast<size_t>(isFirstPsg ? 0 : 1);
    auto& psgRegisters = currentFramePsgRegisters.at(psgIndex);

    // Overwrites the register normally.
    psgRegisters.setValue(reg, value);

    if (reg == static_cast<int>(PsgRegistersIndex::hardwareEnvelope)) {
        // If present, stores the new envelope in a special container, to be used at the end of the frame.
        const auto newEnvelope = PsgValues::convertEnvelopeCurveToAt(value);        // Security.
        psgToPreviousHardwareEnvelope.at(psgIndex) = static_cast<unsigned char>(newEnvelope);
    }
}

void VgmReader::onWaitFound(const unsigned int sampleCount) noexcept
{
    if (sampleCount < 30U) {
        jassertfalse;           // Ignores the wait that are too near.
        return;
    }

    // End of the frame.
    // Stores the wait, to determine, at the end, what is the most probably used wait count.
    waitSampleToOccurrence.addItem(sampleCount);

    // Stores the pending PSG Registers.
    if (static_cast<int>(currentFramePsgRegisters.size()) == psgCount) {
        jassert(psgToPsgRegisters.size() == static_cast<size_t>(psgCount));
        size_t psgIndex = 0;
        for (auto& psgRegister : currentFramePsgRegisters) {
            // Before encoding, treats the R13.
            const auto foundEnvelope = psgToPreviousHardwareEnvelope.at(psgIndex);
            if (foundEnvelope.isPresent() && psgRegister.getHardwareEnvelopeAndRetrig().getEnvelope() == foundEnvelope.getValue()) {
                // Forces a retrig.
                psgRegister.setRetrig(true);
            }

            psgToPsgRegisters.at(psgIndex).push_back(psgRegister);

            // Removes the retrig for the next frame.
            psgRegister.setRetrig(false);

            ++psgIndex;
        }
    } else {
        jassertfalse;           // Nothing to write??
    }

    // Clears the previously found R13.
    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        psgToPreviousHardwareEnvelope.at(static_cast<size_t>(psgIndex)) = OptionalValue<unsigned char>();
    }
}

void VgmReader::parseGd3() noexcept
{
    auto& stream = getInputStream();

    auto success = stream.setPosition(0x14);
    const auto gd3RawOffset = stream.readInt();
    if (!success || (gd3RawOffset == 0)) {
        jassert(success);       // Cannot reach?
        return;
    }

    success = stream.setPosition(gd3RawOffset + 0x14);
    if (!success) {
        return;
    }

    if (!StreamUtil::readAndCompareString(stream, "Gd3 ")) {
        jassertfalse;       // Wrong GD3 header?
        return;
    }

    // Skips the version and the size.
    stream.skipNextBytes(4);
    const auto dataSize = stream.readInt();
    juce::MemoryBlock memoryBlock;
    stream.readIntoMemoryBlock(memoryBlock, dataSize);

    // Reads the UTF-16 Strings.
    std::vector<juce::String> strings;
    auto dataOffset = 0;
    for (auto stringIndex = 0; stringIndex < 11; ++stringIndex) {       // 11 strings are encoded.
        const juce::CharPointer_UTF16 stringPointer(static_cast<juce::CharPointer_UTF16::CharType*>(memoryBlock.getData()) + dataOffset);
        const juce::String readString(stringPointer);
        strings.push_back(readString);

        dataOffset += static_cast<int>(stringPointer.length() + 1);     // Skips the added 0.
    }

    const auto gameName = strings.at(2U);
    const auto trackName = strings.at(0U);
    title = gameName.isNotEmpty() ? gameName + (trackName.isNotEmpty() ? (" - " + trackName) : juce::String()) : trackName;
    author = strings.at(6U);
}


}   // namespace arkostracker

