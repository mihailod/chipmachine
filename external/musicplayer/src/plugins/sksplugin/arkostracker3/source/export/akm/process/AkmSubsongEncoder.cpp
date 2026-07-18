#include "AkmSubsongEncoder.h"

#include "../../../business/song/tool/BaseNoteFinder.h"
#include "../../../business/song/tool/InstrumentSimpleCounter.h"
#include "../../../business/song/tool/WaitCounter.h"
#include "../../../business/song/tool/browser/CellBrowser.h"
#include "../../../business/song/tool/browser/TrackBrowser.h"
#include "../../../song/Song.h"
#include "../../../utils/CollectionUtil.h"
#include "../../ExportConfiguration.h"
#include "../../SongExportResult.h"
#include "../../sourceGenerator/SourceGenerator.h"
#include "AkmSequenceEncoder.h"
#include "LinkerEncoder.h"

namespace arkostracker 
{

AkmSubsongEncoder::AkmSubsongEncoder(const Song& pSong, Id pSubsongId, const ExportConfiguration& pExportConfiguration,
                                     SongExportResult& pSongExportResult, juce::String pBaseLabelForSubsong) noexcept:
        song(pSong),
        subsongId(std::move(pSubsongId)),
        exportConfiguration(pExportConfiguration),
        songExportResult(pSongExportResult),
        baseLabelForSubsong(std::move(pBaseLabelForSubsong)),
        areEffectsPresentInSong(),
        primaryDefaultInstrument(0),
        secondaryDefaultInstrument(0),
        primaryWait(0),
        secondaryWait(0),
        mostUsedNoteIndexes(),
        startNoteInTrack(0),
        startInstrumentInTrack(0),
        startWaitInTrack(0),
        foundDefaultNoteInstrumentAndWait()
{
}

void AkmSubsongEncoder::generateSubsong() noexcept
{
    parseSubsongAndBuildStructures();

    auto subsongOutputStream = std::make_unique<juce::MemoryOutputStream>();
    SourceGenerator sourceGenerator(exportConfiguration.getSourceConfiguration(), *subsongOutputStream);
    sourceGenerator.setPrefixForDisark(baseLabelForSubsong + "_");

    // Encodes the header.
    encodeHeader(sourceGenerator);

    // Encodes the Linker.
    encodeLinker(sourceGenerator);

    // Encodes the Tracks.
    encodeTracks(sourceGenerator);

    // Encodes the table with the note indexes.
    encodeNoteIndexTable(sourceGenerator);

    // Stores the output stream.
    songExportResult.addSubsongStream(std::move(subsongOutputStream));
}

void AkmSubsongEncoder::parseSubsongAndBuildStructures() noexcept
{
    // Are there effects in the song? Important to know, because we will use a data value for more note reference, if there are no effects.
    CellBrowser::browseConstSubsongCells(song, subsongId, true, [&](const Cell& cell) noexcept {
        areEffectsPresentInSong = cell.hasEffects();
        return areEffectsPresentInSong;     // Stops the browsing as soon as an effect is found.
    });

    const auto noteReferenceCount = areEffectsPresentInSong ? AkmSequenceEncoder::valueNoteAndEffects : AkmSequenceEncoder::valueNoteAndEffects + 1;

    // Finds the most used notes.
    const BaseNoteFinder baseNoteFinder(song, subsongId, noteReferenceCount);
    auto sortedNoteIndexToIteration = baseNoteFinder.getSortedNoteIndexToIteration();
    // Only keeps a minimum of them, and those that are worth keeping.
    CollectionUtil::removeIfAndResize(sortedNoteIndexToIteration, [](const auto& pair) {
        return pair.second < 3U;
    });
    CollectionUtil::shrinkIfBigger(sortedNoteIndexToIteration, static_cast<size_t>(noteReferenceCount));
    // Only keeps the notes themselves, we don't need the iteration count anymore.
    mostUsedNoteIndexes = CollectionUtil::mapToVector<decltype(sortedNoteIndexToIteration), int>(sortedNoteIndexToIteration);

    // What are the most used instruments?
    const auto instrumentOccurrenceMap = InstrumentSimpleCounter::countInstrument(song, subsongId);

    const auto& sortedMostUsedInstrumentIndexesAndReferences = instrumentOccurrenceMap.generateSortedMostUsedItemsAndOccurrence();

    // Gets the most used Instrument, if any (there should be... But an empty Song would be possible).
    if (!sortedMostUsedInstrumentIndexesAndReferences.empty()) {
        primaryDefaultInstrument = sortedMostUsedInstrumentIndexesAndReferences.at(0U).first;

        // Gets the second most used Instrument, if any.
        if (sortedMostUsedInstrumentIndexesAndReferences.size() > 1U) {
            secondaryDefaultInstrument = sortedMostUsedInstrumentIndexesAndReferences.at(1U).first;
        }
    }
    foundDefaultNoteInstrumentAndWait = true;

    // For more optimization, finds default note/instrument/wait at the beginning of each Track, keep the best.
    findDefaultTrackStartItems();

    // What are the most used Wait?
    const auto waitOccurrenceMap = WaitCounter::countWait(song, subsongId);
    const auto sortedMostUseWaits = waitOccurrenceMap.generateSortedMostUsedItemsAndOccurrence();
    // Gets the most used Wait, if any (there should be... But an empty Song is possible).
    if (!sortedMostUseWaits.empty()) {
        primaryWait = sortedMostUseWaits.at(0U).first;

        // Gets the second most used Wait, if any.
        if (sortedMostUseWaits.size() > 1U) {
            secondaryWait = sortedMostUseWaits.at(1U).first;
        }
    }
}

void AkmSubsongEncoder::findDefaultTrackStartItems() noexcept
{
    // The primary/secondary instrument/notes/wait must have been found already.
    jassert(foundDefaultNoteInstrumentAndWait);

    OccurrenceMap<int> noteMap;
    OccurrenceMap<int> instrumentMap;
    OccurrenceMap<int> waitMap;

    // Browses each Track.
    for (const auto trackIndex : TrackBrowser::getAllUsedTrackIndexes(song, subsongId, true)) {
        Track track;
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            track = subsong.getTrackRefFromIndex(trackIndex);
        });

        auto noteAndInstrumentFound = false;
        auto waitFound = false;
        auto accumulatedWait = 0;
        // Browses each cell. Stops as soon as the start note/instrument and wait are found.
        for (auto cellIndex = 0, cellCount = track.getSize(); !noteAndInstrumentFound && !waitFound && (cellIndex < cellCount); ++cellIndex) {
            const auto& cell = track.getCell(cellIndex);
            // Find the start note and instrument, if not already found.
            if (!noteAndInstrumentFound && cell.isNoteAndInstrument()) {
                // A note with instrument is found.
                // The start note must be different from the most used notes, else it has no purpose.
                const auto note = cell.getNote().getValue().getNote();
                if (CollectionUtil::findItemAndGetIndex(mostUsedNoteIndexes, note) < 0) {
                    noteMap.addItem(note);
                }

                // The start instrument must be different from the primary/secondary instrument, else it has no purpose.
                const auto instrumentIndex = cell.getInstrument().getValue();
                if ((instrumentIndex != primaryDefaultInstrument) && (instrumentIndex != secondaryDefaultInstrument)) {
                    instrumentMap.addItem(instrumentIndex);
                }

                // This is enough for us, we can stop here!
                noteAndInstrumentFound = true;
            }

            // The same for waits, if not already found.
            if (!waitFound) {
                if (cell.isEmpty()) {
                    ++accumulatedWait;
                } else {
                    // Found something. We can stop.
                    // The wait must be different from the primary/secondary wait, else it has no purpose.
                    if ((accumulatedWait != primaryWait) && (accumulatedWait != secondaryWait)) {
                        waitMap.addItem(accumulatedWait);
                    }
                    waitFound = true;
                }
            }
        }
    }

    // Takes the best, if any.
    if (noteMap.getSize() > 0U) {
        startNoteInTrack = noteMap.generateSortedMostUsedItemsAndOccurrence().front().first;
    }
    if (instrumentMap.getSize() > 0U) {
        startInstrumentInTrack = instrumentMap.generateSortedMostUsedItemsAndOccurrence().front().first;
    }
    if (waitMap.getSize() > 0U) {
        startWaitInTrack = waitMap.generateSortedMostUsedItemsAndOccurrence().front().first;
    }
}

void AkmSubsongEncoder::encodeHeader(SourceGenerator& sourceGenerator) const noexcept
{
    const auto metadata = song.getSubsongMetadata(subsongId);
    const auto psgCount = song.getPsgCount(subsongId);
    const auto subsongIndex = song.getSubsongIndex(subsongId).getValue();

    if (psgCount != 1) {
        songExportResult.addWarning("Only one PSG is supported per subsong! The others will be discarded.");
    }

    sourceGenerator.declareComment(song.getTitle() + ", Subsong " + juce::String(subsongIndex) + ".");
    sourceGenerator.declareComment("----------------------------------").addEmptyLine();

    sourceGenerator.declareLabel(baseLabelForSubsong);
    sourceGenerator.declarePointerRegionStart();
    sourceGenerator.declareWord(getNoteIndexTableLabel(), "Index table for the notes.");
    sourceGenerator.declareWord(getTrackIndexTable(), "Index table for the Tracks.");
    sourceGenerator.declarePointerRegionEnd();
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareByteRegionStart();
    sourceGenerator.declareByte(metadata.getInitialSpeed(), "Initial speed.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareByte(primaryDefaultInstrument, "Most used instrument.");
    sourceGenerator.declareByte(secondaryDefaultInstrument, "Second most used instrument.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareByte(primaryWait, "Most used wait.");
    sourceGenerator.declareByte(secondaryWait, "Second most used wait.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareByte(startNoteInTrack, "Default start note in tracks.");
    sourceGenerator.declareByte(startInstrumentInTrack, "Default start instrument in tracks.");
    sourceGenerator.declareByte(startWaitInTrack, "Default start wait in tracks.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareByte((areEffectsPresentInSong ? AkmSequenceEncoder::valueNoteAndEffects : AkmSequenceEncoder::valueNoteAndEffects + 1),
            "Are there effects? 12 if yes, 13 if not. Don't ask.");
    sourceGenerator.declareByteRegionEnd();
    sourceGenerator.addEmptyLine();
}

juce::String AkmSubsongEncoder::getTrackIndexTable() const noexcept
{
    return baseLabelForSubsong + "_TrackIndexes";
}

juce::String AkmSubsongEncoder::getNoteIndexTableLabel() const noexcept
{
    return baseLabelForSubsong + "_NoteIndexes";
}

juce::String AkmSubsongEncoder::getTrackLabel(const int trackIndex) const noexcept
{
    return baseLabelForSubsong + "_Track" + juce::String(trackIndex);
}

void AkmSubsongEncoder::encodeLinker(SourceGenerator& sourceGenerator) const noexcept
{
    LinkerEncoder linkerEncoder(sourceGenerator, songExportResult, song, subsongId, baseLabelForSubsong,
                                getTrackIndexTable(),
                                [&](const int trackIndex) { return getTrackLabel(trackIndex); }
                                );
    linkerEncoder.encodeLinker();

    sourceGenerator.addEmptyLine();
}

void AkmSubsongEncoder::encodeTracks(SourceGenerator& sourceGenerator) const noexcept
{
    sourceGenerator.declareByteRegionStart();

    std::vector<Track> tracks;
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        tracks = subsong.getTracks();
    });

    // Encodes each Tracks.
    auto trackIndex = 0;
    for (const auto& track : tracks) {
        encodeTrack(sourceGenerator, trackIndex, track);
        ++trackIndex;
    }
    sourceGenerator.declareByteRegionEnd();
}

void AkmSubsongEncoder::encodeTrack(SourceGenerator& sourceGenerator, const int trackIndex, const Track& track) const noexcept
{
    sourceGenerator.declareLabel(getTrackLabel(trackIndex));

    // Encodes the track.
    const auto encodedCellsAndComments = AkmSequenceEncoder::encodeTrack(songExportResult, track,
                                                                         areEffectsPresentInSong,
                                                                         primaryDefaultInstrument,
                                                                         secondaryDefaultInstrument, primaryWait, secondaryWait,
                                                                         startNoteInTrack, startInstrumentInTrack, startWaitInTrack,
                                                                         mostUsedNoteIndexes);
    for (const auto& byteAndComment : encodedCellsAndComments) {
        sourceGenerator.declareByte(static_cast<int>(byteAndComment.first), byteAndComment.second);
    }


    sourceGenerator.addEmptyLine();
}

void AkmSubsongEncoder::encodeNoteIndexTable(SourceGenerator& sourceGenerator) const noexcept
{
    // Generates the note indexes.
    sourceGenerator.declareComment("The note indexes.");
    sourceGenerator.declareLabel(getNoteIndexTableLabel());

    sourceGenerator.declareByteRegionStart();

    auto index = 0;
    for (const auto& note : mostUsedNoteIndexes) {
        sourceGenerator.declareByte(note, "Note for index " + juce::String(index) + ".");
        ++index;
    }
    sourceGenerator.declareByteRegionEnd();

    sourceGenerator.addEmptyLine();
}

}   // namespace arkostracker
