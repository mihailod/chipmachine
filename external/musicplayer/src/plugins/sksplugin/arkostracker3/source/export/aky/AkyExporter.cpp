#include "AkyExporter.h"

#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../utils/BitNumber.h"
#include "../../utils/PsgValues.h"
#include "process/AkyConstants.h"
#include "process/RegisterBlockOptimizer.h"

namespace arkostracker 
{

const int AkyExporter::formatVersion = 1;
const int AkyExporter::progressFirst = 10;
const int AkyExporter::progressSecond = 20;

AkyExporter::AkyExporter(const std::shared_ptr<Song>& pSong, ExportConfiguration pExportConfiguration) noexcept :
        songPlayer(pSong),
        song(pSong),
        outputStream(std::make_unique<juce::MemoryOutputStream>(1024 * 32)),
        exportConfiguration(std::move(pExportConfiguration)),
        sourceGenerator(exportConfiguration.getSourceConfiguration(), *outputStream),
        subsongId(exportConfiguration.getFirstSubsongId()),               // Checks in the Encode method.
        subsongIndex(song->getSubsongIndex(subsongId).getValue()),        // No check, no need to.
        sidPlayerCapability(song->getSubsongMetadata(subsongId).getSidPlayerCapability()),
        startLocation(),
        loopLocation(),
        pastEndLocation(),
        iterationCount(),
        patterns(),
        registerBlockPool(),
        registerBlockEncoder(sourceGenerator, registerBlockPool, *this, exportConfiguration.getSourceConfiguration().isLittleEndian(), sidPlayerCapability),
        psgCount(0),
        desiredChannelCount(0)
{
    jassert(exportConfiguration.getSubsongIds().size() == 1U);       // Only one Subsong supported.
}


// Task method implementations.
// ===============================

std::pair<bool, std::unique_ptr<SongExportResult>> AkyExporter::performTask() noexcept
{
    jassert(outputStream->getPosition() == 0);              // Trying to encode twice??

    // Sanity check.
    if (song->getSubsongId(subsongIndex).isAbsent()) {
        jassertfalse;               // Trying to encode a non-existing Subsong!
        return { false, nullptr };
    }

    // Counts how many iterations are in the Song.
    const auto[counter, upTo, loopToCounter] = FrameCounter::count(*song, subsongId);
    iterationCount = counter;
    jassert(iterationCount > 0);

    // Gets the start/end locations.
    startLocation = Location(subsongId, 0);
    const auto[newLoopStartLocation, newPastEndLocation] = song->getLoopStartAndPastEndPositions(subsongId);
    loopLocation = newLoopStartLocation;
    pastEndLocation = newPastEndLocation;

    // Builds the Registers Tracks from the Song.
    auto playerConfiguration = playSongAndBuildRawPatterns();
    patterns->optimizePatterns();
    patterns->buildAkyTracks();

    onTaskProgress(progressFirst);

    // Each note may have empty lines after them. They should be removed and replaced by proper "empty" Instrument.
    // NO NEED TO DO THAT! We gain more by including the silence inside the sound!
    //patterns->modifyAkyTracksToExtractEmptyEndNotes();

    // Some Tracks may contain Blocks that contain repetitions: this takes room and can not be optimized by a loop.
    // But they can be split, to allow the loop to do a better job later.
    // NOT USED: we don't gain enough, and can lose a lot according to the songs...
    //patterns->findAndSplitBlocksWithRepetitions();

    // Fills the RegisterBlockPool, to be passed to the Tracks. They will register all their Blocks, uniquely.
    patterns->addBlocksToPool(registerBlockPool);

    if (isCanceled()) {
        return { false, nullptr };
    }

    // We have all the blocks, but they surely be optimized by making them loop.
    referenceOptimizeSortedEncodedBlocks();

    if (isCanceled()) {
        return { false, nullptr };
    }

    onTaskProgress(progressSecond);

    // Finally, we can encode the source.
    // This is what takes the more time, this is quite a lengthy operation.
    encodeSource();

    auto result = std::make_unique<SongExportResult>(std::move(outputStream), playerConfiguration);
    return { true, std::move(result) };
}

juce::String AkyExporter::getLabelForRegisterBlockIndex(const int registerBlockId, const int lineIndex) noexcept
{
    const auto label = getLabelRegisterBlock(registerBlockId);
    // Returns the label alone, or with an index.
    return (lineIndex < 0) ? label : (label + "_Index_" + juce::String(lineIndex));
}

juce::String AkyExporter::getLabelRegisterBlock(const int registerBlockId) const noexcept
{
    return getBaseLabel() + "RegisterBlock_" + juce::String(registerBlockId);
}

juce::String AkyExporter::getBaseLabel(const bool addSeparator) const noexcept
{
    return exportConfiguration.getBaseLabel() + "Subsong" + juce::String(subsongIndex) + (addSeparator ? "_" : juce::String());
}

juce::String AkyExporter::getLabelTrack(const int trackIndex) const noexcept
{
    return getBaseLabel() + "Track_" + juce::String(trackIndex);
}

PlayerConfiguration AkyExporter::playSongAndBuildRawPatterns() noexcept
{
    PlayerConfiguration playerConfiguration;

    // Re-initializes the player.
    psgCount = song->getPsgCount(subsongId);
    desiredChannelCount = PsgValues::getChannelCount(psgCount);

    songPlayer.play(startLocation, loopLocation, pastEndLocation, true, true);

    patterns = std::make_unique<AkyPatterns>(psgCount);

    // Builds one empty data list per channel.
    std::vector<std::unique_ptr<ChannelData>> channelDataListPerChannel;
    initializeChannelDataListPerChannel(channelDataListPerChannel, desiredChannelCount);

    // For every tick of the Song, studies the ChannelPlayer results.
    auto currentPositionIndex = -1;
    for (auto iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
        // BEFORE playing the tick, gets the position in the Song, else it will advance.
        const auto locationInSong(songPlayer.getCurrentLocationInSongOffline());

        // Plays one tick on all the PSGs of the Song (even if only fewer PSGs are needed).
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            songPlayer.getNextRegisters(psgIndex);
        }

        // New position?
        const auto readPositionIndex = locationInSong.getPosition();
        if ((currentPositionIndex != readPositionIndex) && (currentPositionIndex >= 0)) {          // Dismisses the first "-1" pattern.
            // The pattern is over. Stores the Tracks "dumbly".
            auto akyPattern = std::make_unique<AkyPattern>();
            for (auto& track : channelDataListPerChannel) {
                akyPattern->addTrack(std::move(track));
            }
            // Stores the Pattern.
            patterns->addPattern(std::move(akyPattern));

            // Re-initializes the Pattern.
            initializeChannelDataListPerChannel(channelDataListPerChannel, desiredChannelCount);
        }
        currentPositionIndex = readPositionIndex;

        // Gets the result of all the PSGs in the Song.
        auto results = songPlayer.getResultsOffline();
        jassert(song->getChannelCount(subsongId) == static_cast<int>(results.size()));

        fillPlayerConfiguration(playerConfiguration, results);

        // Stores the data of this line, for each PSGs.
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            auto baseChannelIndex = static_cast<size_t>(psgIndex) * static_cast<unsigned int>(PsgValues::channelCountPerPsg);
            for (auto channelIndex = baseChannelIndex, endChannelIndex = (baseChannelIndex + static_cast<unsigned int>(PsgValues::channelCountPerPsg) - 1U);
                 channelIndex <= endChannelIndex; ++channelIndex) {
                // Stores the ChannelData.
                channelDataListPerChannel.at(channelIndex)->addChannelPlayerResults(std::move(results.at(channelIndex)));
            }
        }
    }

    // The last pattern is over but not encoded.
    auto akyPattern = std::make_unique<AkyPattern>();
    for (auto& track : channelDataListPerChannel) {
        akyPattern->addTrack(std::move(track));
    }
    // Stores the Pattern.
    patterns->addPattern(std::move(akyPattern));

    return playerConfiguration;
}

void AkyExporter::initializeChannelDataListPerChannel(std::vector<std::unique_ptr<ChannelData>>& channelDataListPerChannel, const int channelCount) noexcept
{
    channelDataListPerChannel.clear();

    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        // A unique ID is given to the created track.
        channelDataListPerChannel.push_back(std::make_unique<ChannelData>());
    }
}

void AkyExporter::referenceOptimizeSortedEncodedBlocks() noexcept
{
    registerBlockPool.referenceOptimizeSortedEncodedBlocks();
}

void AkyExporter::encodeSource() noexcept
{
    // Encodes the possible ORG.
    encodeAddressChange();

    sourceGenerator.setPrefixForDisark(getBaseLabel(false));
    sourceGenerator.declareComment("Song " + song->getTitle() + ", in AKY version " + juce::String(formatVersion) + ", generated by Arkos Tracker 3.").addEmptyLine();

    // Encodes the start label.
    const auto startLabel = getBaseLabel(false);
    sourceGenerator.declareLabel(startLabel);
    sourceGenerator.declareExternalLabel(startLabel).addEmptyLine();

    // Encodes the Header.
    encodeHeader();

    // Encodes the Linker.
    encodeLinker();

    // Encodes the index table of the periods (if any).
    //encodePeriodIndexes();

    // Then, encodes the Tracks.
    encodeTracks();

    // Finally, encodes the used RegisterBlocks.
    encodeRegisterBlocks();

    sourceGenerator.declareEndOfFile();
}

void AkyExporter::encodeAddressChange() noexcept
{
    if (exportConfiguration.getAddress().isPresent()) {
        sourceGenerator.declareAddressChange(exportConfiguration.getAddress().getValue()).addEmptyLine();
    }
}

void AkyExporter::encodeHeader() noexcept
{
    // Encodes the channel count, and for each PSG, its frequency (little endian).
    sourceGenerator.declareComment("Header");

    // Declare the format version and the endianness (bit 7).
    const auto isLittleEndian = exportConfiguration.getSourceConfiguration().isLittleEndian();
    const auto endiannessComment = isLittleEndian ? juce::translate("little-endian") : juce::translate("big-endian");
    jassert(formatVersion <= 0b1111);
    sourceGenerator.declareByteRegionStart();

    // The SID format encoding.
    BitNumber headerByte;
    juce::String headerBits;
    juce::String headerProfileComment;
    switch (sidPlayerCapability.getSidPlayerProfile()) {
        default:
            jassertfalse;
        case SidPlayerProfile::cpc:
            headerBits = "00";
            headerProfileComment = "8 bits SID periods.";
            break;
        case SidPlayerProfile::amstradPlus:
        case SidPlayerProfile::custom:
            headerBits = "01";
            headerProfileComment = "16 bits SID periods.";
            break;
        case SidPlayerProfile::atariSt:
            headerBits = "10";
            headerProfileComment = "Atari ST SID periods.";
            break;
    }
    const auto comment = "Format version: " + juce::String(formatVersion) + ". Endianness: " + endiannessComment + ". " + headerProfileComment;

    headerByte.injectNumber(formatVersion, 4);
    headerByte.injectBits(headerBits);
    headerByte.injectBits("0");     // Padding.
    headerByte.injectBool(isLittleEndian);
    sourceGenerator.declareByte(headerByte.get(), comment);
    sourceGenerator.declareByte(desiredChannelCount, "How many channels are encoded.");

    // How many psgs?
    std::vector<Psg> psgs;
    song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
        psgs = subsong.getPsgs();
    });
    jassert(psgCount == static_cast<int>(psgs.size()));
    auto psgIndex = 0;
    for (const auto& psg : psgs) {
        const auto psgFrequency = psg.getPsgFrequency();
        sourceGenerator.declareComment("Frequency of the PSG index " + juce::String(psgIndex) + ": " + juce::String(psgFrequency) + "Hz.");
        sourceGenerator.declareFourBytes(psgFrequency);
        ++psgIndex;
    }
    sourceGenerator.declareByteRegionEnd();

    sourceGenerator.addEmptyLine();
}

void AkyExporter::encodeLinker() noexcept
{
    sourceGenerator.declareLabel(getBaseLabel() + "Linker");
    sourceGenerator.declarePointerRegionStart();

    const auto loopStartIndex = loopLocation.getPosition();
    const auto loopLabel = getBaseLabel() + "LinkerLoop";
    const auto songLength = pastEndLocation.getPosition();

    // Encodes the pattern list.
    jassert(songLength == patterns->getPatternCount());
    for (auto position = 0; position < songLength; ++position) {
        sourceGenerator.declareComment("Position " + juce::String(position));

        // Must loop here? If yes, creates a label.
        if (position == loopStartIndex) {
            sourceGenerator.declareLabel(loopLabel).declareComment("Loops here.");
        }

        const auto duration = patterns->getPatternDuration(position);
        jassert(duration > 0);
        sourceGenerator.declareWordForceNonReference(duration, "Duration in frames.");
        // Encodes the address of each Track.
        for (auto channelIndex = 0; channelIndex < desiredChannelCount; ++channelIndex) {
            // The track is the one re-encoded, and do not necessarily match the ones from the Song!
            const auto trackIndex = patterns->getTrackIndex(position, channelIndex);
            declareAddressOrRelativeAddress(getLabelTrack(trackIndex));
        }
        sourceGenerator.addEmptyLine();
    }

    // Encodes the loop with a duration of 0.
    sourceGenerator.declareWordForceNonReference(0, "Loops (duration = 0).");
    declareAddressOrRelativeAddress(loopLabel);

    sourceGenerator.declarePointerRegionEnd();

    sourceGenerator.addEmptyLine();
}

SourceGenerator& AkyExporter::declareAddressOrRelativeAddress(const juce::String& expression) noexcept
{
    if (exportConfiguration.mustEncodeAllAddressesAsRelativeToSongStart()) {
        return sourceGenerator.declareAddressOrRelativeAddress(expression, getBaseLabel(false));
    }
    return sourceGenerator.declareAddress(expression);
}

void AkyExporter::encodeTracks() noexcept
{
    sourceGenerator.declareComment("The tracks.");

    const auto trackCount = patterns->getTrackToEncodeCount();
    for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        const auto& akyTrack = patterns->getEncodedTrack(trackIndex);
        encodeTrack(akyTrack.getId(), akyTrack);
    }

    sourceGenerator.addEmptyLine();
}

void AkyExporter::encodeTrack(const int trackIndex, const AkyTrack& track) noexcept
{
    sourceGenerator.declareLabel(getLabelTrack(trackIndex));

    const auto blockCount = track.getRegisterBlockCount();

    // Encodes each block.
    for (auto blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
        const auto pair = track.getRegisterBlockAndDuration(blockIndex);
        const auto duration = pair.first;
        const auto& registerBlock = *pair.second;
        // The RegisterBlock declared will probably not be the one used: ask the Pool about the one to use (because data are put in common).
        const auto* registerBlockToUse = registerBlockPool.findUniqueRegisterBlock(registerBlock);
        jassert(registerBlockToUse != nullptr);

        jassert(duration > 0);
        jassert(duration <= AkyConstants::registerBlockMaximumSize);

        // Encodes the first byte (duration).
        sourceGenerator.declareByteRegionStart();
        sourceGenerator.declareByte(duration, "Duration.");
        sourceGenerator.declareByteRegionEnd();

        // Encodes the block address.
        sourceGenerator.declarePointerRegionStart();
        declareAddressOrRelativeAddress(getLabelRegisterBlock(registerBlockToUse->getId()));
        sourceGenerator.declarePointerRegionEnd();

        sourceGenerator.addEmptyLine();
    }
}

void AkyExporter::encodeRegisterBlocks() noexcept
{
    sourceGenerator.declareComment("The RegisterBlocks.");

    // What are the used RegisterBlocks?
    const auto usedRegisterBlocks = registerBlockPool.getUsedRegisterBlocks();
    jassert(!usedRegisterBlocks.empty());

    const auto possibleBaseLabel = exportConfiguration.mustEncodeAllAddressesAsRelativeToSongStart() ? getBaseLabel(false) : "";
    // Encodes all the RegisterBlocks as raw bytes, internally.
    for (auto* registerBlock : usedRegisterBlocks) {
        const auto loopLabel = getLabelRegisterBlock(registerBlock->getId());
        registerBlockEncoder.encode(*registerBlock, loopLabel, possibleBaseLabel);
    }

    // Optimizes the RegisterBlocks from their raw data (bytes).
    // This is a lengthy operation, so we pass a listener to be notified of the progress, as well as "this" for it to know if a Cancel has been requested by the user.
    const auto result = RegisterBlockOptimizer::referenceOptimizeSortedEncodedBlocks(usedRegisterBlocks, this, this,
                                                                                         progressSecond, 100);
    // No need to go further in case of a Cancel.
    if (result == OperationResult::cancelled) {
        return;
    }

    // THEN, generates the source.
    sourceGenerator.declareByteRegionStart();
    for (const auto* registerBlock : usedRegisterBlocks) {
        const auto loopLabel = getLabelRegisterBlock(registerBlock->getId());
        registerBlockEncoder.generateSource(*registerBlock, loopLabel, possibleBaseLabel);
    }
    sourceGenerator.declareByteRegionEnd();
}

void AkyExporter::onTaskProgress(const int percent) const noexcept
{
    publishTaskProgress(percent, 100);
}

void AkyExporter::fillPlayerConfiguration(PlayerConfiguration& playerConfiguration, const std::vector<std::unique_ptr<ChannelPlayerResults>>& playerResults) noexcept
{
    for (const auto& channelPlayerResults : playerResults) {
        const auto& registers = channelPlayerResults->getChannelOutputRegisters();

        const auto soundType = registers.getSoundType();

        // Noise?
        if (registers.isNoise()) {
            switch (soundType) {
                case SoundType::noSoftwareNoHardware :
                    playerConfiguration.addFlag(PlayerConfigurationFlag::noSoftNoHardNoise);
                    break;
                case SoundType::softwareOnly :
                    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareOnlyNoise);
                    break;
                case SoundType::hardwareOnly :
                    playerConfiguration.addFlag(PlayerConfigurationFlag::hardwareOnlyNoise);
                    break;
                case SoundType::softwareAndHardware :
                    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareAndHardwareNoise);
                    break;
            }
        }

        if (registers.isRetrig()) {
            switch (soundType) {
                case SoundType::hardwareOnly:
                    playerConfiguration.addFlag(PlayerConfigurationFlag::hardwareOnlyRetrig);
                    break;
                case SoundType::softwareAndHardware:
                    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareAndHardwareRetrig);
                    break;
                case SoundType::noSoftwareNoHardware:
                    [[fallthrough]];
                case SoundType::softwareOnly:
                    // Nothing to do.
                    break;
            }
        }

        switch (soundType) {
            case SoundType::noSoftwareNoHardware:
                playerConfiguration.addFlag(PlayerConfigurationFlag::noSoftNoHard);
                break;
            case SoundType::softwareOnly:
                playerConfiguration.addFlag(PlayerConfigurationFlag::softwareOnly);
                break;
            case SoundType::hardwareOnly:
                playerConfiguration.addFlag(PlayerConfigurationFlag::hardwareOnly);
                break;
            case SoundType::softwareAndHardware:
                playerConfiguration.addFlag(PlayerConfigurationFlag::softwareAndHardware);
                break;
        }
    }
}


}   // namespace arkostracker

