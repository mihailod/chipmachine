#include "InstrumentReader.h"

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../utils/MemoryBlockUtil.h"

namespace arkostracker 
{

void InstrumentReader::readInstruments(SongBuilder& builder, const juce::MemoryBlock& memoryData, juce::int64& offset) noexcept
{
    bool success;           // NOLINT(*-init-variables)

    // Reads the next Instrument. There might be holes. But the SongValidator will take care of that.
    auto instrumentIndex = MemoryBlockUtil::extractUnsignedWord(memoryData, offset, success);
    offset += 2;

    while (instrumentIndex < 0xffff) {          // 0xffff marks the end of the Instrument section.
        // Reads the instrument header.
        builder.startNewPsgInstrument(instrumentIndex);
        offset += 4;        // Skips size and loop address.
        const auto speed = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryData, offset++, success));
        builder.setCurrentPsgInstrumentSpeed(speed);
        const auto instrumentRetrig = (MemoryBlockUtil::extractUnsignedChar(memoryData, offset++, success) != 0U);
        builder.setCurrentPsgInstrumentRetrig(instrumentRetrig);
        const auto endIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryData, offset++, success));
        const auto loopStartIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryData, offset++, success));
        const auto isLooping = (MemoryBlockUtil::extractUnsignedChar(memoryData, offset++, success) != 0U);
        builder.setCurrentPsgInstrumentMainLoop(loopStartIndex, endIndex, isLooping);
        const auto instrumentName = MemoryBlockUtil::extractString(memoryData, offset, sizeInstrumentName, success);
        builder.setCurrentInstrumentName(instrumentName);
        offset += sizeInstrumentName;

        if (!success) {
            builder.addError("Error while reading the PSG instrument " + juce::String(instrumentIndex));
            return;
        }

        // Reads all the Instrument Cells.
        for (auto instrumentCellIndex = 0; instrumentCellIndex <= endIndex; ++instrumentCellIndex) {
            readInstrumentCells(builder, memoryData, offset, instrumentIndex);
        }

        builder.finishCurrentInstrument();

        // Reads the next Instrument.
        instrumentIndex = MemoryBlockUtil::extractUnsignedWord(memoryData, offset, success);
        offset += 2;
    }
}

void InstrumentReader::readInstrumentCells(SongBuilder& songBuilder, const juce::MemoryBlock& memoryBlock, juce::int64& offset, const int instrumentIndex) noexcept
{
    auto success = true;

    songBuilder.startNewPsgInstrumentCell();

    const auto firstByte = MemoryBlockUtil::extractUnsignedChar(memoryBlock, offset++, success);
    if (!success) {
        songBuilder.addError("Error while reading the first byte of the PSG instrument " + juce::String(instrumentIndex));
        return;
    }
    if ((firstByte & 0x80U) == 0U) {                  // Tests bit 7. 1 means Hardware sound.
        readSoftwareCell(songBuilder, memoryBlock, offset, firstByte, instrumentIndex);
    } else {
        readHardwareCell(songBuilder, memoryBlock, offset, firstByte, instrumentIndex);
    }

    songBuilder.finishCurrentPsgInstrumentCell();
}

void InstrumentReader::readSoftwareCell(SongBuilder& builder, const juce::MemoryBlock& memoryBlock, juce::int64& offset,
                                        const unsigned int firstByte, const int instrumentIndex) noexcept
{
    auto success = true;

    auto isManualPeriod = false;
    auto isSound = true;
    if ((firstByte & 0x10U) == 0U) {              // If Volume (b3-b0) and Noise? (b4) = 0, secondByte must not be read.
        // No noise or second byte needed.
        if ((firstByte & 0xfU) == 0U) {
            // No volume, no noise, no sound. End of this Cell!
            builder.setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::noSoftNoHard);
        } else {
            // Volume, no noise, and sound. End of this Cell!
            builder.setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::softOnly);
            builder.setCurrentPsgInstrumentCellVolume(static_cast<int>(firstByte & 0xfU));
        }
    } else {
        // Not a simple sound (not a volume + sound only).
        const auto secondByte = MemoryBlockUtil::extractUnsignedChar(memoryBlock, offset++, success);
        // Noise? If (b4-b0)=0, the noise is not present.
        const auto noise = (secondByte & 0x1fU);
        const auto isNoise = (noise != 0U);
        if (isNoise) {
            builder.setCurrentPsgInstrumentCellNoise(static_cast<int>(noise));
        }
        isSound = (secondByte & 0x20U) != 0U;
        auto volume = 0U;
        if (isSound || isNoise) {
            volume = (firstByte & 0xfU);         // Reads the Volume only if there's Sound and/or Noise.
        }
        builder.setCurrentPsgInstrumentCellVolume(static_cast<int>(volume));

        isManualPeriod = (secondByte & 0x40U) != 0U;
    }

    if (isManualPeriod) {
        // The two next bytes are the manual period.
        auto manualPeriod = MemoryBlockUtil::extractUnsignedWord(memoryBlock, offset, success);
        offset += 2;
        builder.setCurrentPsgInstrumentCellPrimaryPeriod(manualPeriod);
    } else {
        // If no manual frequency, the arpeggio and pitch may be present.
        builder.setCurrentPsgInstrumentCellPrimaryPeriod(0);       // "auto".
        if ((firstByte & 0x20U) != 0U) {
            // Arpeggio.
            auto arpeggio = MemoryBlockUtil::extractSignedByte(memoryBlock, offset++, success);     // Signed!
            builder.setCurrentPsgInstrumentCellPrimaryArpeggio(arpeggio);
        }
        if ((firstByte & 0x40U) != 0U) {
            // Inverted pitch.
            auto pitch = MemoryBlockUtil::extractSignedWord(memoryBlock, offset, success);      // Signed!
            offset += 2;
            builder.setCurrentPsgInstrumentCellPrimaryPitch(-pitch);
        }
    }
    builder.setCurrentPsgInstrumentCellLink(isSound ? PsgInstrumentCellLink::softOnly : PsgInstrumentCellLink::noSoftNoHard);

    if (!success) {
        builder.addError("Error while reading the bytes of the PSG instrument " + juce::String(instrumentIndex));
    }
}

void InstrumentReader::readHardwareCell(SongBuilder& builder, const juce::MemoryBlock& musicData, juce::int64& offset, const unsigned int firstByte,
                                        const int instrumentIndex) noexcept
{
    auto success = true;

    const auto secondByte = static_cast<unsigned int>(MemoryBlockUtil::extractUnsignedChar(musicData, offset++, success));
    // By default, SoftToHard.
    auto link = PsgInstrumentCellLink::softToHard;

    // Manual Software/Hardware Period?
    const auto isManualSoftwarePeriod = ((firstByte & 0x10U) != 0U);
    const auto isManualHardwarePeriod = ((firstByte & 0x20U) != 0U);
    // Retrig?
    const auto isRetrig = ((firstByte & 0x40U) != 0U);
    builder.setCurrentPsgInstrumentCellRetrig(isRetrig);
    // Noise?
    if ((firstByte & 0x8U) != 0U) {
        const auto noise = MemoryBlockUtil::extractUnsignedChar(musicData, offset++, success);
        builder.setCurrentPsgInstrumentCellNoise(noise);
    } else {
        builder.setCurrentPsgInstrumentCellNoise(0);
    }

    // Sound?
    const auto isSound = ((firstByte & 0x1U) == 0U);                  // The value is inverted (0 = sound on).
    // Hardware envelope. In SKS, only 4 values are possible.
    const auto hardwareEnvelope = (secondByte & 3U) * 2U + 8U;         // +8 to get the real R13 value.
    builder.setCurrentPsgInstrumentCellHardwareEnvelope(static_cast<int>(hardwareEnvelope));

    // If Manual Software Period, Arpeggio and Pitch are not coded.
    auto softwareArpeggio = 0;
    if (!isManualSoftwarePeriod) {
        // Arpeggio?
        if ((firstByte & 0x2U) != 0U) {
            softwareArpeggio = MemoryBlockUtil::extractSignedByte(musicData, offset++, success);     // Signed!
            builder.setCurrentPsgInstrumentCellPrimaryArpeggio(softwareArpeggio);
        }
        // Pitch? Inverted value.
        if ((firstByte & 0x4U) != 0U) {
            const auto pitch = MemoryBlockUtil::extractSignedWord(musicData, offset, success);          // Signed!
            offset += 2;
            builder.setCurrentPsgInstrumentCellPrimaryPitch(-pitch);                                // Inverted.
        }
    } else {
        // Forced manual period.
        const auto softwarePeriod = MemoryBlockUtil::extractUnsignedWord(musicData, offset, success);
        offset += 2;
        builder.setCurrentPsgInstrumentCellPrimaryPeriod(softwarePeriod);
    }

    // If Manual Hardware Period, Shift and HardSync are not used.
    // Also switches to "SoftAndHard" mode (i.e. "independent mode" for SKS and AT1).
    unsigned int ratio;          // NOLINT(*-init-variables)
    if (!isManualHardwarePeriod) {
        ratio = 7U - ((secondByte >> 2U) & 7U);        // Inverted value, from b4 to b2.
        builder.setCurrentPsgInstrumentCellRatio(static_cast<int>(ratio));
    } else {
        // No need to set "ratio".
        link = PsgInstrumentCellLink::softAndHard;
        // Reads the Hardware Period.
        const auto manualHardwarePeriod = MemoryBlockUtil::extractUnsignedWord(musicData, offset, success);
        offset += 2;
        builder.setCurrentPsgInstrumentCellSecondaryPeriod(manualHardwarePeriod);
    }

    // Conversion. If sound = on, not Manual Hardware Period, and Hardware Sync is used, we force into Hardware Dependent mode (=HardToSoft).
    // We convert the Ratio into a multiplication of Octaves plus the Software Arpeggio. But the Shift is still used and not reset.
    if (isSound && !isManualHardwarePeriod && ((secondByte & 0x80U) != 0U)) {
        link = PsgInstrumentCellLink::hardToSoft;
        const auto newArpeggio = static_cast<int>(ratio * 12U) + softwareArpeggio;
        builder.setCurrentPsgInstrumentCellPrimaryArpeggio(newArpeggio);
        builder.setCurrentPsgInstrumentCellSecondaryArpeggio(0);
    }

    // "Hard only" not managed by the original AT code. If sound is off, the link is HardOnly.
    if (!isSound) {
        link = PsgInstrumentCellLink::hardOnly;
    }

    // Fine tune?
    if ((secondByte & 0x40U) != 0U) {
        const auto fineTunePitch = MemoryBlockUtil::extractSignedByte(musicData, offset++, success);            // Signed!
        if (link == PsgInstrumentCellLink::softToHard) {
            builder.setCurrentPsgInstrumentCellPrimaryPitch(fineTunePitch);
        } else if (link == PsgInstrumentCellLink::hardToSoft) {
            builder.setCurrentPsgInstrumentCellSecondaryPitch(fineTunePitch);
        } else {
            jassertfalse;           // Fine tune for an illegal case!
        }
    }

    builder.setCurrentPsgInstrumentCellLink(link);

    if (!success) {
        builder.addError("Error while reading the bytes of the PSG instrument " + juce::String(instrumentIndex));
    }
}

}   // namespace arkostracker
