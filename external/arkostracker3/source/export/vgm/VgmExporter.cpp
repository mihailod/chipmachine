#include "VgmExporter.h"

#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../player/SongPlayer.h"
#include "../../utils/FileUtil.h"

namespace arkostracker 
{

const int VgmExporter::maximumPsgCount = 2;

VgmExporter::VgmExporter(std::shared_ptr<Song> pSong, Id pSubsongId, const bool pCompress) noexcept :
        song(std::move(pSong)),
        subsongId(std::move(pSubsongId)),
        compress(pCompress),
        previousHardwareEnvelopes()
{
}

std::pair<VgmExporter::PreliminaryCheck, juce::String> VgmExporter::performPreliminaryCheck(const Song& song, const Id& subsongId) noexcept
{
    // Too many PSGs?
    const auto psgs = song.getSubsongPsgs(subsongId);
    auto psgCount = static_cast<int>(psgs.size());
    jassert(psgCount >= 1);
    if (psgCount > maximumPsgCount) {
        return { VgmExporter::PreliminaryCheck::tooManyPsgs, juce::translate("Warning: Only two AY/YM PSGs can be exported in VGM. The other ones will be discarded.") };
    }
    psgCount = std::min(psgCount, maximumPsgCount);     // Caps to 2.

    // Same frequencies for all the PSGs?
    const auto firstPsgFrequencyHz = psgs.at(0U).getPsgFrequency();
    for (auto psgIndex = 1; psgIndex < psgCount; ++psgIndex) {
        if (psgs.at(static_cast<size_t>(psgIndex)).getPsgFrequency() != firstPsgFrequencyHz) {
            return { VgmExporter::PreliminaryCheck::differentPsgFrequencies, juce::translate("Warning: The VGM format does not allow AY/YM with different frequencies."
                                                                                             " The second PSG will be forced to the frequency of the first.") };
        }
    }

    return { VgmExporter::PreliminaryCheck::ok, juce::translate("Valid subsong: only one PSG is present.") };
}


// Task method implementations.
// ==========================================

std::pair<bool, std::unique_ptr<juce::MemoryOutputStream>> VgmExporter::performTask() noexcept
{
    auto vgmData = exportVgm();

    // Error and/or no need to compress? Stops here.
    auto success = (vgmData != nullptr);
    if (!compress || !success) {
        return { success, std::move(vgmData) };
    }

    // Must compress.
    auto compressedVgmData = FileUtil::gZip(vgmData->getData(), vgmData->getDataSize());

    return { (compressedVgmData != nullptr), std::move(compressedVgmData) };
}

std::unique_ptr<juce::MemoryOutputStream> VgmExporter::exportVgm() noexcept
{
    auto vgmData = std::make_unique<juce::MemoryOutputStream>();

    // Gets the PSGs. Caps to 2 maximum, ignores the rest.
    const auto psgs = song->getSubsongPsgs(subsongId);
    const auto originalPsgCount = static_cast<int>(psgs.size());
    const auto psgCount = std::min(originalPsgCount, maximumPsgCount);      // Only two PSGs maximum are managed.

    // Sentinel values on the previous hardware envelopes, one per PSG.
    previousHardwareEnvelopes.clear();
    previousHardwareEnvelopes.resize(static_cast<unsigned int>(maximumPsgCount), 16);

    const auto subsongMetadata = song->getSubsongMetadata(subsongId);

    // How much to wait between two frames? 44100 is hardcoded in the VGM format.
    const auto replayFrequencyHz = subsongMetadata.getReplayFrequencyHz();
    const auto waitValue = static_cast<int>(44100.0F / replayFrequencyHz);

    // Counts how many iterations are in the Song.
    const auto[counter, upTo, loopCounter] = FrameCounter::count(*song, subsongId);
    const auto iterationCount = counter;
    jassert(iterationCount > 0);

    const auto firstPsg = psgs.at(0U);
    const auto isSecondPsg = (psgCount > 1);
    constexpr auto vgmDataOffset = 0x100;           // Arbitrary, just a safe value after the header.

    // Specs: https://vgmrips.net/wiki/VGM_Specification
    // Writes the header.
    vgmData->writeText("Vgm ", false, false, nullptr);
    const auto eofOffset = vgmData->getPosition();
    vgmData->writeInt(0);               // EOF offset. Written later.
    vgmData->writeInt(0x00000171);      // v1.70 as BCD-code.
    vgmData->writeInt(0);               // Useless: SN76489 clock.
    vgmData->writeInt(0);               // Useless: YM2413 clock.
    const auto gd3Offset = vgmData->getPosition();
    vgmData->writeInt(0);               // GD3 offset. Written later.
    const auto totalWaitCountOffset = vgmData->getPosition();
    vgmData->writeInt(0);               // 0x18. Total of all wait values in the file. Doesn't seem useful, but done anyway.
    const auto loopOffset = vgmData->getPosition();
    vgmData->writeInt(0);               // 0x1c. Relative offset to loop point, or 0 if no loop. Written later.
    const auto waitCountInOneLoopOffset = vgmData->getPosition();
    // 0x20. Number of samples in one loop, or 0 if no loop.
    // Total of all wait values between the loop point and the end of the file. Written later. Needed for the loop to work.
    vgmData->writeInt(0);
    vgmData->writeInt(static_cast<int>(replayFrequencyHz));               // 0x24. Rate of recording in Hz.
    vgmData->writeShort(0);             // 0x28. SN76489 feedback.
    vgmData->writeByte(0);               // SN76489 shift.
    vgmData->writeByte(0);               // SN76489 flags.
    vgmData->writeInt(0);               // 0x2c. YM2612 clock.
    vgmData->writeInt(0);               // 0x30. YM2151 clock.
    vgmData->writeInt(vgmDataOffset - 0x34);    // 0x34. VGM data offset.

    while (vgmData->getDataSize() < 0x74U) {
        vgmData->writeInt(0);
    }
    // 0x74. AY8910 clock. Sets bit 30 for dual chip. Both must have the same frequency.
    vgmData->writeInt(static_cast<int>(static_cast<unsigned int>(firstPsg.getPsgFrequency()) | (isSecondPsg ? 0x40000000U : 0U)));
    vgmData->writeByte(firstPsg.getType() == PsgType::ay ? 0x01 : 0x10);        // AY8910 chip type.
    vgmData->writeByte(0x01);           // AY8910 flags. Single output (default).

    // Reaches the data.
    while (vgmData->getDataSize() < static_cast<unsigned int>(vgmDataOffset)) {
        vgmData->writeByte(0);
    }

    // Initializes the player.
    const auto startLocation = Location(subsongId, 0);
    const auto[loopStartLocation, pastEndLocation] = song->getLoopStartAndPastEndPositions(subsongId);

    SongPlayer songPlayer(song);
    songPlayer.play(startLocation, startLocation, pastEndLocation, true, true);

    auto waitCountAfterLoop = 0;
    auto waitCount = 0;
    juce::int64 loopIterationOffset = 0;
    // Reads all the ticks and encode them. One pass is required.
    for (auto iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
        // Stores the offset if we are where the loop is performed.
        if (iterationIndex == loopCounter) {
            loopIterationOffset = vgmData->getPosition();
        }

        // Plays for EACH PSG, even the ones will do NOT use, else the SongPlayer complains!
        for (auto psgIndex = 0; psgIndex < originalPsgCount; ++psgIndex) {
            auto psgRegistersAndSamples = songPlayer.getNextRegisters(psgIndex);
            auto& psgRegisters = *psgRegistersAndSamples.first;
            if (psgIndex >= maximumPsgCount) {
                // Ignores this value if the PSG is out of bounds of what the format is capable of.
                continue;
            }

            auto& previousHardwareEnvelope = previousHardwareEnvelopes.at(static_cast<size_t>(psgIndex));
            encodePsgFrame(*vgmData, psgRegisters, previousHardwareEnvelope, (psgIndex == 1));

            previousHardwareEnvelope = psgRegisters.getHardwareEnvelopeAndRetrig().getEnvelope();
        }

        // Encodes the wait to the next frame, increases the counters.
        encodeWait(*vgmData, waitValue);
        waitCount += waitValue;
        if (iterationIndex >= loopCounter) {
            waitCountAfterLoop += waitValue;
        }

        // Cancelled?
        if (isCanceled()) {
            return nullptr;
        }

        // Notifies the progress.
        onTaskProgressed(iterationIndex, iterationCount);
    }

    vgmData->writeByte(static_cast<char>(0x66));        // End of sound data.

    // GD3. https://vgmrips.net/wiki/GD3_Specification
    const auto gd3StartOffset = vgmData->getPosition();
    vgmData->writeText("Gd3 ", false, false, nullptr);
    vgmData->writeInt(0x00000100);      // Version.
    const auto vgmSizeOffset = vgmData->getPosition();
    vgmData->writeInt(0x0);             // Size of the GD3.

    writeGd3Text(*vgmData, subsongMetadata.getName());       // Track name.
    writeGd3Text(*vgmData, song->getTitle());   // Game name.
    writeGd3Text(*vgmData, PsgFrequency::frequencyToPlatform(firstPsg.getPsgFrequency()));  // System name.
    writeGd3Text(*vgmData, song->getComposer());  // Original composer.

    const auto creationDate = juce::Time(song->getCreationDateMs());
    const auto formattedDate = juce::String(creationDate.getYear()) + "/"
                               + juce::String(creationDate.getMonth() + 1) + "/" + juce::String(creationDate.getDayOfMonth());
    writeGd3Text(*vgmData, formattedDate, false);             // Release game (yyyy/mm/dd), or yyyy/mm, or yyyy).
    writeGd3Text(*vgmData, "Arkos Tracker 3", false);      // Converter name.
    writeGd3Text(*vgmData, song->getComments(), false);      // Notes.
    const auto vgmEndOffset = vgmData->getPosition();
    writeInt(*vgmData, vgmSizeOffset, static_cast<int>(vgmEndOffset - vgmSizeOffset - 4));

    writeInt(*vgmData, gd3Offset, static_cast<int>(gd3StartOffset - gd3Offset));        // Writes the GD3 offset.
    writeInt(*vgmData, eofOffset, static_cast<int>(vgmEndOffset - eofOffset));          // Writes the EOF offset.
    writeInt(*vgmData, loopOffset, static_cast<int>(loopIterationOffset - loopOffset)); // Writes the loop.
    writeInt(*vgmData, waitCountInOneLoopOffset, waitCountAfterLoop);                   // Needed for the loop to work.
    writeInt(*vgmData, totalWaitCountOffset, waitCount);                                // Doesn't seem useful, but done anyway.

    return vgmData;
}

void VgmExporter::encodePsgFrame(juce::MemoryOutputStream& vgmData, const PsgRegisters& psgRegisters, const int previousHardwareEnvelope, const bool isSecondPsg) noexcept
{
    // Encodes each register.
    for (auto reg = 0; reg <= static_cast<int>(PsgRegistersIndex::hardwareEnvelope); ++reg) {
        const auto value = psgRegisters.getValueFromRegister(reg);

        // Don't encode the hardware envelope if the value is the same, and there is no retrig.
        if ((reg == static_cast<int>(PsgRegistersIndex::hardwareEnvelope) && !psgRegisters.isRetrig() && (value == previousHardwareEnvelope))) {
            continue;
        }

        // 0xA0 / register (+0x80 if second psg) / value.
        vgmData.writeByte(static_cast<char>(0xa0));
        vgmData.writeByte(static_cast<char>(static_cast<unsigned int>(reg) | (isSecondPsg ? 0x80U : 0U)));
        vgmData.writeByte(static_cast<char>(value));
    }
}

void VgmExporter::writeGd3Text(juce::MemoryOutputStream& vgmData, const juce::String& text, const bool addEmptyNonEnglishSentence) noexcept
{
    vgmData.writeText(text, true, false, nullptr);
    vgmData.writeShort(0x0);
    // Adds an empty String for "non-english", which we don't use.
    if (addEmptyNonEnglishSentence) {
        vgmData.writeShort(0x0);
    }
}

void VgmExporter::writeInt(juce::MemoryOutputStream& vgmData, const juce::int64 whereToWrite, const int data) noexcept
{
    const auto success = vgmData.setPosition(whereToWrite);
    jassert(success); (void)success;
    vgmData.writeInt(data);
}

void VgmExporter::encodeWait(juce::MemoryOutputStream& vgmData, const int waitValue) noexcept
{
    // Some values can be optimized.
    if (waitValue == 735) {         // 60hz.
        vgmData.writeByte(static_cast<char>(0x62));
    } else if (waitValue == 882) {  // 50hz.
        vgmData.writeByte(static_cast<char>(0x63));
    } else {
        vgmData.writeByte(static_cast<char>(0x61));
        vgmData.writeByte(static_cast<char>(waitValue % 255));
        vgmData.writeByte(static_cast<char>(waitValue / 256));
    }
}

}   // namespace arkostracker
