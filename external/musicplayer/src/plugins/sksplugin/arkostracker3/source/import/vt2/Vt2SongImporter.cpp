#include "Vt2SongImporter.h"

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../business/song/tool/optimizers/SongOptimizer.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../song/cells/CellConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StringUtil.h"
#include "Vt2InstrumentBuilder.h"

namespace arkostracker 
{

const juce::String Vt2SongImporter::headerTag = "[Module]";    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

Vt2SongImporter::Vt2SongImporter() :
        readLines(),
        usedPatternIndexes(),
        patternCount(),
        patternIndexToHeight(),
        positionToPatternIndex()
{
}


// SongImporter method implementations.
// =======================================

bool Vt2SongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Checks the header.
    if (inputStream.getTotalLength() < headerTagSize) {
        return false;
    }

    char readHeader[headerTagSize]; // NOLINT(*-avoid-c-arrays)
    inputStream.read(readHeader, headerTagSize);            // NOLINT(*)
    juce::CharPointer_UTF8 pointer(readHeader);                 // NOLINT(*)
    const juce::String readString(pointer, static_cast<size_t>(headerTagSize));

    return (readString == headerTag);
}

ImportedFormat Vt2SongImporter::getFormat() noexcept
{
    return ImportedFormat::vt2;
}

std::unique_ptr<SongImporter::Result> Vt2SongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    const auto memoryBlock = MemoryBlockUtil::fromInputStream(inputStream);
    const auto songBuilder = parse(memoryBlock);
    jassert(songBuilder != nullptr);

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    // Some useless Instruments may have been created. Optimizes them.
    const auto& generatedSong = songAndReport.first;
    std::unique_ptr<Song> optimizedSong;
    if (generatedSong != nullptr) {
        optimizedSong = SongOptimizer::optimize(*generatedSong, false, true, true, false, false,
                                false, true, false, false, false,
                                generatedSong->getSubsongIds());
    }
    return std::make_unique<SongImporter::Result>(std::move(optimizedSong), std::move(songAndReport.second));
}


// =======================================

std::unique_ptr<SongBuilder> Vt2SongImporter::parse(const juce::MemoryBlock& memoryBlock) noexcept
{
    auto songBuilder = std::make_unique<SongBuilder>();

    songBuilder->startNewSubsong(0);

    // Gets the lines from the MemoryBlock, once and for all.
    readLines = MemoryBlockUtil::extractStrings(memoryBlock);
    // Reads the header, the Ornaments (converted to Arpeggios), the "Samples" (converted to PSG Instruments), and the Patterns.
    auto success = parseHeader(*songBuilder);
    success = success && parseOrnaments(*songBuilder);
    success = success && parseSamples(*songBuilder);
    success = success && parsePatterns(*songBuilder);
    jassert(success);

    songBuilder->finishCurrentSubsong();

    return songBuilder;
}

std::vector<juce::String> Vt2SongImporter::extractSectionLines(const juce::String& sectionString) const noexcept
{
    std::vector<juce::String> lines;

    // Looks for the tag.
    auto iterator = readLines.cbegin();
    auto found = false;
    while (!found && (iterator != readLines.cend())) {
        found = iterator->startsWith(sectionString);

        ++iterator;         // Done in all cases, so that the section is also skipped.
    }

    if (!found) {
        return lines;
    }

    // Adds the lines as long as no empty line is found, or one starting with "[" (security).
    auto mustContinue = true;
    while (mustContinue && (iterator != readLines.cend())) {
        const auto& line = *iterator;
        mustContinue = (line.trim().isNotEmpty() && !line.startsWith("["));

        // If the line is relevant, stores it.
        if (mustContinue) {
            lines.push_back(line);

            ++iterator;
        }
    }

    jassert(!lines.empty());

    return lines;
}

juce::String Vt2SongImporter::extractValue(const std::vector<juce::String>& lines, const juce::String& key) noexcept
{
    const auto keyWithSeparator = key + "=";
    for (const auto& line : lines) {
        if (line.startsWith(keyWithSeparator)) {
            // Key found! Returns the value, just behind.
            return line.substring(keyWithSeparator.length());
        }
    }

    // No value found.
    return { };
}

bool Vt2SongImporter::parseHeader(SongBuilder& songBuilder) noexcept
{
    usedPatternIndexes.clear();
    positionToPatternIndex.clear();

    // Extracts the header lines.
    auto headerLines = extractSectionLines(headerTag);

    const auto songTitle = extractValue(headerLines, "Title");
    const auto author = extractValue(headerLines, "Author");
    auto chipFrequency = extractValue(headerLines, "ChipFreq").getIntValue();
    if (chipFrequency == 0) {       // Fallback for VT1 export.
        chipFrequency = static_cast<int>(PsgFrequency::psgFrequencySpectrum);
    }
    auto speed = extractValue(headerLines, "Speed").getIntValue();
    if (speed == 0) {       // Sanity check.
        speed = 6;
    }
    const auto playOrder = extractValue(headerLines, "PlayOrder");

    songBuilder.setSongMetaData(songTitle, author, author, "Converted by Arkos Tracker 3");
    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName, speed);

    // Creates the PSG metadata.
    subsongBuilder.addPsg(Psg(PsgType::ay, chipFrequency, PsgFrequency::defaultReferenceFrequencyHz,
                              PsgFrequency::defaultSamplePlayerFrequencyHz, PsgMixingOutput::ABC));

    // Extracts the play order (example : "0, 1, 0, 1, L2, 1, 3, 4", etc. L for Loop).
    const auto indexesAndLoopTo = extractIndexesAndLoopTo(playOrder);
    patternCount = static_cast<int>(indexesAndLoopTo.first.size());
    for (const auto patternIndex : indexesAndLoopTo.first) {
        // Already added? If yes, don't do it.
        if (usedPatternIndexes.find(patternIndex) == usedPatternIndexes.cend()) {
            // Stores the Pattern index, for later.
            usedPatternIndexes.insert(patternIndex);

            // Creates a Pattern. For now, the height and SpeedTrack are generic, they will be found later.
            const auto trackIndex = patternIndex * PsgValues::channelCountPerPsg;
            // Encodes the Pattern.
            const auto specialTrackIndex = patternIndex;
            subsongBuilder.startNewPattern(patternIndex);
            subsongBuilder.setCurrentPatternTrackIndex(0, trackIndex);
            subsongBuilder.setCurrentPatternTrackIndex(1, trackIndex + 1);
            subsongBuilder.setCurrentPatternTrackIndex(2, trackIndex + 2);
            subsongBuilder.setCurrentPatternSpeedTrackIndex(specialTrackIndex);
            subsongBuilder.setCurrentPatternEventTrackIndex(specialTrackIndex);
            subsongBuilder.finishCurrentPattern();
        }

        // Stores the Pattern index for a position index, to easily insert the height and SpeedTrack index later.
        positionToPatternIndex.emplace_back(patternIndex);
    }
    // Sets the end and loop indexes.
    subsongBuilder.setLoop(indexesAndLoopTo.second, patternCount - 1);

    const auto success = (patternCount > 0);
    if (!success) {
        songBuilder.addError("Unable to parse the header.");
    }

    return success;
}

std::pair<std::vector<int>, int> Vt2SongImporter::extractIndexesAndLoopTo(const juce::String& string) noexcept
{
    std::vector<int> indexes;

    // Extracts the play order (example : "0, 1, 0, 1, L2, 1, 3, 4", etc. L for Loop).
    auto splitStrings = StringUtil::split(string, ",");
    // Converts to numbers, watching out for "L".
    auto loopToIndex = 0;
    auto index = 0;
    for (auto splitString : splitStrings) {
        // Starts with "L"?
        splitString = splitString.trim();
        if (splitString.startsWithChar('L')) {
            // Loops here!
            loopToIndex = index;
            splitString = splitString.substring(1); // Skips the L.
        }

        indexes.push_back(splitString.getIntValue());

        ++index;
    }

    return { indexes, loopToIndex };
}

bool Vt2SongImporter::parseOrnaments(SongBuilder& songBuilder) const noexcept
{
    // Expression 0 will be set by AT.
    for (auto ornamentIndex = 1; ornamentIndex <= maximumOrnamentIndex; ++ornamentIndex) {
        // Extracts the ornament line, starting at "Ornament1".
        const auto headerLines = extractSectionLines("[Ornament" + juce::String(ornamentIndex) + "]");
        // Didn't find this ornament? Then skips it.
        if (headerLines.empty()) {
            continue;
        }
        // There should only be one line!
        if (headerLines.size() != 1U) {
            songBuilder.addError("Ornament " + juce::String(ornamentIndex) + " is invalid.");
            return false;
        }

        // Reads the first line only.
        const auto indexesAndLoopTo = extractIndexesAndLoopTo(headerLines.at(0U));
        const auto& values = indexesAndLoopTo.first;
        const auto arpeggioLength = static_cast<int>(values.size());
        // Ignores empty (shouldn't happen) or ornament with only one 0.
        if ((values.empty() || ((values.size() == 1U) && (values.at(0U) == 0)))) {
            continue;
        }

        // Silently corrects the loop index, if needed.
        const auto loopIndex = juce::jmin(indexesAndLoopTo.second, arpeggioLength - 1);

        // Creates and fills the Arpeggio.
        songBuilder.startNewExpression(true, ornamentIndex, "Arpeggio " + juce::String(ornamentIndex));

        for (const auto value : values) {
            songBuilder.addCurrentExpressionValue(value);
        }
        songBuilder.setCurrentExpressionLoop(loopIndex, arpeggioLength - 1);

        songBuilder.finishCurrentExpression();
    }

    return true;
}

bool Vt2SongImporter::parseSamples(SongBuilder& songBuilder) const noexcept
{
    // Browses each "sample" section.
    for (auto sampleIndex = 1; sampleIndex < maximumSampleIndex; ++sampleIndex) {
        auto headerLines = extractSectionLines("[Sample" + juce::String(sampleIndex) + "]");
        // There should only be at least one line!
        if (headerLines.empty()) {
            songBuilder.addError("Sample " + juce::String(sampleIndex) + " is empty.");
            return false;
        }

        songBuilder.startNewPsgInstrument(sampleIndex);

        Vt2InstrumentBuilder vt2InstrumentBuilder(songBuilder, "Sample " + juce::String(sampleIndex));

        // Adds each parsed line.
        for (const auto& line : headerLines) {
            vt2InstrumentBuilder.addInstrumentLine(line);
        }
        // Then creates the Instrument itself.
        const auto success = vt2InstrumentBuilder.parseEnded();
        if (!success) {
            songBuilder.addError("Unable to create Instrument " + juce::String(sampleIndex));
            return false;
        }
    }

    return true;
}

bool Vt2SongImporter::parsePatterns(SongBuilder& songBuilder) noexcept
{
    jassert(!usedPatternIndexes.empty());

    patternIndexToHeight.clear();
    //patternIndexToSpeedTrackIndex.clear();

    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();

    // Converts all the Patterns that were found in the linker.
    for (const auto patternIndex : usedPatternIndexes) {
        std::vector<TrailingEffectContext> channelContexts;
        channelContexts.reserve(3U);
        channelContexts.emplace_back();
        channelContexts.emplace_back();
        channelContexts.emplace_back();

        auto patternLines = extractSectionLines("[Pattern" + juce::String(patternIndex) + "]");
        // Skips non-empty Patterns (a rather abnormal case).
        if (patternLines.empty()) {
            continue;
        }

        // The height is now known. But it is within limit? If no, restricts it.
        auto height = static_cast<int>(patternLines.size());
        const auto maximumHeight = TrackConstants::maximumCapacity;
        if (height > maximumHeight) {
            songBuilder.addWarning("Pattern " + juce::String(patternIndex) + " is too high.");
            height = maximumHeight;
        }
        patternIndexToHeight.insert(std::make_pair(patternIndex, height));

        // Reads each line for the Pattern.
        auto lineIndex = 0;
        for (const auto& line : patternLines) {
            auto success = parsePatternLine(songBuilder, channelContexts, lineIndex, line, patternIndex);
            if (!success) {
                jassertfalse;
                return false;
            }
            ++lineIndex;
        }
    }

    // We can now fill the SpeedTrack index and height of all the Patterns.
    fillPatternMetadata(subsongBuilder);

    return true;
}

bool Vt2SongImporter::parsePatternLine(SongBuilder& songBuilder, std::vector<TrailingEffectContext>& channelContexts, int lineIndex,
                                       const juce::String& lineToParse, int patternIndex) noexcept
{
    // Example:
    // ....|..|A-4 1F1E ....|--- .... ....|A-4 1F3E ....
    // ....|..|R-- .... ....|--- .... ....|--- .... ....
    // ....|..|--- .... ....|--- .... ....|--- .... ....
    // HARD NO NOT SSVA effx

    if (lineToParse.length() < 49) {
        jassertfalse;
        songBuilder.addError("Wrong format in the line " + juce::String(lineIndex) + " of the Pattern " + juce::String(patternIndex) + ".");
        return false;
    }

    // For now, the Hardware and Noise columns are ignored. Maybe in a later version?

    // Parses the three channels.
    constexpr auto trackCellSize = 14;       // Includes the separator.
    auto charIndex = 8;
    auto success = true;
    for (auto channelIndex = 0; channelIndex < 3; ++channelIndex, charIndex += trackCellSize) {
        const auto trackIndex = patternIndex * 3 + channelIndex;
        success = success && parseTrackCell(songBuilder, patternIndex, channelContexts.at(static_cast<size_t>(channelIndex)), lineIndex,
                                  lineToParse.substring(charIndex, charIndex + trackCellSize - 1), trackIndex);        // -1 to remove the separator.
    }

    return success;
}

bool Vt2SongImporter::parseTrackCell(SongBuilder& songBuilder, int patternIndex, TrailingEffectContext& channelContext, int lineIndex,
                                     const juce::String& cellToParse, int trackIndex) noexcept
{
    // Example:
    // A-4 1F1E ....
    // --- .... ....
    // NOT SeAV effx
    //      envelope
    //     Sound: 1-9 + A-V

    jassert(cellToParse.length() >= 13);

    auto& subsongBuilder = songBuilder.getCurrentSubsongBuilder();

    // Converts the note and octave to a note number.
    // -1 if there is no note.
    auto charIndex = 0;
    const auto noteString = cellToParse.substring(charIndex, charIndex + 3);
    // Maybe a RST?
    auto isRst = (noteString == "R--");
    const auto noteNumber = isRst ? CellConstants::rstNote : StringUtil::stringToNoteNumber(noteString);
    if (noteNumber > CellConstants::maximumNote) {
        return false;
    }
    charIndex += 4;

    // Gets the Instrument, if any. It can be absent, in which case the latest instrument must be used.
    OptionalInt instrumentNumber;
    OptionalInt lastInstrumentNumberUsed;
    if (isRst) {
        instrumentNumber = CellConstants::rstInstrument;
        // This does not change the last instrument used.
        lastInstrumentNumberUsed = channelContext.getCurrentInstrumentIndex();
    } else {
        const auto instrumentString = cellToParse.substring(charIndex, charIndex + 1);
        if (instrumentString != ".") {
            // Instrument is 1-9 or A-V.
            auto instrumentChar = static_cast<char>(instrumentString[0]);
            if ((instrumentChar >= '1') && (instrumentChar <= '9')) {
                instrumentNumber = instrumentChar - '1' + 1;
            } else if ((instrumentChar >= 'A') && (instrumentChar <= 'V')) {
                instrumentNumber = instrumentChar - 'A' + 1;
            } else {
                instrumentNumber = 1;
                subsongBuilder.addWarning("Unable to parse the instrument in cell: " + instrumentString);
            }
        } else {
            // No Instrument? Then use the latest Instrument. If none, use 1 (that's how the VT sounds).
            instrumentNumber = channelContext.getCurrentInstrumentIndex();
        }
        lastInstrumentNumberUsed = instrumentNumber;
    }
    channelContext.setCurrentInstrumentIndex(lastInstrumentNumberUsed);
    charIndex += 2;

    // Gets the Arpeggio, if any (-1 if none).
    const auto arpeggioString = cellToParse.substring(charIndex, charIndex + 1);
    const auto arpeggio = (arpeggioString == ".") ? -1 : arpeggioString.getHexValue32();
    ++charIndex;

    // Gets the volume, if any (-1 if none).
    const auto volumeString = cellToParse.substring(charIndex, charIndex + 1);
    const auto volume = (volumeString == ".") ? -1 : volumeString.getHexValue32();
    charIndex += 2;

    // Creates the Cell.
    Cell cell;
    if ((noteNumber >= 0) && instrumentNumber.isPresent()) {
        cell.setNote(Note::buildNote(noteNumber));
        cell.setInstrument(instrumentNumber);
        channelContext.declareNote();
    }

    // Effect. Only if not RST.
    auto success = true;
    if (!isRst) {
        // Volume effect?
        if (volume >= 0) {
            const auto contextResult = channelContext.declareEffect(TrailingEffect::volume, volume);
            if (contextResult.first == TrailingEffect::volume) {
                cell.addEffect(Effect::volume, contextResult.second);
            }
        }

        // Arpeggio?
        if (arpeggio >= 0) {
            // If not already declared, consider we want to stop the arp (see track 0 of "nnq_skrju" song).
            const auto arpeggioToUse = (!songBuilder.doesExpressionExist(true, arpeggio)) ? 0 : arpeggio;
            const auto contextResult = channelContext.declareEffect(TrailingEffect::arpeggioTable, arpeggioToUse);
            if (contextResult.first == TrailingEffect::arpeggioTable) {
                cell.addEffect(Effect::arpeggioTable, contextResult.second);
            }
        }

        // Parses the effect column.
        const auto effectColumn = cellToParse.substring(charIndex);
        parseEffectColumn(subsongBuilder, patternIndex, effectColumn, cell, lineIndex);
    }

    subsongBuilder.setCellOnTrack(trackIndex, lineIndex, cell);

    return success;
}

void Vt2SongImporter::parseEffectColumn(SubsongBuilder& subsongBuilder, int patternIndex, const juce::String& effectColumn, Cell& cell, int cellIndex) noexcept
{
    // Silently refuses malformed column.
    jassert(effectColumn.length() >= 4);
    if (effectColumn.length() < 4) {
        subsongBuilder.addWarning("Effect column malformed.");
        return;
    }

    const auto effectString = effectColumn.substring(0, 1);
    const auto valueString = effectColumn.substring(1, 4);
    static constexpr auto pitchMultiplier = 14.0;
    static constexpr auto glideMultiplier = 360.0;
    if (effectString == ".") {
        // No effect.
    } else if (effectString == "1") {
        // Pitch down.
        managePitchOrGlideEffect(valueString, Effect::fastPitchDown, pitchMultiplier, cell);
    } else if (effectString == "2") {
        // Pitch up.
        managePitchOrGlideEffect(valueString, Effect::fastPitchUp, pitchMultiplier, cell);
    } else if (effectString == "3") {
        // Glide.
        managePitchOrGlideEffect(valueString, Effect::pitchGlide, glideMultiplier, cell);
    } else if (effectString == "B") {
        // Break. This may create a new Speed Track.
        const auto speed = static_cast<unsigned int>(valueString.substring(1, 3).getHexValue32()) & 0xffU;   // Keeps only the second and third characters. For example: ".10" -> "10".
        if (speed != 0U) {            // Silently ignores 0 speed.
            declareSpeed(subsongBuilder, patternIndex, static_cast<int>(speed), cellIndex);
        }
    } else {
        // Unsupported effect.
        subsongBuilder.addWarning("Unsupported effect: " + effectString);
    }
}

void Vt2SongImporter::managePitchOrGlideEffect(const juce::String& valueString, Effect effect, double multiplier, Cell& cell) noexcept
{
    jassert(valueString.length() >= 3);

    const auto firstDigit = valueString.substring(0, 1).getIntValue();
    if (firstDigit == 0) {
        return;
    }
    const auto secondAndThirdDigits = valueString.substring(1, 3).replaceCharacter('.', '0').getIntValue();

    // The digit is the "speed" in frames. We don't manage this, so we invert it and use it as a ratio.
    const auto finalSpeed = static_cast<int>(static_cast<double>(secondAndThirdDigits) * multiplier / static_cast<double>(firstDigit));
    cell.addEffect(effect, finalSpeed);
}

void Vt2SongImporter::declareSpeed(SubsongBuilder& subsongBuilder, int patternIndex, int speed, int cellIndex) noexcept
{
    jassert((speed > 0) && (speed <= 255));

    // Sets the Cell on the current Speed Track.
    subsongBuilder.setSpecialCellOnTrack(true, patternIndex, cellIndex, SpecialCell::buildSpecialCell(speed));
}

void Vt2SongImporter::fillPatternMetadata(SubsongBuilder& subsongBuilder) noexcept
{
    jassert(!positionToPatternIndex.empty());

    // Creates the positions.
    for (auto patternIndex : positionToPatternIndex) {
        subsongBuilder.startNewPosition();

        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);

        // Any encoded height?
        const auto heightIterator = patternIndexToHeight.find(patternIndex);
        const auto height = heightIterator != patternIndexToHeight.cend() ? heightIterator->second : TrackConstants::defaultPositionHeight;
        subsongBuilder.setCurrentPositionHeight(height);

        subsongBuilder.finishCurrentPosition();
    }
}


}   // namespace arkostracker

