#include "ChpSongImporter.h"

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../song/cells/CellConstants.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StringUtil.h"

namespace arkostracker 
{

const int ChpSongImporter::glideValue = 0x200;
const juce::String ChpSongImporter::headerTag = "CHIPNSFX format";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const size_t ChpSongImporter::lineIndexLinker = 5U;                         // Line where the linker starts.

ChpSongImporter::ChpSongImporter() :
        readLines(),
        globalTransposition(0),
        lineIndexOfLinker(0U),
        lineIndexOfTracks(0U),
        lineIndexOfInstruments(0U),
        trackIndexToLastCellIndexUsed()
{
}


// SongImporter method implementations.
// =======================================

bool ChpSongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Checks the header.
    const auto headerTagSize = headerTag.length();
    if (inputStream.getTotalLength() < headerTagSize) {
        return false;
    }

    juce::MemoryBlock memoryBlock(static_cast<size_t>(headerTagSize));
    inputStream.read(memoryBlock.getData(), headerTagSize);

    return (memoryBlock.toString() == headerTag);
}

ImportedFormat ChpSongImporter::getFormat() noexcept
{
    return ImportedFormat::chp;
}

std::unique_ptr<SongImporter::Result> ChpSongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    const auto memoryBlock = MemoryBlockUtil::fromInputStream(inputStream);
    const auto songBuilder = parse(memoryBlock);
    jassert(songBuilder != nullptr);

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();
    return std::make_unique<Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}


// =======================================

std::unique_ptr<SongBuilder> ChpSongImporter::parse(const juce::MemoryBlock& memoryBlock) noexcept
{
    auto songBuilder = std::make_unique<SongBuilder>();

    // Gets the lines from the MemoryBlock, once and for all.
    readLines = MemoryBlockUtil::extractStrings(memoryBlock);
    if (readLines.size() < 6U) {
        songBuilder->addError("Too few lines in the file, abnormal.");
        return songBuilder;
    }

    // Finds the two separators (for Tracks and Instruments).
    auto success = findPartsFromSeparator();
    if (!success) {
        songBuilder->addError("Unable to find the tracks/instruments parts.");
        return songBuilder;
    }

    // Reads each part of the music. The Tracks are parsed BEFORE the Linker, because we need to know their height, it is easier this way.
    songBuilder->startNewSubsong(0);
    success = parseHeader(*songBuilder);
    success = success && parseTracks(*songBuilder);
    success = success && parseLinker(*songBuilder);
    songBuilder->finishCurrentSubsong();

    success = success && parseInstruments(*songBuilder);
    
    if (!success) {
        songBuilder->addError("Error while parsing the song.");
    }

    return songBuilder;
}

bool ChpSongImporter::findPartsFromSeparator() noexcept
{
    jassert(!readLines.empty());

    // First separator: the Linker.
    auto separatorIndex = findSeparator(0U);
    if (separatorIndex == 0U) {
        return false;
    }
    lineIndexOfLinker = separatorIndex + 1U;

    // Second separator: the Tracks.
    separatorIndex = findSeparator(lineIndexOfLinker);
    if (separatorIndex == 0U) {
        return false;
    }
    lineIndexOfTracks = separatorIndex + 1U;          // The Tracks are just after.

    // Third separator: the Instruments.
    separatorIndex = findSeparator(lineIndexOfTracks);
    if (separatorIndex == 0U) {
        return false;
    }
    lineIndexOfInstruments = separatorIndex + 1U;

    return true;
}

size_t ChpSongImporter::findSeparator(const size_t startLineIndex) const noexcept
{
    for (auto lineIndex = startLineIndex, lineCount = readLines.size(); lineIndex < lineCount; ++lineIndex) {
        if (readLines.at(lineIndex) == "===") {
            // Found the separator, we can stop!
            return lineIndex;
        }
    }

    // Separator not found.
    return 0U;
}

bool ChpSongImporter::parseHeader(SongBuilder& songBuilder) noexcept
{
    size_t lineIndex = 1U;
    const auto songName = readLines.at(lineIndex++);
    const auto comments = readLines.at(lineIndex++);

    jassert(readLines.at(lineIndex) == "===");
    ++lineIndex;     // Skips the separator ("===").

    // Reads the refresh rate, initial speed, global transposition (format: "25/3 +00").
    const auto refreshAndSpeedAndGlobalTransposition = readLines.at(lineIndex);

    const auto globalSplits = StringUtil::split(refreshAndSpeedAndGlobalTransposition, "/");
    if (globalSplits.size() != 2U) {
        songBuilder.addError("Header malformed (refresh rate).");
        return false;
    }
    const auto refreshRate = globalSplits.at(0U).getFloatValue();

    // The second split is split once again.
    const auto speedAndGlobalTranspositionSplit = StringUtil::split(globalSplits.at(1U), " ");
    if (speedAndGlobalTranspositionSplit.size() != 2U) {
        songBuilder.addError("Header malformed (initial speed).");
        return false;
    }
    const auto initialSpeed = speedAndGlobalTranspositionSplit.at(0U).getIntValue();
    globalTransposition = speedAndGlobalTranspositionSplit.at(1U).getIntValue();

    songBuilder.setSongMetaData(songName, juce::String(), juce::String(), comments);

    // Creates the PSG metadata.
    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName, initialSpeed, refreshRate);
    subsongBuilder.addPsg(Psg(PsgType::ay, PsgFrequency::psgFrequencyCPC));

    return true;
}

bool ChpSongImporter::parseTracks(SongBuilder& songBuilder) noexcept
{
    // Example:
    // #00 C-4:01 ...:.. ...:.. C-4:01 ...:.. ...:.. C-4:01 D-4:01 ...:.. C-4:01 ...:.. ...:.. C-4:01 ...:.. ...:.. C-4:01 ...:.. ...:.. C-4:01 C-4:01 C-4:02 C-4:01 D-4:02 D-4:02 D-4:02 D-4:02 D#4:02 D#4:02 D#4:02 D#4:02 E-4:02 E-4:02 E-4:02 E-4:02 F-4:02 F-4:02 F-4:02 F-4:02
    // #01 ^^^:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:.. ...:..
    // ...
    // Each line is a Track.

    // Browses each line.
    juce::String readLine;
    auto lineIndex = lineIndexOfTracks;
    while ((readLine = readLines.at(lineIndex)) != "===") {
        const auto splits = StringUtil::split(readLine, " ");

        const auto trackHeight = splits.size();
        if ((trackHeight < 1U) || (splits.at(0U)[0] != '#')) {
            songBuilder.addError("Track at line " + juce::String(lineIndex) + " is malformed.");
            return false;
        }

        // Gets the track index.
        const auto trackIndex = splits.at(0U).substring(1).getHexValue32();
        auto firstSplit = true;
        auto cellIndex = 0;
        // Browses each Track Cell.
        for (const auto& split : splits) {
            // Skips the track index.
            if (firstSplit) {
                firstSplit = false;
                continue;
            }

            // Reads the Track Cell.
            const auto success = parseTrackCell(songBuilder, split, trackIndex, cellIndex);
            if (!success) {
                jassertfalse;
                songBuilder.addError("Track index " + juce::String(trackIndex) + ", cell index " + juce::String(cellIndex) + " is malformed.");
                return false;
            }

            ++cellIndex;
        }

        // Stores the height of each Track. Useful to get the height of the Patterns.
        jassert(trackIndexToLastCellIndexUsed.find(trackIndex) == trackIndexToLastCellIndexUsed.cend());
        trackIndexToLastCellIndexUsed.insert(std::make_pair(trackIndex, static_cast<int>(trackHeight) - 1));

        ++lineIndex;
    }

    return true;
}

bool ChpSongImporter::parseTrackCell(SongBuilder& songBuilder, const juce::String& trackCell, const int trackIndex, const int cellIndex) const noexcept
{
    // Example: "^^^:.." or "B-1:05". Instrument 0 is for Glide.
    // "===:.." for "brake" that switches the current amplitude envelope on and off. Ignored in AT!

    if (trackCell.length() != 6) {
        return false;
    }

    // Extracts the note, if any.
    const auto rawNote = trackCell.substring(0, 3);
    // Empty note or Brake? If yes, nothing to do.
    if ((rawNote == "...") || (rawNote == "===")) {
        return true;
    }

    // RST?
    Cell cell;
    if (rawNote == "^^^") {
        cell = Cell::buildRst();
    } else {
        // "normal" note. Note that notes as high as C-B can be used, but they are upped to C-9 in the editor. We only limit to AT limit.
        auto noteNumber = StringUtil::stringToNoteNumber(rawNote);
        if (noteNumber < 0) {
            return false;
        }
        // Adds the global transposition, checks the limits.
        noteNumber += globalTransposition;
        noteNumber = NumberUtil::correctNumber(noteNumber, 0, CellConstants::maximumNote);

        // Builds a fake Cell (right note, wrong instrument).
        cell = Cell::build(noteNumber, 0);

        // Gets the Instrument. Instrument 0 has a special meaning: glide to note.
        OptionalInt instrumentIndex = trackCell.substring(4).getHexValue32();
        if (instrumentIndex == 0) {
            instrumentIndex = OptionalInt();         // No instrument.
            cell.setEffect(0, CellEffect::buildFromLogicalValue(Effect::pitchGlide, glideValue));
        }

        cell.setInstrument(instrumentIndex);
    }

    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();
    subsongBuilder.setCellOnTrack(trackIndex, cellIndex, cell);

    return true;
}

bool ChpSongImporter::parseLinker(SongBuilder& songBuilder) noexcept
{
    jassert(lineIndexLinker < readLines.size());
    // The Tracks must have been parsed before.
    jassert(!trackIndexToLastCellIndexUsed.empty());

    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();

    juce::String readLine;
    auto lineIndex = lineIndexLinker;
    size_t positionIndex = 0U;
    size_t loopStartIndex = 0U;
    // Browses each line. Must of the form: "#00+00 #08+00 #17+00 *". The star is optional.
    while ((readLine = readLines.at(lineIndex)) != "===") {
        const auto splits = StringUtil::split(readLine, " ");
        jassert(splits.size() >= 3U);
        if (splits.size() < 3U) {
            songBuilder.addError("Linker line " + juce::String(positionIndex) + "is malformed.");
            return false;
        }

        // Loops here? A star marks the loop.
        if ((splits.size() >= 4U) && (splits.at(3U) == "*")) {
            loopStartIndex = positionIndex;
        }

        // Reads the three Linker Cells.
        const auto trackIndexChannel0AndTransposition = extractTrackIndexAndTranspositionFromLinkerCell(splits.at(0U));
        const auto trackIndexChannel1AndTransposition = extractTrackIndexAndTranspositionFromLinkerCell(splits.at(1U));
        const auto trackIndexChannel2AndTransposition = extractTrackIndexAndTranspositionFromLinkerCell(splits.at(2U));
        if ((trackIndexChannel0AndTransposition.first < 0) || (trackIndexChannel1AndTransposition.first < 0) || (trackIndexChannel2AndTransposition.first < 0)) {
            songBuilder.addError("Linker line " + juce::String(positionIndex) + "is malformed.");
            return false;
        }

        // We can generate the Pattern. The height is not known yet, it will be later.
        const auto channel0TrackIndex = trackIndexChannel0AndTransposition.first;
        // Creates a fake pattern, to know if it is unique.
        const Pattern fakePattern({ channel0TrackIndex, trackIndexChannel1AndTransposition.first, trackIndexChannel2AndTransposition.first },
                            0, 0);
        const auto patternResult = subsongBuilder.checkRawOriginalPatternAndStoreIfDifferent(fakePattern);
        const auto patternIndex = patternResult.second;
        if (patternResult.first) {
            // Encodes the Pattern.
            constexpr auto specialTrackIndex = 0;       // No events, and no speed change.
            subsongBuilder.startNewPattern(patternIndex);
            subsongBuilder.setCurrentPatternTrackIndex(0, channel0TrackIndex);
            subsongBuilder.setCurrentPatternTrackIndex(1, trackIndexChannel1AndTransposition.first);
            subsongBuilder.setCurrentPatternTrackIndex(2, trackIndexChannel2AndTransposition.first);
            subsongBuilder.setCurrentPatternSpeedTrackIndex(specialTrackIndex);
            subsongBuilder.setCurrentPatternEventTrackIndex(specialTrackIndex);
            subsongBuilder.finishCurrentPattern();
        }

        // Sets the height of the Pattern.
        // Strangely enough, in CHP format, the height of the Pattern is the height of the FIRST Track. How logical.
        jassert(trackIndexToLastCellIndexUsed.find(channel0TrackIndex) != trackIndexToLastCellIndexUsed.cend());    // Should exist.
        auto iterator = trackIndexToLastCellIndexUsed.find(channel0TrackIndex);
        auto height = 64;
        if (iterator != trackIndexToLastCellIndexUsed.cend()) {
            height = iterator->second;
        }

        // Creates the Position too.
        subsongBuilder.startNewPosition();
        subsongBuilder.setCurrentPositionHeight(height);
        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
        subsongBuilder.setCurrentPositionTransposition(0, trackIndexChannel0AndTransposition.second);
        subsongBuilder.setCurrentPositionTransposition(1, trackIndexChannel1AndTransposition.second);
        subsongBuilder.setCurrentPositionTransposition(2, trackIndexChannel2AndTransposition.second);
        subsongBuilder.finishCurrentPosition();

        ++lineIndex;
        ++positionIndex;
    }

    subsongBuilder.setLoop(static_cast<int>(loopStartIndex), static_cast<int>(positionIndex - 1U));

    return true;
}

std::pair<int, int> ChpSongImporter::extractTrackIndexAndTranspositionFromLinkerCell(const juce::String& linkerCell) noexcept
{
    // Example : "#1A+00". Checks the sign, the Diese, and the sign.
    constexpr auto dieseIndex = 0;
    constexpr auto signIndex = 3;
    if ((linkerCell.length() < 6) || (linkerCell[dieseIndex] != '#') || ((linkerCell[signIndex] != '+') && (linkerCell[signIndex] != '-'))) {
        // Error.
        return { -1, 0 };
    }

    const auto trackIndex = linkerCell.substring(dieseIndex + 1, dieseIndex + 3).getHexValue32();
    const auto transposition = linkerCell.substring(signIndex, signIndex + 3).getIntValue();

    return { trackIndex, transposition };
}

bool ChpSongImporter::parseInstruments(SongBuilder& songBuilder) const noexcept
{
    // Example:
    //:01 FFFF 0000 000 Piano Bob Bab

    // Browses each line.
    juce::String readLine;
    auto lineIndex = lineIndexOfInstruments;
    jassert(lineIndex > 0U);
    auto foundInstrument = false;
    auto success = true;
    while (success && (readLine = readLines.at(lineIndex)).startsWithChar(':')) {
        // We should find at least one Instrument!
        foundInstrument = true;

        success = parseInstrument(songBuilder, readLine);

        ++lineIndex;
    }

    if (!foundInstrument) {
        songBuilder.addError("No instrument were found, abnormal.");
        return false;
    }

    return success;
}

bool ChpSongImporter::parseInstrument(SongBuilder& songBuilder, const juce::String& readLine) noexcept
{
    // Example:
    //:01 FFFF 0000 000 Piano Bob Bab
    //:0A FFD3 3080 00E Chan2 Beep      Chimera2 song.
    //           80                     Undocumented? Stops the noise.

    auto splits = StringUtil::split(readLine, " ");
    // Checks the validity of the line.
    if ((splits.size() < 4U) || (splits.at(1U).length() != 4) || (splits.at(2U).length() != 4) || (splits.at(3U).length() != 3)) {
        songBuilder.addError("Instrument line malformed.");
        return false;
    }

    // Gets the Instrument Number, after the ":".
    const auto instrumentIndex = splits.at(0U).substring(1).getHexValue32();
    if (instrumentIndex == 0) {
        songBuilder.addError("Instrument line is malformed.");
        return false;
    }

    // Gets the Instrument name. The remaining splits are concatenated.
    juce::String instrumentName;
    auto first = true;
    for (size_t i = 4U; i < splits.size(); ++i) {
        if (!first) {
            instrumentName += " ";
        }
        first = false;
        instrumentName += splits.at(i);
    }
    if (instrumentName.isEmpty()) {
        instrumentName = "Instrument " + juce::String(instrumentIndex);
    }

    auto minLength = 1U;

    // First split: volume and volume slide.
    auto& split = splits.at(1U);

    // Gets the volume. Only kept the most significant nibble.
    auto currentVolume = static_cast<float>(split.substring(0, 2).getHexValue32()) / 16.0F;
    // Does the volume evolves?
    const auto volumeEvolutionRaw = split.substring(2, 4).getHexValue32();
    auto volumeEvolution = 0.0F;
    auto volumeEvolutionOnlyOnce = false;
    if (currentVolume > 0) {        // A start volume of 0 means no sound, no evolution.
        if (volumeEvolutionRaw == 0) {
            // Nothing to do.
        } else if ((volumeEvolutionRaw >= 1) && (volumeEvolutionRaw <= 0x5f)) {
            // Volume slide up. 01 is up very slow. 10 is fast, more doesn't seem to go any faster, because it cannot.
            volumeEvolution = static_cast<float>(volumeEvolutionRaw) * 0.07F;      // Empirical.
        } else if ((volumeEvolutionRaw >= 0x60) && (volumeEvolutionRaw <= 0x7f)) {
            const auto up = volumeEvolutionRaw <= 0x6f;
            volumeEvolution = (static_cast<float>(up ? volumeEvolutionRaw - 0x60 : -(0x7f - volumeEvolutionRaw)) / 1.5F);
            volumeEvolutionOnlyOnce = true;
            minLength = 2;
        } else if ((volumeEvolutionRaw >= 0xa0) && (volumeEvolutionRaw <= 0xff)) {
            // Volume slide down. a0-e0 sounds actually the same (one frame).
            volumeEvolution = static_cast<float>(0x100 - volumeEvolutionRaw) * -0.07F;      // Empirical.
        } else {
            jassertfalse;   // Not managed yet: volume tremolo (0x80-0x9f).
        }
    }

    // Second split: noise and noise slide.
    split = splits.at(2U);
    // Gets the volume. Only kept the most significant nibble.
    const auto noiseValueRaw = split.substring(0, 2).getHexValue32();
    auto currentNoise = (noiseValueRaw == 0) ? 0.0F : (static_cast<float>(noiseValueRaw) / 8.0F);     // If noise present, changed from 8 bits to 5 bits.
    // Does the noise evolves?
    const auto noiseEvolutionRaw = split.substring(2, 4).getHexValue32();
    float noiseEvolution;    // NOLINT(*-init-variables)
    if ((noiseEvolutionRaw >= 1) && (noiseEvolutionRaw <= 0x7f)) {
        // Noise slide up.
        noiseEvolution = static_cast<float>(noiseEvolutionRaw) / 16.0F;
    } else if ((noiseEvolutionRaw > 0x80) && (noiseEvolutionRaw <= 0xff)) {     // 0x80 means "stop noise".
        // Noise slide down.
        noiseEvolution = -(static_cast<float>(noiseEvolutionRaw) - static_cast<float>(0x7f)) / 128.0F;
    } else {
        noiseEvolution = 0.0F;
    }

    const auto mustStopNoiseAfterIteration = (noiseEvolutionRaw == 0x80);

    // Arpeggio/vibrato/pitch?
    split = splits.at(3U);
    const auto firstEffectDigit = split.substring(0, 1).getHexValue32();
    const auto secondEffectDigit = split.substring(1, 2).getHexValue32();
    const auto thirdEffectDigit = split.substring(2, 3).getHexValue32();
    std::vector arpeggioValues = { 0, 0, 0 };
    std::vector vibratoValues = { 0 };
    auto isVibrato = false;
    auto isArpeggio = false;
    size_t arpeggioLoopIndex = 0;
    auto loopLength = 1U;
    size_t vibratoCurrentSpeed = 0U;
    size_t vibratoSpeedToReach = 1U;
    size_t vibratoCurrentIndex = 0U;       // Increases when ++currentSpeed has reached the speed to reach.

    auto pitchEveryXFrames = 0U;             // 0 = no pitch up/down.
    auto pitchValueToAdd = 0;
    auto currentPitch = 0;
    auto forceDontLoop = false;

    auto forcedPitch = 0;
    if ((firstEffectDigit == 0) && ((secondEffectDigit != 0) || (thirdEffectDigit != 0))) {
        // Arpeggio.
        auto loopOnLastArpeggioValue = false;
        arpeggioValues.at(1U) = getArpeggioFromDigit(secondEffectDigit);
        arpeggioValues.at(2U) = getArpeggioFromDigit(thirdEffectDigit);
        // If last value is 0xe or 0xf, loops on last digit (continuous arp, don't loop).
        if ((thirdEffectDigit == 0xe) || (thirdEffectDigit == 0xf)) {
            loopOnLastArpeggioValue = true;
            arpeggioLoopIndex = 2U;
        }

        isArpeggio = true;
        loopLength = loopOnLastArpeggioValue ? 1 : static_cast<unsigned int>(arpeggioValues.size());
        minLength = loopLength;      // Makes sure at least the three lines of the arpeggio are played.
    } else if (firstEffectDigit == 1) {
        // Vibrato? Only according to these strange data.
        if ((secondEffectDigit > 0) && (thirdEffectDigit > 0) && (thirdEffectDigit != 8)) {
            const auto positiveStartingVibrato = (thirdEffectDigit <= 7);
            auto vibratoValue = positiveStartingVibrato ? thirdEffectDigit : thirdEffectDigit - 8;   // Scales from 9-F to 1-7.
            // Just a hack to emulate a large value, can't figure how it works.
            // "After the war" has 123, and "3" does not need this applied.
            if (vibratoValue > 3) {
                vibratoValue = vibratoValue * (vibratoValue + 2);
            }

            // Positive starting vibrato.
            if (positiveStartingVibrato) {
                vibratoValues = { 0, vibratoValue, 0, -vibratoValue };
            } else {
                // Negative starting vibrato.
                vibratoValues = { 0, -vibratoValue, 0, vibratoValue };
            }

            vibratoSpeedToReach = static_cast<size_t>(secondEffectDigit);
            isVibrato = true;
            loopLength = static_cast<unsigned int>(vibratoValues.size() * vibratoSpeedToReach);
            minLength = loopLength;      // Makes sure the vibrato is fully played.
        } else if ((secondEffectDigit == 0) && (thirdEffectDigit == 8)) {       // Don't ask...
            forcedPitch = -1;
        } else if ((secondEffectDigit == 0) && (thirdEffectDigit != 8)) {
            // Add Y to pitch every frame, with a limit in the pitch range. But the limit cannot be done, no such thing with AT3.
            const auto up = (thirdEffectDigit < 8);
            pitchValueToAdd = (thirdEffectDigit - (up ? 0 : 8)) * (up ? 1 : -1);      // If > 9, it means down.
            // A coef seems applied (Fernandez, drums B and C, and drop on instr 6, up on instr 8).
            pitchValueToAdd = static_cast<int>(static_cast<double>(pitchValueToAdd) * 1.5);
            pitchEveryXFrames = 1;
        } else if ((secondEffectDigit != 0) && ((thirdEffectDigit == 0) || (thirdEffectDigit == 8))) {
            // Pitch up/down of one each X frames, with a limit in the pitch range. But the limit cannot be done, no such thing with AT3.
            const auto up = (thirdEffectDigit == 0);
            pitchEveryXFrames = static_cast<unsigned int>(secondEffectDigit);
            pitchValueToAdd = up ? 1 : -1;
        }
    }

    // If starting with a single noise, make sure the arpeggio/vibrato don't loop on it.
    if (mustStopNoiseAfterIteration && (isVibrato || isArpeggio)) {
        ++minLength;
    }

    // Corner case (Fernandez sound 6): flat sound but with pitch. We need to hear a bit about the pitch!
    if (juce::exactlyEqual(volumeEvolution, 0.0F) && (pitchEveryXFrames != 0)) {
        minLength = 32;         // Arbitrary.
        forceDontLoop = true;   // Else, will sounds ugly at the end.
    }

    // Builds the sound.
    songBuilder.startNewPsgInstrument(instrumentIndex);
    songBuilder.setCurrentInstrumentName(instrumentName);

    auto soundFinished = false;
    auto cellIndex = 0U;
    auto arpeggioIndex = 0U;
    while (!soundFinished && (cellIndex < 256U)) {        // Security.
        // Arpeggio.
        auto arpeggio = 0;
        if (isArpeggio) {
            arpeggio = arpeggioValues.at(arpeggioIndex);
            if (++arpeggioIndex >= arpeggioValues.size()) {
                arpeggioIndex = static_cast<unsigned int>(arpeggioLoopIndex);
            }
        }

        // Makes sure we don't encode out of limit values!
        const auto encodedVolume = NumberUtil::correctNumber(static_cast<int>(currentVolume), PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);
        const auto encodedNoise = NumberUtil::correctNumber(static_cast<int>(currentNoise), PsgValues::minimumNoise, PsgValues::maximumNoise);

        songBuilder.startNewPsgInstrumentCell();
        songBuilder.setCurrentPsgInstrumentCellVolume(encodedVolume);
        songBuilder.setCurrentPsgInstrumentCellNoise(encodedNoise);
        songBuilder.setCurrentPsgInstrumentCellPrimaryArpeggio(arpeggio);
        songBuilder.setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::softOnly);

        currentVolume += volumeEvolution;
        if (mustStopNoiseAfterIteration) {
            currentNoise = 0;
        } else {
            currentNoise += noiseEvolution;
        }

        // Pitch. Forced/vibrato?
        auto pitch = (forcedPitch != 0) ? forcedPitch : vibratoValues.at(vibratoCurrentIndex % vibratoValues.size());
        if (++vibratoCurrentSpeed >= vibratoSpeedToReach) {
            ++vibratoCurrentIndex;
            vibratoCurrentSpeed = 0;
        }
        // Override with pitch effect.
        if (pitchEveryXFrames != 0) {
            if (((cellIndex + 1) % pitchEveryXFrames) == 0) {
                currentPitch += pitchValueToAdd;
            }
            pitch = currentPitch;
        }

        songBuilder.setCurrentPsgInstrumentCellPrimaryPitch(pitch);

        ++cellIndex;

        if (volumeEvolutionOnlyOnce) {
            volumeEvolution = 0.0F;
        }

        // Detects the end of the sound. The highest limit is higher than the normal boundary, to be sure the latter will be reached.
        const auto endVolumeEvolution = juce::exactlyEqual(volumeEvolution, 0.0F)
                                        || ((volumeEvolution > 0.0F) && (currentVolume >= 16.0F))           // End of volume slide up?
                                        || ((volumeEvolution < 0.0F) && (currentVolume < 1.0F));            // End of volume slide down?
        const auto endNoiseEvolution = juce::exactlyEqual(noiseEvolution, 0.0F)
                                       || ((noiseEvolution > 0.0F) && (noiseEvolution >= 32.0F))            // End of noise slide up?
                                       || ((noiseEvolution < 0.0F) && (noiseEvolution < 1.0F));             // End of noise slide down?
        soundFinished = (cellIndex >= minLength) && endVolumeEvolution && endNoiseEvolution;

        songBuilder.finishCurrentPsgInstrumentCell();
    }

    // By default, the sound always loops on the last cell, unless the last cell is a very low volume.
    const auto lastIndex = static_cast<int>(cellIndex) - 1;
    auto startIndex = lastIndex;
    const auto isLooping = (currentVolume > 1.0F) && !forceDontLoop;
    if (isLooping && (isVibrato || isArpeggio) && juce::exactlyEqual(volumeEvolution, 0.0F)) {
        // But if there was a vibrato/arpeggio and the volume is continuous, loops on it.
        startIndex = std::max(0, lastIndex + 1 - static_cast<int>(loopLength));
    }

    songBuilder.setCurrentPsgInstrumentMainLoop(startIndex, lastIndex, isLooping);

    songBuilder.finishCurrentInstrument();

    return true;
}

int ChpSongImporter::getArpeggioFromDigit(const int arpeggio) noexcept
{
    if (arpeggio < 0xd) {
        return arpeggio;
    }

    // Other values have special meaning. How clever!! Note that the "locked" arpeggio is not managed here.
    if (arpeggio == 0xd) {
        return 24;
    }
    if (arpeggio == 0xe) {
        return -24;
    }

    return -12;
}

}   // namespace arkostracker
