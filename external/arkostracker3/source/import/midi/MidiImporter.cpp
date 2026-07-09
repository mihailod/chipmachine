#include "MidiImporter.h"

#include "../../business/instrument/GenericInstrumentGenerator.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/PsgValues.h"
#include "../../utils/SpeedUtil.h"
#include "MidiTrackReader.h"

namespace arkostracker 
{

MidiImporter::MidiImporter() noexcept :
        songBuilder(),
        importFirstPassReturnData()
{
}


// SongImporter method implementations.
// =======================================

ConfigurationType MidiImporter::requireConfiguration() const noexcept
{
    return ConfigurationType::midi;
}

ImportFirstPassReturnData MidiImporter::getReturnData() const noexcept
{
    return importFirstPassReturnData;
}

bool MidiImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Tries to open the midi song.
    auto midiFile = loadMidiFile(inputStream);
    return (midiFile != nullptr);
}

std::unique_ptr<SongImporter::Result> MidiImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept
{
    auto midiFile = loadMidiFile(inputStream);
    // Security, but this have been tested before.
    if (midiFile == nullptr) {
        jassertfalse;       // Shouldn't happen.
        return nullptr;
    }

    songBuilder = std::make_unique<SongBuilder>();

    parse(inputStream, *midiFile, configuration.getConfigurationForMidi());

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    return std::make_unique<Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}


// =======================================

std::unique_ptr<juce::MidiFile> MidiImporter::loadMidiFile(juce::InputStream& inputStream) noexcept
{
    // Loads the midi file.
    juce::MidiFile loadedMidiFile;
    const auto success = loadedMidiFile.readFrom(inputStream);
    if (!success) {
        return nullptr;
    }

    // Should be a Tick format, not SMTP!
    const auto timeFormat = loadedMidiFile.getTimeFormat();
    if (timeFormat <= 0) {
        return nullptr;
    }

    return std::make_unique<juce::MidiFile>(loadedMidiFile);
}

void MidiImporter::parse(juce::InputStream& inputStream, const juce::MidiFile& midiFile, const MidiConfiguration& midiConfiguration) noexcept
{
    // MIDI format 0 is forbidden.
    const auto success = checkIfFormatOk(inputStream);
    if (!success) {
        songBuilder->addError("Midi format 0 is forbidden! Too messy for a good result! Export your MIDI to Format 1 instead.");
        return;
    }

    const auto importVelocities = midiConfiguration.isImportVelocities();
    MidiTrackReader midiTrackReader(midiFile.getTimeFormat(), midiConfiguration.getPpq(), midiConfiguration.getDrumChannel(), importVelocities);

    const auto midiTrackCount = midiFile.getNumTracks();

    // Generates all the Cell lists, one per Midi Track.
    std::vector<std::map<int, Cell>> cellIndexToCellsForEachMidiTrack;
    for (auto midiTrackIndex = 0; midiTrackIndex < midiTrackCount; ++midiTrackIndex) {
        // Reads the Midi Track.
        const auto* midiMessageSequence = midiFile.getTrack(midiTrackIndex);
        if (midiMessageSequence == nullptr) {
            jassertfalse;           // Shouldn't happen.
            songBuilder->addError("Midi track " + juce::String(midiTrackIndex) + " has no data, abnormal.");
            return;
        }

        // The track may be empty, in which case it may be "conductor", that is, no notes but only time signature.
        const auto conductorTrack = !hasMidiTrackNotes(midiFile, midiTrackIndex);

        // Converts it to Cells.
        auto cellIndexToCells = midiTrackReader.readMidiTrack(*midiMessageSequence, conductorTrack);

        // Stores them, even if a conductor Track. After all, the user may want to load it anyway.
        cellIndexToCellsForEachMidiTrack.push_back(cellIndexToCells);
    }

    // Mixes the Tracks, keeps only the ones that are wanted.
    const auto& channelMixerKeeper = midiConfiguration.getChannelMixerKeeper();
    mixTracks(cellIndexToCellsForEachMidiTrack, channelMixerKeeper);
    keepTracks(cellIndexToCellsForEachMidiTrack, channelMixerKeeper);

    // Now creates the Song itself from the kept Tracks.
    const auto originalPpq = findOriginalSongPpq(midiFile);
    generateSongFromMidiCells(midiTrackReader, cellIndexToCellsForEachMidiTrack, importVelocities, originalPpq, midiConfiguration.getPpq());

    // We can create the "return data" object to show in the Import Panel (if not configuration was given).
    auto trackCountResult = findTrackCount(midiFile);
    importFirstPassReturnData = ImportFirstPassReturnData::buildForMidi(trackCountResult.first, originalPpq, trackCountResult.second);
}

bool MidiImporter::hasMidiTrackNotes(const juce::MidiFile& midiFile, const int midiTrackIndex) noexcept
{
    jassert(midiTrackIndex < midiFile.getNumTracks());

    const auto* midiMessageSequence = midiFile.getTrack(midiTrackIndex);
    if (midiMessageSequence == nullptr) {
        jassertfalse;           // Shouldn't happen.
        return false;
    }

    // Finds the first midi track note.
    return std::any_of(midiMessageSequence->begin(), midiMessageSequence->end(), [](const juce::MidiMessageSequence::MidiEventHolder* midiEventHolder) {
        const auto& midiMessage = midiEventHolder->message;
        return midiMessage.isNoteOn();
    });
}

bool MidiImporter::checkIfFormatOk(juce::InputStream& inputStream) noexcept
{
    auto success = inputStream.setPosition(0);
    jassert(success);       // Unable to rewind!

    // Checks the header. If not present, don't bother searching for it (these chunks can be anywhere).
    constexpr juce::int64 sizeToRead = 4 + 4 + 2;           // Mthd + chunk length + format.
    const auto memoryBlock = MemoryBlockUtil::fromInputStream(inputStream, sizeToRead);
    // Is the first chunk the header?
    const auto tag = MemoryBlockUtil::extractString(memoryBlock, 0, 4, success);
    if (tag != "MThd") {
        return true;                // Not the right header? Then give up. It's all right...
    }

    // Reads the format number.
    const auto formatNumber = MemoryBlockUtil::extractUnsignedWord(memoryBlock, 4 + 4, success, true);
    if (!success) {
        jassertfalse;           // A bit strange...
        return true;
    }

    return (formatNumber > 0);
}

void MidiImporter::mixTracks(std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack, const ChannelMixerKeeper& channelMixerKeeper) noexcept
{
    for (const auto& sourceAndDestinationTracks : channelMixerKeeper.getTrackMixes()) {
        const auto sourceTrackIndex = sourceAndDestinationTracks.first;
        const auto destinationTrackIndex = sourceAndDestinationTracks.second;
        // Same source/destination? Not really normal, but don't do anything.
        if (sourceTrackIndex == destinationTrackIndex) {
            jassertfalse;
            continue;
        }

        const auto& sourceTrack = cellIndexToCellsForEachMidiTrack.at(static_cast<size_t>(sourceTrackIndex));
        auto& destinationTrack = cellIndexToCellsForEachMidiTrack.at(static_cast<size_t>(destinationTrackIndex));

        // Browses the source Track.
        for (const auto& sourceCellIndexAndCell : sourceTrack) {
            const auto& sourceCell = sourceCellIndexAndCell.second;
            jassert(sourceCell.isNoteAndInstrument());

            // Overwrites the destination.
            const auto cellIndex = sourceCellIndexAndCell.first;
            destinationTrack[cellIndex] = sourceCell;
        }
    }
}

void MidiImporter::keepTracks(std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack, const ChannelMixerKeeper& channelMixerKeeper) noexcept
{
    const auto& tracksToKeep = channelMixerKeeper.getTrackIndexesToKeep();

    // Reverse-iterates the source tracks to remove them easily.
    for (auto trackIndex = static_cast<int>(cellIndexToCellsForEachMidiTrack.size()) - 1; trackIndex >= 0; --trackIndex) {
        // Should we keep it?
        if (tracksToKeep.find(trackIndex) == tracksToKeep.cend()) {
            // Removes it!
            cellIndexToCellsForEachMidiTrack.erase(cellIndexToCellsForEachMidiTrack.begin() + trackIndex);
        }
    }
}

void MidiImporter::generateSongFromMidiCells(const MidiTrackReader& midiTrackReader, const std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack,
                                             const bool importVelocities, const int originalPpq, const int ppq) const noexcept
{
    // Asks about the heights of each Pattern.
    const auto patternHeights = midiTrackReader.buildPatternHeights();

    const auto inputChannelCount = static_cast<int>(cellIndexToCellsForEachMidiTrack.size());
    const auto psgCount = PsgValues::getPsgCount(inputChannelCount);
    const auto outputChannelCount = PsgValues::getChannelCount(psgCount);
    auto currentSpeed = -1;       // Unreachable value to force a refresh.

    // Builds the output Song with the right count of PSG and Subsong.
    songBuilder->setSongMetaData(juce::translate("Untitled"), juce::translate("Unknown"), juce::translate("Unknown"),
                                 juce::translate("Imported from Midi with Arkos Tracker 2"));
    auto& subsongBuilder = songBuilder->startNewSubsong(0);
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName);

    // Creates as many PSGs as needed.
    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        subsongBuilder.addPsg(Psg());
    }

    // Browses each "midi cell" (which is a "midi track" converted to Cells).
    // If we have gone beyond the current Pattern height, we create a new Pattern.
    auto position = 0;
    auto patternBaseTrackIndex = 0;          // One track per Track for each Channel. This will be optimized later.
    auto currentLineInPattern = 0;
    const auto subsongLength = static_cast<int>(patternHeights.size());
    auto patternHeight = patternHeights.at(static_cast<size_t>(position));
    std::vector volumeForEachChannel(static_cast<size_t>(inputChannelCount), -1);      // All the volumes are unreachable at first, to force their encoding.

    for (auto midiCell = 0, lastMidiCell = midiTrackReader.getLastCellIndex(); midiCell <= lastMidiCell; ++midiCell) {

        // Browses each midi track for this line.
        for (auto channelIndex = 0; channelIndex < inputChannelCount; ++channelIndex) {
            const auto& cellIndexToCells = cellIndexToCellsForEachMidiTrack.at(static_cast<size_t>(channelIndex));
            // Is there a cell in this Track, at the line?
            auto iterator = cellIndexToCells.find(midiCell);
            if (iterator == cellIndexToCells.cend()) {
                continue;           // No Cell. Nothing to do.
            }
            // Writes the Cell in a destination Track.
            auto cell = iterator->second;
            // If the volume is the same, removes it.
            if (importVelocities) {
                auto newVolumeForChannel = removeVolumeEffectIfTheSame(volumeForEachChannel.at(static_cast<size_t>(channelIndex)), cell);
                volumeForEachChannel.at(static_cast<size_t>(channelIndex)) = newVolumeForChannel;
            }

            // Writes the cell.
            if (channelIndex < outputChannelCount) {        // Dismisses the track if out of output bounds.
                const auto trackIndex = patternBaseTrackIndex + channelIndex;
                subsongBuilder.setCellOnTrack(trackIndex, currentLineInPattern, cell);
            }
        }

        // Should a Speed Cell be written?
        const auto speedTrackIndex = position;
        currentSpeed = manageSpeedCell(subsongBuilder, midiTrackReader, midiCell, speedTrackIndex, currentLineInPattern, currentSpeed, originalPpq, ppq);

        // Ok for this line.
        ++currentLineInPattern;

        // Should we stop here for this Pattern?
        if (currentLineInPattern >= patternHeight) {
            encodePatternAndPosition(subsongBuilder, position, patternBaseTrackIndex, outputChannelCount, patternHeight);

            // Next Pattern.
            currentLineInPattern = 0;
            patternBaseTrackIndex += outputChannelCount;

            ++position;
            if (position < static_cast<int>(patternHeights.size())) {            // Only a security for a corner-case (on last iteration, so shouldn't matter).
                patternHeight = patternHeights.at(static_cast<size_t>(position));
            }
        }
    }

    // Any more data? Stores this last Pattern.
    if (currentLineInPattern > 0) {
        encodePatternAndPosition(subsongBuilder, position, patternBaseTrackIndex, outputChannelCount, patternHeight);
    }

    // Builds the Instruments.
    buildInstruments(midiTrackReader);

    // Assumes the whole song is played.
    subsongBuilder.setLoop(0, subsongLength - 1);

    songBuilder->finishCurrentSubsong();
}

void MidiImporter::encodePatternAndPosition(SubsongBuilder& subsongBuilder, const int position, const int patternBaseTrackIndex, const int channelCount, const int patternHeight) noexcept
{
    const auto patternIndex = position;
    const auto speedTrackIndex = position;

    // Creates the Pattern.
    subsongBuilder.startNewPattern(patternIndex);
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        const auto trackIndex = patternBaseTrackIndex + channelIndex;
        subsongBuilder.setCurrentPatternTrackIndex(channelIndex, trackIndex);
    }
    subsongBuilder.setCurrentPatternSpeedTrackIndex(speedTrackIndex);
    subsongBuilder.finishCurrentPattern();

    // Creates the Position.
    subsongBuilder.startNewPosition();
    subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
    subsongBuilder.setCurrentPositionHeight(patternHeight);
    subsongBuilder.finishCurrentPosition();
}

int MidiImporter::removeVolumeEffectIfTheSame(const int currentVolumeForChannel, Cell& cell) noexcept
{
    // No volume effect? Then don't do anything.
    auto cellEffectOptional = cell.find(Effect::volume);
    if (cellEffectOptional.isAbsent()) {
        // No volume effect. Don't do anything!
        return currentVolumeForChannel;
    }

    // There is a volume. Is it different?
    const auto newVolume = cellEffectOptional.getValue().getEffectLogicalValue();
    if (newVolume != currentVolumeForChannel) {
        // Different! Don't modify it, but returns it to use it later.
        return newVolume;
    }

    // Same volume! Removes it from the Cell.
    cell.removeEffect(Effect::volume);

    return currentVolumeForChannel;
}

int MidiImporter::manageSpeedCell(SubsongBuilder& subsongBuilder, const MidiTrackReader& midiTrackReader, const int midiCell, const int speedTrackIndex,
                                  const int cellIndexInPosition, const int currentSpeed, const int originalSongPpq, const int ppq) noexcept
{
    // Is there a speed change?
    const auto& cellIndexToBpms = midiTrackReader.getCellIndexToBpm();
    const auto iterator = cellIndexToBpms.find(midiCell);
    if (iterator == cellIndexToBpms.cend()) {
        // No speed change. We can stop.
        return currentSpeed;
    }

    // There is a speed change.
    const auto bpm = iterator->second;
    // Converts it to Speed.
    const auto linesPerBeat = static_cast<float>(originalSongPpq) / static_cast<float>(ppq);
    const auto speed = SpeedUtil::convertBpmToSpeed(bpm, linesPerBeat, PsgFrequency::defaultReplayFrequencyHz);
    jassert((speed > 0) && (speed < 256));
    if ((speed == currentSpeed) || (speed == 0) || (speed > 255)) {
        // No need to do anything if the speed didn't change.
        return currentSpeed;
    }

    // Writes the new speed.
    subsongBuilder.setSpeedToTrack(speedTrackIndex, cellIndexInPosition, speed);

    return speed;
}

int MidiImporter::findOriginalSongPpq(const juce::MidiFile& midiFile) noexcept
{
    auto timeFormat = midiFile.getTimeFormat();
    // Should be positive, else SMTP, and should have been discarded before.

    jassert(timeFormat > 0);
    if (timeFormat <= 0) {
        timeFormat = 0;
    }

    return timeFormat;
}

std::pair<int, std::set<int>> MidiImporter::findTrackCount(const juce::MidiFile& midiFile) noexcept
{
    auto trackCount = midiFile.getNumTracks();
    auto nonEmptyTrackIndexes = std::set<int>();

    // Browses each Midi Track, stores the indexes of the ones that have notes.
    for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        if (hasMidiTrackNotes(midiFile, trackIndex)) {
            nonEmptyTrackIndexes.emplace(trackIndex);
        }
    }

    return { trackCount, nonEmptyTrackIndexes };
}

void MidiImporter::buildInstruments(const MidiTrackReader& midiTrackReader) const noexcept
{
    auto instrumentIndexToInstrumentType = midiTrackReader.getInstrumentIndexToMidiInstrumentType();
    for (const auto&[instrumentIndex, midiInstrumentData] : instrumentIndexToInstrumentType) {
        const auto midiInstrumentType = midiInstrumentData.midiInstrumentType;
        const auto instrumentName = midiInstrumentData.name;

        buildInstrument(instrumentIndex, midiInstrumentType, instrumentName);
    }
}

void MidiImporter::buildInstrument(const int instrumentIndex, const MidiTrackReader::MidiInstrumentType instrumentType, const juce::String& name) const noexcept
{
    // Generates the right Instrument.
    std::unique_ptr<Instrument> instrument;
    switch (instrumentType) {
        case MidiTrackReader::MidiInstrumentType::longSound:
            instrument = GenericInstrumentGenerator::generateInstrumentForAttackSustainDecay();
            break;
        case MidiTrackReader::MidiInstrumentType::drumBassdrum:
            instrument = GenericInstrumentGenerator::generateInstrumentForBassDrum();
            break;
        case MidiTrackReader::MidiInstrumentType::drumSnare:
            instrument = GenericInstrumentGenerator::generateInstrumentForSnare();
            break;
        case MidiTrackReader::MidiInstrumentType::drumTomHigh:
            instrument = GenericInstrumentGenerator::generateInstrumentForTom(0);
            break;
        case MidiTrackReader::MidiInstrumentType::drumTomMedium:
            instrument = GenericInstrumentGenerator::generateInstrumentForTom(-200);
            break;
        case MidiTrackReader::MidiInstrumentType::drumTomLow:
            instrument = GenericInstrumentGenerator::generateInstrumentForTom(-500);
            break;
        case MidiTrackReader::MidiInstrumentType::drumHihat:
        case MidiTrackReader::MidiInstrumentType::drumGeneric:
            instrument = GenericInstrumentGenerator::generateInstrumentForHihat();
            break;
        case MidiTrackReader::MidiInstrumentType::drumCrash:
            instrument = GenericInstrumentGenerator::generateInstrumentForCrash();
            break;
        case MidiTrackReader::MidiInstrumentType::shortSound:
        default:
            instrument = GenericInstrumentGenerator::generateInstrumentForDecreasingVolume();
            break;
    }
    jassert(instrument != nullptr);

    instrument->setName(name);

    // Stores it in the Song.
    songBuilder->setInstrument(instrumentIndex, std::move(instrument));
}

ImportedFormat MidiImporter::getFormat() noexcept
{
    return ImportedFormat::midi;
}


}   // namespace arkostracker

