#include "ModSongImporter.h"

#include "ModPeriods.h"
#include "../../business/instrument/GenericInstrumentGenerator.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../song/cells/CellConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "ModTrackCell.h"

namespace arkostracker 
{

const juce::String ModSongImporter::modExtension = "mod";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const double ModSongImporter::pitchMultiplier = 14.0;

ModSongImporter::ModSongImporter() noexcept:
        songBuilder(),
        originalSongChannelCount(),
        instrumentCount(),
        outputSongChannelCount(),
        offsetPatternLinker(),
        offsetPatternsData(),

        originalSizePattern(0),
        positionToPatternIndexes(),
        highestPatternIndex(0U),
        sampleOffsets(),
        sampleIndexToVolume(),
        notePeriodToNoteNumber(),

        patternIndexToHeight(),
        foundLoopTo(),
        foundEnd()
{
}


// SongImporter method implementations.
// =======================================

bool ModSongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept
{
    const auto isExtensionMod = modExtension.equalsIgnoreCase(extension);

    const auto localChannelCount = findChannelCount(inputStream, isExtensionMod);
    return (localChannelCount != 0);
}

ImportedFormat ModSongImporter::getFormat() noexcept
{
    return ImportedFormat::mod;
}

ConfigurationType ModSongImporter::requireConfiguration() const noexcept
{
    return ConfigurationType::mod;
}

std::unique_ptr<SongImporter::Result> ModSongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& importConfiguration) noexcept
{
    const auto& modConfiguration = importConfiguration.getConfigurationForMod();

    songBuilder = std::make_unique<SongBuilder>();

    // Parses the Song.
    const auto musicData = MemoryBlockUtil::fromInputStream(inputStream);
    parse(musicData, modConfiguration);

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    return std::make_unique<Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}

ImportFirstPassReturnData ModSongImporter::getReturnData() const noexcept
{
    return ImportFirstPassReturnData::buildForMod(originalSongChannelCount);
}

// =======================================

int ModSongImporter::findChannelCount(juce::InputStream& inputStream, const bool isExtensionMod) noexcept
{
    // Reads till the M.K. mark, no need to read further.
    juce::MemoryBlock memoryBlock;
    inputStream.readIntoMemoryBlock(memoryBlock, maximumEndOffsetOfMK + 1);

    // Resets the stream, because it will surely be read once again later.
    const auto success = inputStream.setPosition(0);
    jassert(success);
    (void)success;            // To avoid a warning in Release.

    // First, channels there are.
    return findChannelAndInstrumentCount(memoryBlock, isExtensionMod).first;
}

std::pair<int, int> ModSongImporter::findChannelAndInstrumentCount(const juce::MemoryBlock& memoryBlock, const bool isExtensionMod) noexcept
{
    // There should be enough bytes read.
    if (memoryBlock.getSize() < (static_cast<size_t>(offsetMKMark31Samples) + static_cast<size_t>(mkMarkerSize))) {       // Not very logical for 15-instrument, but works anyway.
        return { 0, 0 };
    }

    auto localInstrumentCount = 31;      // Default value.

    // Tries with a 31-sample format.
    auto localChannelCount = readChannelCount(memoryBlock, offsetMKMark31Samples);
    // In case the M.K. marker couldn't be found, we rely on the extension of the file. Useful for 15-instrument modules.
    if ((localChannelCount == 0U) && isExtensionMod) {
        localChannelCount = 4U;          // Old modules!
        localInstrumentCount = 15;
    }

    return { localChannelCount, static_cast<unsigned int>(localInstrumentCount) };
}

unsigned int ModSongImporter::readChannelCount(const juce::MemoryBlock& memoryBlock, const int offsetMarker) noexcept
{
    // Reads the marker from the MemoryBlock.
    char markerBytes[4];            // NOLINT(*)
    memoryBlock.copyTo(markerBytes, offsetMarker, mkMarkerSize);          // NOLINT(*)
    if (!juce::CharPointer_ASCII::isValidString(markerBytes, 4)) {        // If not valid, do not try to convert to String (would crash).   // NOLINT(*)
        return 0U;
    }
    const juce::String marker(markerBytes, 4);    // NOLINT(*)

    // What channel count from the marker?
    auto localChannelCount = 0U;
    if ((marker == "M.K.") || (marker == "M!K!") || (marker == "4CHN") || (marker == "FLT4")) {
        localChannelCount = 4U;
    } else if (marker == "6CHN") {
        localChannelCount = 6U;
    } else if ((marker == "8CHN") || (marker == "FLT8")) {
        localChannelCount = 8U;
    }

    return localChannelCount;
}

void ModSongImporter::parse(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept
{
    if (modConfiguration.getKeptTrackCount() == 0) {
        songBuilder->addError("No final tracks were selected!");
        return;
    }

    bool success;           // NOLINT(*-init-variables)
    const auto title = MemoryBlockUtil::extractString(memoryBlock, offsetSongTitle, sizeSongTitle, success);
    songBuilder->setSongMetaData(title, juce::String(), juce::String(), "Converted by Arkos Tracker 3");

    // Creates one Subsong and populates it.
    auto& subsongBuilder = songBuilder->startNewSubsong(0);
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName);

    // Gets the channel count and thus, the size of a Pattern.
    // "Cheat" by saying this a "mod". Anyway, since the parsing started, it is logical to consider the format was well determined.
    const auto channelAndInstrumentCount = findChannelAndInstrumentCount(memoryBlock, true);
    originalSongChannelCount = channelAndInstrumentCount.first;
    instrumentCount = channelAndInstrumentCount.second;

    if (originalSongChannelCount < 4) {
        songBuilder->addError("Invalid number of channel: " + juce::String(originalSongChannelCount));
        return;
    }

    // Calculates where are the data according to the sample count.
    if (instrumentCount == 15) {
        offsetPatternLinker = offsetSamplesMetadata + (15 * sizeSampleMetadata);
        offsetPatternsData = offsetPatternLinker + sizeLinkerData;
    } else {
        offsetPatternLinker = offsetMKMark31Samples - sizeLinkerData;
        offsetPatternsData = offsetMKMark31Samples + mkMarkerSize;
    }

    // Calculates the size of the original Patterns.
    originalSizePattern = originalSongChannelCount * linesPerPattern * sizePatternCell;

    // First, reads the Pattern indexes. This must be done before reading the Instruments, to know where the samples are located.
    readPatternIndexes(memoryBlock);

    // THEN, reads the Instruments.
    readAndCreateAllInstruments(memoryBlock, modConfiguration);

    // Then reads the Patterns.
    populateSubsong(memoryBlock, modConfiguration);

    // Then declares the Positions, now that the Pattern heights are known.
    declarePositions();

    songBuilder->finishCurrentSubsong();
}

bool ModSongImporter::readPatternIndexes(const juce::MemoryBlock& memoryBlock) noexcept
{
    bool success;    // NOLINT(*-init-variables)

    highestPatternIndex = 0U;
    positionToPatternIndexes.clear();

    // How many positions?
    const auto positionCount = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetPatternLinker, success));
    if ((!success) || (positionCount == 0) || (positionCount > 129)) {
        songBuilder->addError("Illegal position count: " + juce::String(positionCount));
        return false;
    }

    // Skips the "position jump", not "standard".
    // Reads the pattern linker. Also notes the highest Pattern index.
    for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
        const auto readPatternIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetPatternLinker + 2 + positionIndex, success));
        if ((!success) || (readPatternIndex > 63)) {
            songBuilder->addError("Illegal pattern index (" + juce::String(readPatternIndex) + ") at position + " + juce::String(positionIndex) + ".");
            return false;
        }
        // Stores the Pattern index.
        positionToPatternIndexes.push_back(static_cast<unsigned char>(readPatternIndex));

        // Updates the highest Pattern Index.
        if (readPatternIndex > static_cast<int>(highestPatternIndex)) {
            highestPatternIndex = static_cast<unsigned char>(readPatternIndex);
        }
    }

    return true;
}

bool ModSongImporter::readAndCreateAllInstruments(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept
{
    // The Pattern indexes must have been read to know where the samples are stored!
    jassert(!positionToPatternIndexes.empty());

    // Builds a table locating all the offset of the samples. Simplifies the rest of the code.
    auto success = buildSampleOffsetTable(memoryBlock);
    if (!success) {
        songBuilder->addError("Unable to read the sample locations.");
        return false;
    }

    // There are always 31 instruments, from 1 to 31.
    for (auto instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
        success = readAndCreateInstrument(memoryBlock, instrumentIndex, modConfiguration);

        // Stops in case of failure.
        if (!success) {
            songBuilder->addError("Unable to read the instrument " + juce::String(instrumentIndex));
            break;
        }
    }

    return success;
}

bool ModSongImporter::readAndCreateInstrument(const juce::MemoryBlock& memoryBlock, const int instrumentIndex, const ModConfiguration& modConfiguration) noexcept
{
    // Makes sure the sample offsets has been found first.
    jassert(static_cast<int>(sampleOffsets.size()) == instrumentCount);

    const auto displayInstrumentIndex = instrumentIndex + 1;         // + 1 because the instrument 0 is reserved for AT!

    // Gets the offset on the data of the instrument.
    const auto offsetInstrumentMetadata = offsetSamplesMetadata + (instrumentIndex * sizeSampleMetadata);

    const auto sampleLengthOrError = retrieveSampleLength(memoryBlock, instrumentIndex);
    if (sampleLengthOrError == -1) {     // Error.
        return false;
    }
    if (sampleLengthOrError == 0) {      // There is no sample.
        // No sample, so we create an empty FM Instrument instead.
        songBuilder->startNewPsgInstrument(displayInstrumentIndex);
        songBuilder->startNewPsgInstrumentCell();
        songBuilder->setCurrentPsgInstrumentCell(PsgInstrumentCell::buildNoSoftwareNoHardwareCell());
        songBuilder->finishCurrentPsgInstrumentCell();
        songBuilder->finishCurrentInstrument();
        return true;
    }
    const auto sampleLength = sampleLengthOrError;

    // The sample exists. Gets the title.
    bool success;    // NOLINT(*-init-variables)
    auto title = MemoryBlockUtil::extractString(memoryBlock, offsetInstrumentMetadata, sizeSampleTitle, success);
    if (!success) {
        songBuilder->addError("Unable to read the name for the instrument " + juce::String(displayInstrumentIndex));
        return false;
    }
    title = title.trim();

    // Gets the finetune. We actually don't use it!
    const auto finetune = MemoryBlockUtil::extractSignedByte(memoryBlock, offsetInstrumentMetadata + offsetFinetuneInInstrumentMetadata, success);
    if ((finetune < -8) || (finetune > 7)) {
        songBuilder->addWarning("Illegal finetuning value " + juce::String(finetune) + " for the instrument " + juce::String(displayInstrumentIndex));
    }
    if (finetune != 0) {
        songBuilder->addWarning("The finetuning is not supported for instrument " + juce::String(displayInstrumentIndex));
    }

    // Gets the volume of the sample, stores it in a map.
    auto sampleVolume = static_cast<unsigned char>(MemoryBlockUtil::extractSignedByte(memoryBlock, offsetInstrumentMetadata + offsetVolumeInInstrumentMetadata, success));
    if (sampleVolume > 64U) {    // Yes, the maximum volume is 64, not 63.
        songBuilder->addWarning("Illegal volume value " + juce::String(sampleVolume) + " for the instrument " + juce::String(displayInstrumentIndex));
        sampleVolume = 64U;
    }
    // This "64" volume is annoying, so we consider it only 63.
    if (sampleVolume == 64U) {
        sampleVolume = 63U;
    }
    // Stores the volume. The sample index is the one as found in the Patterns (>0!).
    sampleIndexToVolume.insert(std::pair(instrumentIndex + 1, sampleVolume));

    // Gets the sample repeat offset (the read value was in word, hence must be *2).
    auto sampleRepeatOffset = 2 * MemoryBlockUtil::extractUnsignedWord(memoryBlock, offsetInstrumentMetadata + offsetRepeatOffsetInInstrumentMetadata,
                                                                       success, true);
    const auto sampleRepeatLengthRaw = MemoryBlockUtil::extractUnsignedWord(memoryBlock, offsetInstrumentMetadata + offsetRepeatLengthInInstrumentMetadata,
                                                                            success, true);
    const auto sampleRepeatLength = sampleRepeatLengthRaw * 2;

    // For very old MOD (15-instrument), the repeat offset can be way off, so it is silently corrected (MODPlug/Milky seem to do that).
    if ((sampleRepeatLength + sampleRepeatOffset) >= sampleLength) {
        // Silently corrects the sample Repeat offset.
        sampleRepeatOffset = sampleLength - sampleRepeatLength;
    }

    // The sample loops only if the encoded repeat length is superior to 1.
    const auto isSampleLooping = (sampleRepeatLengthRaw > 1);

    // Only use a PSG sample?
    if (modConfiguration.isImportInstrumentsAsPsg()) {
        buildGenericPsgInstrument(displayInstrumentIndex, title, isSampleLooping);
        return true;
    }

    // Uses a Sample...
    songBuilder->startNewSampleInstrument(displayInstrumentIndex);
    songBuilder->setCurrentInstrumentName(title);

    int endIndex;    // NOLINT(*-init-variables)
    if (isSampleLooping) {
        endIndex = sampleRepeatLength + sampleRepeatOffset - 1;
    } else {
        endIndex = sampleLength - 1;
    }
    songBuilder->setCurrentSampleInstrumentLoop(Loop(sampleRepeatOffset, endIndex, isSampleLooping));
    songBuilder->setCurrentSampleInstrumentFrequencyHz(defaultSampleFrequency);

    // The sample offset is now easy to find: a table has been generated before.
    const auto sampleOffset = sampleOffsets.at(static_cast<size_t>(instrumentIndex));

    // Copies the sample in a buffer. For now, it is signed, like the original.
    const auto* originalSampleData = static_cast<const unsigned char*>(memoryBlock.getData()) + sampleOffset;       // NOLINT(*)
    juce::MemoryBlock sampleData(originalSampleData, static_cast<size_t>(sampleLength));
    for (auto i = 0; (i < sampleLength); ++i) {
        // Converts the sample data to unsigned.
        const auto rawSampleData = static_cast<unsigned char>(sampleData[i]);
        sampleData[i] = static_cast<char>(static_cast<int>(rawSampleData) + 128);
    }
    if (!success) {
        songBuilder->addError("An error occurred while reading the sample data for the instrument " + juce::String(displayInstrumentIndex));
        return false;
    }

    songBuilder->setCurrentSampleInstrumentSample(sampleData);
    songBuilder->finishCurrentInstrument();

    return success;
}

bool ModSongImporter::buildSampleOffsetTable(const juce::MemoryBlock& memoryBlock) noexcept
{
    sampleOffsets.clear();

    // To know the offset of the sample data, the highest pattern index is required. This has been done before.
    // All the Patterns must be skipped.
    auto sampleOffset = offsetPatternsData + (originalSizePattern * (static_cast<int>(highestPatternIndex) + 1));

    for (auto instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
        sampleOffsets.push_back(sampleOffset);

        const auto sampleLength = retrieveSampleLength(memoryBlock, instrumentIndex);
        if (sampleLength == -1) {
            return false;
        }
        // The next sample will be after the current one.
        sampleOffset += sampleLength;
    }

    return true;
}

int ModSongImporter::retrieveSampleLength(const juce::MemoryBlock& memoryBlock, const int instrumentIndex) const noexcept
{
    // Gets the offset on the data of the instrument.
    const auto offsetInstrumentMetadata = offsetSamplesMetadata + (instrumentIndex * sizeSampleMetadata);

    // Reads the length, as it shows if the sample is present or not.
    bool success;    // NOLINT(*-init-variables)
    const auto sampleLengthRaw = MemoryBlockUtil::extractUnsignedWord(memoryBlock, offsetInstrumentMetadata + sizeSampleTitle, success, true); // Big endian.
    if (!success) {
        songBuilder->addError("Unable to read the size for the instrument " + juce::String(instrumentIndex + 1));
        return -1;
    }
    return sampleLengthRaw * 2;      // The encoded length is in "word".
}

void ModSongImporter::buildGenericPsgInstrument(const int instrumentIndex, const juce::String& title, const bool isLooping) const noexcept
{
    // What type of sample?
    auto instrument = std::unique_ptr<Instrument>();
    if (title.containsIgnoreCase("snare")) {
        instrument = GenericInstrumentGenerator::generateInstrumentForSnare();
    } else if (title.containsIgnoreCase("drum") || title.contains("BD") || title.containsIgnoreCase("bassd")) {
        instrument = GenericInstrumentGenerator::generateInstrumentForBassDrum();
    } else if (title.containsIgnoreCase("hat")) {
        instrument = GenericInstrumentGenerator::generateInstrumentForHihat();
    } else {
        instrument = isLooping ? GenericInstrumentGenerator::generateInstrumentForSmallAttackAndLoop() : GenericInstrumentGenerator::generateInstrumentForDecreasingVolume();
    }
    jassert(instrument != nullptr);

    instrument->setName(title);           // Overwrites the name of the generic instruments.
    songBuilder->setInstrument(instrumentIndex, std::move(instrument));
}

void ModSongImporter::populateSubsong(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept
{
    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

    // How many PSG depends on how many Tracks are kept.
    const auto keptTrackCount = modConfiguration.getKeptTrackCount();
    const auto psgCount = PsgValues::getPsgCount(keptTrackCount);
    jassert(keptTrackCount > 0);
    jassert(psgCount > 0);

    outputSongChannelCount = PsgValues::getChannelCount(psgCount);

    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        subsongBuilder.addPsg(Psg().withSamplePlayerFrequency(PsgFrequency::defaultModSamplePlayerFrequencyHz));
    }

    // Reads each Pattern.
    readAndCreatePatternsAndTracks(memoryBlock, modConfiguration);

    // The loopStart/end may have been found, or not, in which case default values are set.
    if (foundLoopTo.isAbsent()) {
        foundLoopTo = 0;
    }
    if (foundEnd.isAbsent()) {
        jassert(!positionToPatternIndexes.empty());
        foundEnd = static_cast<int>(positionToPatternIndexes.size() - 1U);
    }
    subsongBuilder.setLoop(foundLoopTo.getValue(), foundEnd.getValue());
}

void ModSongImporter::declarePositions() noexcept
{
    jassert(!positionToPatternIndexes.empty());

    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

    for (const auto patternIndex : positionToPatternIndexes) {
        subsongBuilder.startNewPosition();

        // Any non-default height stored for the Pattern?
        if (auto it = patternIndexToHeight.find(patternIndex); it != patternIndexToHeight.cend()) {
            subsongBuilder.setCurrentPositionHeight(it->second);
        }

        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);

        subsongBuilder.finishCurrentPosition();
    }
}

void ModSongImporter::readAndCreatePatternsAndTracks(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept
{
    const auto& keptTrackIndexes = modConfiguration.getTrackIndexesToKeep();
    const auto& trackIndexesToMix = modConfiguration.getTrackMixes();

    // Makes sure the Pattern indexes are unique, but in order (so that the Trailing Context matches the song evolution).
    const std::set<int> patternIndexes(positionToPatternIndexes.cbegin(), positionToPatternIndexes.cend());

    std::vector<TrailingEffectContext> contexts;
    contexts.resize(static_cast<size_t>(outputSongChannelCount));

    // Reads every Pattern. The Positions are declared later because we need to parse the Pattern to know their height, to be declared in the Positions.
    for (const auto patternIndex : patternIndexes) {
        // Extracts the original pattern.
        const auto originalPatternMemoryBlock = extractFullPattern(memoryBlock, patternIndex);

        // Reads the pattern, extracts the speed/break events. This must be done now, because these data may be lost in the mix/track selection.
        readPatternGenerateSpeedTrackFindHeightsAndLoop(*originalPatternMemoryBlock, originalSongChannelCount, patternIndex);

        // The Pattern will most of the time be changed: the user will want to mix Tracks, and keep only certain Tracks.
        // We create a new local Pattern from scratch from these changes.
        const auto generatedPattern = generatePatternFromConfiguration(*originalPatternMemoryBlock, keptTrackIndexes, trackIndexesToMix);

        readPatternBlockAndCreateTracksAndDeclarePattern(*generatedPattern, patternIndex, modConfiguration.isImportInstrumentsAsPsg(),
                                                         contexts);
    }
}

void ModSongImporter::readPatternGenerateSpeedTrackFindHeightsAndLoop(const juce::MemoryBlock& patternMemoryBlock, const int inputPatternChannelCount, const int patternIndex) noexcept
{
    jassert(inputPatternChannelCount >= 0);

    auto patternHeightFound = false;

    // Reads the Pattern, searching for speed and break effects.
    for (auto line = 0; line < linesPerPattern; ++line) {
        for (auto channelIndex = 0; channelIndex < inputPatternChannelCount; ++channelIndex) {
            auto cell = readCell(patternMemoryBlock, inputPatternChannelCount, channelIndex, line);

            if (cell.isEffectPresent()) {
                const auto value = cell.getEffectValue();
                switch (cell.getEffectNumber()) {
                    case 0xb:           // Position jump. "Includes" a pattern break.
                    {
                        if (foundLoopTo.isAbsent()) {
                            foundLoopTo = value;
                        }
                        // Keeps only the first break.
                        if (!patternHeightFound) {
                            patternHeightFound = true;
                            patternIndexToHeight[patternIndex] = line + 1;           // +1 because the current line must be included to the height.
                        }
                        break;
                    }
                    case 0xd:           // Pattern break.
                    {
                        // Keeps only the first break.
                        if (!patternHeightFound) {
                            patternHeightFound = true;
                            patternIndexToHeight[patternIndex] = line + 1;           // +1 because the current line must be included to the height.
                        }
                        // The value is not managed by AT2.
                        if (value != 0) {
                            songBuilder->addWarning("The Pattern Break value is not managed for line " + juce::String(line)
                                                    + ", channel " + juce::String(channelIndex));
                        }
                        break;
                    }
                    case 0xf:           // Speed.
                    {
                        manageSpeedEffect(patternIndex, value, line);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

void ModSongImporter::manageSpeedEffect(const int patternIndex, const int effectValue, const int lineIndex) const noexcept
{
    auto speed = effectValue;

    if (speed > 32) {
        // If >32, this means BPM. Converts to Speed. Considers a 50hz player.
        // See the calculation here: https://modarchive.org/forums/index.php?topic=2709.0
        speed = static_cast<int>(60.0F / (static_cast<float>(speed) * 0.08F));
    }

    // Speed 0 is treated as speed 1.
    if (speed == 0) {
        speed = 1;
    }

    // Puts the speed in a Speed Track.
    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();
    subsongBuilder.setSpeedToTrack(patternIndex, lineIndex, speed);
}

std::unique_ptr<juce::MemoryBlock> ModSongImporter::extractFullPattern(const juce::MemoryBlock& songMemoryBlock, const int patternIndex) noexcept
{
    const auto patternDataOffset = offsetPatternsData + patternIndex * originalSizePattern;

    // Creates a pattern MemoryBlock, as big as an original Pattern and with its data.
    const auto originalPatternSize = linesPerPattern * sizePatternCell * originalSongChannelCount;
    auto originalPatternMemoryBlock = std::make_unique<juce::MemoryBlock>(originalSizePattern, false);
    // Copies the data.
    auto* destination = static_cast<unsigned char*>(originalPatternMemoryBlock->getData()); // NOLINT(clion-misra-cpp2008-5-2-8)
    songMemoryBlock.copyTo(destination, patternDataOffset, static_cast<size_t>(originalPatternSize));

    return originalPatternMemoryBlock;
}

std::unique_ptr<juce::MemoryBlock> ModSongImporter::generatePatternFromConfiguration(const juce::MemoryBlock& originalPatternMemoryBlock,
                                                                                     const std::set<int>& keptTrackIndexes,
                                                                                     const std::vector<std::pair<int, int>>& trackIndexesToMix) const noexcept
{
    // Creates a copy of the original Pattern.
    const auto generatedPattern = std::make_unique<juce::MemoryBlock>(originalPatternMemoryBlock);

    // Mixes the tracks.
    for (const auto& trackIndexToMix : trackIndexesToMix) {
        mixTracks(*generatedPattern, originalSongChannelCount, trackIndexToMix);
    }

    // Keeps only the selected Tracks and fills a new output Pattern MemoryBlock. There might be unused Tracks (which is fine).
    return generateOutputPatternWithSpecificTracks(*generatedPattern, originalSongChannelCount,
                                                                               outputSongChannelCount, keptTrackIndexes);
}

std::unique_ptr<juce::MemoryBlock> ModSongImporter::generateOutputPatternWithSpecificTracks(const juce::MemoryBlock& inputPatternMemoryBlock,
                                                                                            const int inputChannelCount, const int outputChannelCount,
                                                                                            const std::set<int>& keptTrackIndexes) noexcept
{
    jassert((outputChannelCount % 3) == 0);

    // Generates an output Pattern buffer that is large enough to hold all the Tracks.
    const auto outputSongSizePattern = linesPerPattern * sizePatternCell * outputChannelCount;
    auto generatedPatternMemoryBlock = std::make_unique<juce::MemoryBlock>(outputSongSizePattern, true);

    // For each line of the input Pattern, takes the Cells that must be kept and put them in the output Pattern.
    unsigned char cellBytes[sizePatternCell]; // NOLINT(*-avoid-c-arrays)
    for (auto line = 0; line < linesPerPattern; ++line) {
        const auto sourceLineOffset = line * inputChannelCount * sizePatternCell;
        const auto destinationLineOffset = line * outputChannelCount * sizePatternCell;

        auto destinationTrackIndex = 0;
        for (const auto trackIndexToKeep : keptTrackIndexes) {
            // Extracts the source data.
            const auto sourceOffset = sourceLineOffset + trackIndexToKeep * sizePatternCell;
            inputPatternMemoryBlock.copyTo(cellBytes, sourceOffset, sizePatternCell);           // NOLINT(*)

            // Copies it to the destination. The destination Cells are added one after the other.
            const auto destinationOffset = destinationLineOffset + destinationTrackIndex * sizePatternCell;
            generatedPatternMemoryBlock->copyFrom(cellBytes, destinationOffset, sizePatternCell);       // NOLINT(*)

            ++destinationTrackIndex;
        }
    }

    return generatedPatternMemoryBlock;
}

void ModSongImporter::readPatternBlockAndCreateTracksAndDeclarePattern(const juce::MemoryBlock& patternMemoryBlock, const int patternIndex,
                                                                       const bool isImportInstrumentsAsPsg, std::vector<TrailingEffectContext>& contexts) noexcept
{
    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

    // Declares the Patterns.
    subsongBuilder.startNewPattern(patternIndex);

    // Creates one track for each column.
    for (auto channelIndex = 0; channelIndex < outputSongChannelCount; ++channelIndex) {
        // Opens the PatternCell to enter the Track Index that is going to be generated.
        const auto trackIndex = patternIndex * outputSongChannelCount + channelIndex;
        subsongBuilder.setCurrentPatternTrackIndex(channelIndex, trackIndex);

        // Encodes each line of this Track.
        for (auto lineIndex = 0; lineIndex < linesPerPattern; ++lineIndex) {
            const auto modTrackCell = readCell(patternMemoryBlock, outputSongChannelCount, channelIndex, lineIndex);
            encodeCell(lineIndex, modTrackCell, trackIndex, isImportInstrumentsAsPsg, contexts.at(static_cast<size_t>(channelIndex)));
        }
    }

    subsongBuilder.setCurrentPatternSpeedTrackIndex(patternIndex);
    subsongBuilder.setCurrentPatternEventTrackIndex(patternIndex);

    subsongBuilder.finishCurrentPattern();
}

void ModSongImporter::encodeCell(const int lineIndex, const ModTrackCell& modCell, const int trackIndex, const bool isImportInstrumentsAsPsg, TrailingEffectContext& context) noexcept
{
    if (modCell.isEmpty()) {
        return;
    }

    jassert((lineIndex >= 0) && (lineIndex < linesPerPattern));

    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();
    Cell cell;

    // Is there a note and an instrument?
    const auto isNote = modCell.isNotePresent();
    auto instrumentNumber = modCell.getSampleNumber();
    if (isNote) {
        const auto period = modCell.getSamplePeriod();
        auto noteAndOctave = findModuleNoteIndexFromPeriod(period);
        // The note range on AT is higher, for samples.
        noteAndOctave += isImportInstrumentsAsPsg ? -3 : 4 * 12 - 3;

        cell.setNote(Note::buildNote(noteAndOctave));

        // The note must go with an Instrument in MOD, but it may not be set.
        // If 0, use the one previously stored.
        if (instrumentNumber == 0) {
            const auto previousInstrument = context.getCurrentInstrumentIndex();
            instrumentNumber = previousInstrument.isPresent() ? previousInstrument.getValue() : CellConstants::rstInstrument;   // Better than nothing...
        }

        cell.setInstrument(instrumentNumber);

        context.declareNote();
    }

    // Note or not, if there is an Instrument, it becomes the current Instrument (if there is one).
    if (instrumentNumber > 0) {
        context.setCurrentInstrumentIndex(instrumentNumber);
    }

    // Reads and manages the effects.
    readAndManageEffects(modCell, lineIndex, isNote, instrumentNumber, context, cell);

    if (!cell.isEmpty()) {
        subsongBuilder.setCellOnTrack(trackIndex, lineIndex, cell);
    }
}

ModTrackCell ModSongImporter::readCell(const juce::MemoryBlock& patternData, const int patternChannelCount, const int channelIndex, const int lineIndex) noexcept
{
    const auto lineOffset = lineIndex * patternChannelCount * sizePatternCell;
    const auto cellOffset = lineOffset + channelIndex * sizePatternCell;
    return ModTrackCell::buildFromMemoryBlock(patternData, cellOffset);
}

int ModSongImporter::findModuleNoteIndexFromPeriod(const int period) noexcept
{
    // Is the note already known for this period?
    if (notePeriodToNoteNumber.find(period) != notePeriodToNoteNumber.cend()) {
        return notePeriodToNoteNumber[period];
    }

    // No. Let's find it. It is not fast code. Who cares?
    // The note index starts at 3. Why? Because the periods are (probably) centered on the C-5,
    // not the A4 (LA4), so the notes won't look the same as on other trackers.
    // The sample frequency calculator also compensates this.
    auto noteIndex = 3;
    // Upper-bounds limit must be tested.
    const auto lowestPeriod = ModPeriods::amigaPeriods.at(ModPeriods::amigaPeriods.size() - 1U);
    if (period < lowestPeriod) {
        // Use the highest possible note.
        noteIndex = static_cast<int>(notePeriodToNoteNumber.size() - 1U);
    } else {
        // Searches for the note in all the periods.
        for (const auto amigaPeriod : ModPeriods::amigaPeriods) {
            if (amigaPeriod <= period) {
                break;
            }
            ++noteIndex;
        }
    }

    // Stores the relation between the note and the period for a faster future look-up.
    notePeriodToNoteNumber[period] = noteIndex;

    return noteIndex;
}

void ModSongImporter::readAndManageEffects(const ModTrackCell& modCell, const int lineIndex, const bool isNote, const int instrumentNumber, TrailingEffectContext& context,
                                           Cell& cell) noexcept
{
    jassert((lineIndex >= 0) && (lineIndex <= 127)); (void)lineIndex;           // To avoid a warning in Release.

    const auto effectNumber = modCell.getEffectNumber();
    const auto effectValue = modCell.getEffectValue();
    jassert(effectNumber <= 0xf);
    jassert(effectValue <= 0xff);

    // Pre-pass: manages the volume. If there is no value, we may create one if there is a note.
    // ------------------------------------------------------------------------------------
    const auto isVolumeEffect = (effectNumber == 0xc);
    OptionalInt volume;

    if (isNote && !isVolumeEffect) {      // A note, but no volume. Adds our own from the volume of the sample itself.
        if (sampleIndexToVolume.find(instrumentNumber) == sampleIndexToVolume.cend()) {
            volume = 0x3f;
        } else {
            volume = sampleIndexToVolume.at(instrumentNumber);
        }
    }
    // If volume effect, simply writes it.
    if (isVolumeEffect) {
        volume = effectValue;
    }

    if (volume.isPresent()) {
        auto volumeToEncode = volume.getValue();
        volumeToEncode = NumberUtil::correctNumber(volumeToEncode, 0, 0x3f);           // Volume to 64 is actually authorized in MOD. But we keep it to 63.
        // Converts the volume from linear to logarithmic, else the volume are too low for our PSG.
        volumeToEncode = NumberUtil::toLog(volumeToEncode, 0x3f, 0xf);      // Encodes the volume only on 4 bits.

        addEffectFromContext(TrailingEffect::volume, Effect::volume, volumeToEncode, context, cell);
    }

    // ------------------------------------------------------------------------------------
    // Manages all the effects, except the volume, pattern break/jump/speed, managed just above.
    switch (effectNumber) {
        case 0x0:   // Arpeggio, or nothing.
        {
            if (effectValue > 0) {
                addEffectFromContext(TrailingEffect::arpeggio3Notes, Effect::arpeggio3Notes, effectValue, context, cell);
            } else {
                // If no Arpeggio in the note, we must stop it if there was an arpeggio that was started before!
                if (context.isEffectPresent(TrailingEffect::arpeggio3Notes)) {
                    context.invalidateEffect(TrailingEffect::arpeggio3Notes);
                    cell.addEffect(Effect::arpeggio3Notes, 0);
                }
            }
            break;
        }
        case 0x1:   // Pitch up.
        {
            // 0 means "don't do anything", which is the same in AT, so don't need any particular behavior.
            addEffectFromContext(TrailingEffect::pitchUp, Effect::fastPitchUp,
                                 static_cast<int>(static_cast<double>(effectValue) * pitchMultiplier), context, cell);
            break;
        }
        case 0x2:   // Pitch down.
        {
            // 0 means "don't do anything", which is the same in AT, so don't need any particular behavior.
            addEffectFromContext(TrailingEffect::pitchDown, Effect::fastPitchDown,
                                 static_cast<int>(static_cast<double>(effectValue) * pitchMultiplier), context, cell);
            break;
        }
        case 0xb:   // Position jump. Nothing to do, already managed before.
            [[fallthrough]];
        case 0xc:   // Volume. Nothing to do, already managed above.
            [[fallthrough]];
        case 0xd:   // Pattern break. Nothing to do, already managed above.
            [[fallthrough]];
        case 0xf:   // Change speed. Nothing to do, already managed before.
            [[fallthrough]];
        default:
            break;
    }
}

void ModSongImporter::addEffectFromContext(const TrailingEffect trailingEffect, const Effect outputEffect, const int value, TrailingEffectContext& context, Cell& cell) noexcept
{
    const auto trailingEffectToEncode = context.declareEffect(trailingEffect, value);
    if (trailingEffectToEncode.first == trailingEffect) {
        cell.addEffect(outputEffect, trailingEffectToEncode.second);
    }
}

void ModSongImporter::mixTracks(juce::MemoryBlock& patternMemoryBlock, const int channelCount, const std::pair<int, int>& trackIndexToMix) noexcept
{
    jassert(channelCount >= 4);
    jassert(trackIndexToMix.first != trackIndexToMix.second);      // The UI should prevent this.

    const auto sourceTrackIndex = trackIndexToMix.first;
    const auto destinationTrackIndex = trackIndexToMix.second;
    jassert(sourceTrackIndex < channelCount);
    jassert(destinationTrackIndex < channelCount);
    // If the source and destination is the same, there is nothing to do.
    if (sourceTrackIndex == destinationTrackIndex) {
        return;
    }

    // What is called "current Track" is the Track from which data is picked up.
    // The priority is on the source Track, always.
    auto currentTrackIndex = destinationTrackIndex;
    unsigned char cellBytes[sizePatternCell];          // One buffer for reading bytes from a cell. NOLINT(*-avoid-c-arrays)
    for (auto line = 0; line < linesPerPattern; ++line) {
        // Reads the source and destination cell.
        const auto lineOffset = line * channelCount * sizePatternCell;
        const auto sourceOffset = lineOffset + sourceTrackIndex * sizePatternCell;
        const auto destinationOffset = lineOffset + destinationTrackIndex * sizePatternCell;
        const auto sourceCell = ModTrackCell::buildFromMemoryBlock(patternMemoryBlock, sourceOffset);
        const auto destinationCell = ModTrackCell::buildFromMemoryBlock(patternMemoryBlock, destinationOffset);

        // If there is a note in the source, it is prioritized (even if it is not in the destination).
        if (sourceCell.isNotePresent()) {
            currentTrackIndex = sourceTrackIndex;
        } else if (destinationCell.isNotePresent()) {   // No note in the source. If note in the destination, it is used.
            currentTrackIndex = destinationTrackIndex;
        }

        // Reads the source data and puts in the destination, if the Tracks are different.
        if (currentTrackIndex != destinationTrackIndex) {
            // Direct copy of bytes. Copies source to a small array.
            patternMemoryBlock.copyTo(cellBytes, sourceOffset, sizePatternCell);        // NOLINT(*)
            // Copies array to destination.
            patternMemoryBlock.copyFrom(cellBytes, destinationOffset, sizePatternCell); // NOLINT(*)
        }
    }
}

}   // namespace arkostracker
