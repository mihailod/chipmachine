#include "ModExporter.h"

#include <utility>

#include "../../business/instrument/SampleLoader.h"
#include "../../import/mod/ModPeriods.h"
#include "../../utils/NoteUtil.h"
#include "../../utils/NumberUtil.h"
#include "../instrumentAsWav/InstrumentAsWavExporter.h"
#include "../instrumentAsWav/InstrumentRenderer.h"

namespace arkostracker
{
ModExporter::ModExporter(std::shared_ptr<Song> pSong, Id pSubsongId, const int pMinimumDurationMs,
                         juce::OutputStream& pOutputStream) noexcept :
        song(std::move(pSong)),
        subsongId(std::move(pSubsongId)),
        psg(song->getSubsongPsgs(subsongId).at(0)), // Simplification, but do we need more?
        minimumDurationMs(pMinimumDurationMs),
        outputStream(pOutputStream),
        instrumentSampleData(),
        patternIndexToHeight(),
        modChannelCount(),
        lastPatternIndex(),
        firstPositionPatternIndex()
{
}


// Task method implementations.
// ===============================

std::pair<bool, std::unique_ptr<bool> > ModExporter::performTask() noexcept
{
    encodePaddedText(song->getTitle(), maximumLengthSongName);
    auto success = encodeSampleHeaderAndStoreData();
    success = success && encodeLinker();
    if (success) {
        encodePatterns();
        encodeSampleData();
    }

    return { success, std::make_unique<bool>(success) };
}

// ===============================

void ModExporter::encodePaddedText(const juce::String& inputText, const int maximumLength) const noexcept
{
    auto text = inputText;
    if (inputText.length() > maximumLength) {
        text = text.substring(0, maximumLength);
    }
    outputStream.writeText(text, false, false, nullptr);
    outputStream.writeRepeatedByte(0, static_cast<size_t>(maximumLength - text.length())); // Padding with 0.
}

bool ModExporter::encodeSampleHeaderAndStoreData() noexcept
{
    const auto instrumentCountInSong = std::min(song->getInstrumentCount() - 1, maximumInstrumentCount); // Ignores the Empty.

    for (auto instrumentIndex = 1; instrumentIndex <= instrumentCountInSong; ++instrumentIndex) {
        const auto instrumentId = song->getInstrumentId(instrumentIndex);
        if (instrumentId.isAbsent()) {
            return false;
        }
        publishTaskProgress(instrumentIndex - 1, instrumentCountInSong);
        if (isCanceled()) {
            return false;
        }

        const auto replayFrequencyHz = song->getReplayFrequencyHz(subsongId);
        const auto psgSampleRate = PsgFrequency::defaultModSamplePlayerFrequencyHz;

        // Renders the instrument. It is made of floats.
        juce::MemoryOutputStream floatSampleOutputStream;
        const auto sidPlayerCapability = song->getSubsongMetadata(subsongId).getSidPlayerCapability();
        InstrumentRenderer instrumentRenderer(song, psg, sidPlayerCapability);
        instrumentRenderer.renderNote(PsgValues::digidrumNote, instrumentId.getValue(),
                                      floatSampleOutputStream, replayFrequencyHz, psgSampleRate, minimumDurationMs, sampleMaximumSize);

        auto sampleFloatsMemoryBlock = floatSampleOutputStream.getMemoryBlock();
        juce::MemoryInputStream mis(sampleFloatsMemoryBlock, false);

        const auto sampleCount = static_cast<int>(mis.getTotalLength() / 4); // Because 32 bits.

        // Puts the sample from floats to signed char, and stores it for later.
        auto finalMemoryBlock = std::make_unique<juce::MemoryBlock>(sampleCount, false);
        auto sampleIndex = 0;
        while (!mis.isExhausted()) {
            const auto value = mis.readFloat();
            const auto signedValue = SampleLoader::floatToSignedChar(value) + 128;

            finalMemoryBlock->operator[](sampleIndex) = static_cast<char>(signedValue); // Forced to char for ARM, else error.

            ++sampleIndex;
        }
        instrumentSampleData.push_back(std::move(finalMemoryBlock));

        song->performOnConstInstrumentFromIndex(instrumentIndex, [&](const Instrument& instrument) {
            // Encodes the name, 0 terminated.
            encodePaddedText(instrument.getName(), maximumLengthInstrumentName);
            outputStream.writeShortBigEndian(static_cast<int16_t>((sampleCount + 1) / 2)); // Sample length in words, plus one word for the repeat info.
            outputStream.writeByte(0); // Finetune.
            outputStream.writeByte(64); // Volume.
            outputStream.writeShortBigEndian(0); // Loop start.
            outputStream.writeShortBigEndian(0); // Loop length.
        });
    }

    // Encodes the "empty" instruments at the end.
    auto remainingInstruments = maximumInstrumentCount - instrumentCountInSong;
    while (remainingInstruments > 0) {
        outputStream.writeRepeatedByte(0, maximumLengthInstrumentName);
        outputStream.writeShortBigEndian(0); // Sample length in words.
        outputStream.writeByte(0); // Finetune.
        outputStream.writeByte(0); // Volume.
        outputStream.writeShortBigEndian(0); // Loop start.
        outputStream.writeShortBigEndian(0); // Loop length.

        --remainingInstruments;
    }

    return true;
}

bool ModExporter::encodeLinker() noexcept
{
    modChannelCount = 0;
    patternIndexToHeight.clear();

    song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto loop = subsong.getLoop();
        const auto positionCount = std::min(maximumPositionCount, loop.getEndIndex() + 1);

        // Encodes the Linker.
        outputStream.writeByte(static_cast<char>(positionCount)); // 1 to 128. Forced to char for ARM, else error.
        outputStream.writeByte(127); // Historical byte.
        for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
            const auto& position = subsong.getPositionRef(positionIndex);
            const auto patternIndex = position.getPatternIndex();
            const auto positionHeight = position.getHeight();
            lastPatternIndex = std::max(lastPatternIndex, patternIndex);
            outputStream.writeByte(static_cast<char>(patternIndex)); // Technically, only 64 patterns are available. No check here. Forced to char for ARM, else error.

            // Considers the first pattern sets its height.
            patternIndexToHeight.insert({ patternIndex, positionHeight });

            // Stores the first pattern index, so that is can be set the speed.
            if (positionIndex == 0) {
                firstPositionPatternIndex = patternIndex;
            }
        }
        outputStream.writeRepeatedByte(0, static_cast<size_t>(128 - positionCount)); // Padding for the 128 linker area.

        // Encodes the marker, also indicating the channel count.
        modChannelCount = NumberUtil::correctNumber(subsong.getChannelCount(), 4, 99);
        juce::String marker;
        switch (modChannelCount) {
            case 4: // Low limit is checked above.
                modChannelCount = 4;
                marker = "4CHN";
                break;
            case 5:
            case 6:
                modChannelCount = 6;
                marker = "6CHN";
                break;
            case 7:
            case 8:
                modChannelCount = 8;
                marker = "8CHN";
                break;
            case 9:
                modChannelCount = 9;
                marker = "9CHN"; // Seems to be known by OpenMPT.
                break;
            default:
                marker = juce::String(modChannelCount).paddedLeft('0', 2) + "CH"; // May not be standard, but is somehow documented...
                break;
        }
        jassert(marker.length() == markerSize);
        encodePaddedText(marker, markerSize);
    });

    return (modChannelCount > 0);
}

void ModExporter::encodePatterns() noexcept
{
    for (auto patternIndex = 0; (patternIndex <= lastPatternIndex); ++patternIndex) {
        encodePattern(patternIndex);
    }
}

void ModExporter::encodePattern(const int patternIndex) noexcept
{
    // The matrix may be larger than what we actually have.
    juce::MemoryBlock patternData(static_cast<size_t>(modChannelCount * modPatternHeight * encodedCellSize), true);

    std::unordered_map<int, int> cellIndexToSpeed;

    song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto pattern = subsong.getPatternFromIndex(patternIndex);
        const auto channelCountInPattern = std::min(pattern.getChannelCount(), modChannelCount); // Make sure we don't go over.

        // What is the height of this pattern? Ignored if non-default or absent.
        OptionalInt patternHeightToEncode = { };
        if (const auto it = patternIndexToHeight.find(patternIndex); it != patternIndexToHeight.cend()) {
            if (const auto patternHeight = it->second; patternHeight < modPatternHeight) {
                patternHeightToEncode = it->second;
            }
        }

        // Builds the speed map.
        const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);
        const auto& speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);

        for (auto cellIndex = 0; cellIndex < modPatternHeight; ++cellIndex) {
            if (const auto& speedCell = speedTrack.getCell(cellIndex); !speedCell.isEmpty()) {
                const auto speed = speedCell.getValue();
                cellIndexToSpeed.insert({ cellIndex, speed });
            }
        }

        // If it is the pattern starting the song, a speed is encoded.
        if (patternIndex == firstPositionPatternIndex) {
            const auto initialSpeed = subsong.getMetadata().getInitialSpeed();
            cellIndexToSpeed.insert({ 0, initialSpeed });       // Not inserted if the key is already present, which suits us.
        }

        for (auto channelIndex = 0; channelIndex < channelCountInPattern; ++channelIndex) {
            const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);
            const auto& track = subsong.getTrackRefFromIndex(trackIndex);

            // Parses each channel.
            for (auto cellIndex = 0; cellIndex < modPatternHeight; ++cellIndex) {
                const auto cell = track.getCell(cellIndex);

                auto instrumentIndex = 0U;
                auto samplePeriod = 0U;
                auto effectNumber = 0U;
                auto effectValue = 0U;
                if (cell.isRst()) {
                    // No better way to have a RST than a volume at 0.
                    effectNumber = effectNumberVolume;
                    effectValue = 0U;
                } else {
                    if (cell.isNote()) {
                        samplePeriod = static_cast<unsigned int>(getSamplePeriod(cell.getNote().getValue().getNote()));
                    }
                    if (cell.isInstrument()) {      // Legato possible.
                        instrumentIndex = static_cast<unsigned int>(cell.getInstrument().getValue());
                    }
                }

                // Effects?
                const auto& effects = cell.getEffects();
                if (const auto volume = effects.find(Effect::volume); volume.isPresent()) {
                    effectNumber = effectNumberVolume;
                    effectValue = getEncodedVolume(volume.getValue().getEffectLogicalValue());
                } else if (const auto reset = effects.find(Effect::reset); reset.isPresent()) {
                    effectNumber = effectNumberVolume;
                    effectValue = getEncodedVolume(PsgValues::maximumVolumeNoHard - reset.getValue().getEffectLogicalValue());
                }

                // Encodes the pattern break/speed if we can. If not, we may have more luck on the next channel.
                if (effectNumber == 0) {
                    if (patternHeightToEncode.isPresent() && (cellIndex == (patternHeightToEncode.getValue() - 1))) {
                        effectNumber = effectNumberPatternBreak;
                        effectValue = 0;
                        patternHeightToEncode = { };        // Consumed.
                    } else {
                        // Speed?
                        if (auto it = cellIndexToSpeed.find(cellIndex); it != cellIndexToSpeed.cend()) {
                            effectNumber = effectNumberSpeed;
                            effectValue = static_cast<unsigned int>(NumberUtil::correctNumber(it->second, minimumSpeed, maximumSpeed));
                            cellIndexToSpeed.erase(cellIndex);       // Consumed.
                        }
                    }
                }

                // Encodes the cell.
                encodeCell(patternData, channelIndex, cellIndex, samplePeriod, instrumentIndex, effectNumber, effectValue);
            }
        }
    });

    // Encodes the pattern raw bytes.
    outputStream.write(patternData.getData(), patternData.getSize());
}

int ModExporter::getSamplePeriod(const int note) noexcept
{
    const auto [noteInOctave, octave] = NoteUtil::getNoteInOctaveAndOctave(note);
    // Changes the octave (shifts it down, as most PSG sounds will be high, plus it makes the samples sound right).
    const auto normalizedOctave = NumberUtil::correctNumber(octave - 4, 0, ModPeriods::amigaPeriodCount / 12);

    const auto finalNote = NoteUtil::getNote(noteInOctave, normalizedOctave);

    return ModPeriods::amigaPeriods.at(static_cast<size_t>(finalNote));
}

void ModExporter::encodeSampleData() const noexcept
{
    for (const auto& sampleData : instrumentSampleData) {
        const auto originalSize = sampleData->getSize();
        outputStream.writeShortBigEndian(0); // First word is the "repeat" information.

        for (size_t index = 0U; index < originalSize; ++index) {
            outputStream.writeByte(sampleData->operator[](index));
        }

        // Add padding, as the size is encoded by words.
        if ((originalSize & 0b1U) != 0) {
            outputStream.writeByte(0);
        }
    }
}

unsigned int ModExporter::getEncodedVolume(const int atVolume) noexcept
{
    return static_cast<unsigned int>(atVolume * static_cast<double>(maximumVolumeInTrack) / PsgValues::maximumVolumeIncludeHardwareVolume);
}

void ModExporter::encodeCell(juce::MemoryBlock& patternData, const int channelIndex, const int cellIndex,
                             const unsigned int samplePeriod, const unsigned int instrumentIndex, const unsigned int effectNumber, const unsigned int effectValue) const noexcept
{
    const auto byte1 = (instrumentIndex & 0b11110000U) | ((samplePeriod >> 8U) & 0b1111U);
    const auto byte2 = samplePeriod & 0b11111111U;
    const auto byte3 = ((instrumentIndex & 0b1111U) << 4U) | (effectNumber & 0b1111U);
    const auto byte4 = effectValue & 0b11111111U;

    const auto offsetInPatternData =
                    (modChannelCount * cellIndex * encodedCellSize) + // Reaches the line.
                    (channelIndex * encodedCellSize); // Reaches the channel.

    jassert(offsetInPatternData < static_cast<int>(patternData.getSize()));
    patternData[offsetInPatternData + 0] = static_cast<char>(byte1); // Forced to char for ARM, else error.
    patternData[offsetInPatternData + 1] = static_cast<char>(byte2);
    patternData[offsetInPatternData + 2] = static_cast<char>(byte3);
    patternData[offsetInPatternData + 3] = static_cast<char>(byte4);
}

} // namespace arkostracker
