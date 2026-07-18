#include "AkgSubsongExporter.h"

#include "../../../business/song/tool/BaseNoteFinder.h"
#include "../../../song/Song.h"
#include "../../../utils/PsgValues.h"
#include "../../ExportConfiguration.h"
#include "../../SongExportResult.h"
#include "../../sourceGenerator/SourceGenerator.h"
#include "EncodedEffectBlock.h"
#include "LinkerBlockContainer.h"

namespace arkostracker 
{

const juce::String AkgSubsongExporter::linkerBlockLabelString = "LinkerBlock";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AkgSubsongExporter::trackLabelString = "Track";           			// NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AkgSubsongExporter::speedTrackLabelString = "SpeedTrack";           	// NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AkgSubsongExporter::eventTrackLabelString = "EventTrack";           	// NOLINT(cert-err58-cpp, *-statically-constructed-objects)

AkgSubsongExporter::AkgSubsongExporter(const Song& pSong, Id pSubsongId, const ExportConfiguration& pExportConfiguration,
                                       PlayerConfiguration& pPlayerConfiguration,
                                       SongExportResult& pSongExportResult, juce::String pBaseLabelForSubsong,
                                       const std::map<juce::String, int>& pEffectBlockIdToIndex,
                                       const std::map<juce::String, int>& pEffectBlockIdToOffsetFromEffectBlockTable) noexcept :
        song(pSong),
        subsongId(std::move(pSubsongId)),
        exportConfiguration(pExportConfiguration),
        playerConfiguration(pPlayerConfiguration),
        exportResult(pSongExportResult),
        baseLabel(std::move(pBaseLabelForSubsong)),
        effectBlockIdToIndex(pEffectBlockIdToIndex),
        effectBlockIdToOffsetFromEffectBlockTable(pEffectBlockIdToOffsetFromEffectBlockTable),
        usedTrackIndexes(),
        usedSpeedTrackIndexes(),
        usedEventTrackIndexes()
{
}

void AkgSubsongExporter::generateSubsong() noexcept
{
    auto subsongOutputStream = std::make_unique<juce::MemoryOutputStream>();
    SourceGenerator sourceGenerator(exportConfiguration.getSourceConfiguration(), *subsongOutputStream);
    
    sourceGenerator.setPrefixForDisark(baseLabel);
    const auto subsongIndex = song.getSubsongIndex(subsongId).getValue();

    sourceGenerator.declareComment("Subsong " + juce::String(subsongIndex));
    sourceGenerator.declareComment("----------------------");

    sourceGenerator.declareByteRegionStart();

    // Encodes the metadata of the Subsong.
    const auto startLabel = baseLabel + "Start";
    sourceGenerator.declareLabel(startLabel);

    sourceGenerator.declareByte(convertHzToPlayerFrequencyIndex(song.getReplayFrequencyHz(subsongId)),
                                "ReplayFrequency (0=12.5hz, 1=25hz, 2=50hz, 3=100hz, 4=150hz, 5=300hz).");

    // Finds the base note.
    const BaseNoteFinder baseNoteFinder(song, subsongId, optimizedNoteRange);
    const auto baseNoteIndex = baseNoteFinder.getBaseNote();

    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto metadata = subsong.getMetadata();
        const auto psgCount = subsong.getPsgCount();
        sourceGenerator.declareByte(metadata.getDigiChannel(), "Digichannel (>=0).");
        sourceGenerator.declareByte(psgCount, "PSG count (>0).");
        sourceGenerator.declareByte(metadata.getLoopStartPosition(), "Loop start index (>=0).");
        sourceGenerator.declareByte(metadata.getEndPosition(), "End index (>=0).");
        sourceGenerator.declareByte(metadata.getInitialSpeed(), "Initial speed (>=0).");
        sourceGenerator.declareByte(baseNoteIndex, "Base note index (>=0).");
        sourceGenerator.addEmptyLine();
    });

    // Encodes the Linker.
    encodeSubsongLinker(sourceGenerator);

    // Encodes the Tracks.
    encodeTracks(sourceGenerator, baseNoteIndex);

    // Encodes the SpeedTracks and EventTracks.
    encodeSpecialTrack(sourceGenerator, true);
    encodeSpecialTrack(sourceGenerator, false);

    sourceGenerator.declareByteRegionEnd();
    sourceGenerator.declareEndOfFile();

    exportResult.addSubsongStream(std::move(subsongOutputStream));
}

int AkgSubsongExporter::convertHzToPlayerFrequencyIndex(const float frequency) noexcept
{
    int result;    // NOLINT(*-init-variables)
    if (juce::exactlyEqual(frequency, 12.5F)) {
        result = 0;
    } else if (juce::exactlyEqual(frequency, 25.0F)) {
        result = 1;
    } else if (juce::exactlyEqual(frequency, 50.0F)) {
        result = 2;
    } else if (juce::exactlyEqual(frequency, 100.0F)) {
        result = 3;
    } else if (juce::exactlyEqual(frequency, 150.0F)) {
        result = 4;
    } else if (juce::exactlyEqual(frequency, 300.0F)) {
        result = 5;
    } else {
        result = 2;
        jassertfalse;         // Unknown frequency. We don't care much...
    }
    return result;
}

void AkgSubsongExporter::encodeSubsongLinker(SourceGenerator& sourceGenerator) noexcept
{
    usedTrackIndexes.clear();

    // Declares the label of the Linker.
    const auto linkerLabel = baseLabel + "Linker";
    const auto linkerLoopLabel = linkerLabel + "_Loop";
    sourceGenerator.declareLabel(linkerLabel);

    // Builds the unique LinkerBlocks.
    const LinkerBlockContainer linkerBlockContainer(song, subsongId);

    // Encodes each Position.
    sourceGenerator.declarePointerRegionStart();        // One region for all this.
    auto positionLoopStartIndex = 0;
    auto positionEndIndex = 0;
    auto psgCount = 0;
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
        positionLoopStartIndex = subsong.getLoopStartPosition();
        positionEndIndex = subsong.getEndPosition();
        psgCount = subsong.getPsgCount();
    });

    for (auto positionIndex = 0; positionIndex <= positionEndIndex; ++positionIndex) {
        sourceGenerator.declareComment("Position " + juce::String(positionIndex));

        // Shows the loop Label, if it loops here.
        if (positionIndex == positionLoopStartIndex) {
            sourceGenerator.declareLabel(linkerLoopLabel);
        }

        Pattern pattern;
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
            pattern = subsong.getPatternRef(positionIndex);
        });

        // Encodes the track indexes.
        for (auto trackIndex : pattern.getCurrentTrackIndexes()) {
            sourceGenerator.declareWord(getTrackLabel(trackIndex));

            // Stores it for later.
            usedTrackIndexes.emplace(trackIndex);

            // Also stores the Speed/Event tracks indexes for a later encoding.
            usedSpeedTrackIndexes.insert(pattern.getCurrentSpecialTrackIndex(true));
            usedEventTrackIndexes.insert(pattern.getCurrentSpecialTrackIndex(false));
        }

        // Encodes the Linker Block address.
        const auto linkerBlockIndex = linkerBlockContainer.getLinkerBlockIndexFromPosition(positionIndex);

        const auto labelLinkerBlock = getLinkerBlockLabel(static_cast<int>(linkerBlockIndex));
        sourceGenerator.declareWord(labelLinkerBlock);
    }
    sourceGenerator.declarePointerRegionEnd();

    // Encodes the loop in the Linker.
    sourceGenerator.declareWord(0, "Loop.");
    sourceGenerator.declareWordForceReference(linkerLoopLabel).addEmptyLine();

    // Encodes the LinkerBlocks.
    for (auto linkerBlockIndex = 0, linkerBlockCount = linkerBlockContainer.getLinkerBlockCount(); linkerBlockIndex < linkerBlockCount; ++linkerBlockIndex) {
        // Encodes the label of the LinkerBlock.
        sourceGenerator.declareLabel(getLinkerBlockLabel(linkerBlockIndex));
        // Encodes its data.
        const auto& linkerBlock = linkerBlockContainer.getLinkerBlock(linkerBlockIndex);
        sourceGenerator.declareByte(linkerBlock.getHeight(), "Height.");
        // Encodes the transpositions.
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            const auto basePatternIndex = PsgValues::channelCountPerPsg * psgIndex;
            for (auto patternIndex = basePatternIndex, afterLastPatternIndex = (basePatternIndex + PsgValues::channelCountPerPsg);
            patternIndex < afterLastPatternIndex; ++patternIndex) {
                const auto transposition = linkerBlock.getTransposition(patternIndex);
                sourceGenerator.declareByte(transposition, "Transposition " + juce::String(patternIndex) + ".");

                playerConfiguration.addFlag((transposition != 0), PlayerConfigurationFlag::transpositions);
            }
        }

        const auto speedTrackIndex = linkerBlock.getSpeedTrackIndex();
        sourceGenerator.declareWordForceReference(getSpeedTrackLabel(speedTrackIndex), "SpeedTrack address.");
        const auto eventTrackIndex = linkerBlock.getEventTrackIndex();
        sourceGenerator.declareWordForceReference(getEventTrackLabel(eventTrackIndex), "EventTrack address.");

        sourceGenerator.addEmptyLine();
    }

    sourceGenerator.addEmptyLine();
}

juce::String AkgSubsongExporter::getTrackLabel(const int trackIndex) const noexcept
{
    jassert(trackIndex >= 0);

    return baseLabel + trackLabelString + juce::String(trackIndex);
}

juce::String AkgSubsongExporter::getLinkerBlockLabel(const int linkerBlockIndex) const noexcept
{
    jassert(linkerBlockIndex >= 0);

    return baseLabel + linkerBlockLabelString + juce::String(linkerBlockIndex);
}

juce::String AkgSubsongExporter::getSpeedTrackLabel(const int speedTrackIndex) const noexcept
{
    jassert(speedTrackIndex >= 0);

    return baseLabel + speedTrackLabelString + juce::String(speedTrackIndex);
}

juce::String AkgSubsongExporter::getEventTrackLabel(const int eventTrackIndex) const noexcept
{
    jassert(eventTrackIndex >= 0);

    return baseLabel + eventTrackLabelString + juce::String(eventTrackIndex);
}

void AkgSubsongExporter::encodeTracks(SourceGenerator& sourceGenerator, const int baseNoteIndex) const noexcept
{
    // Browses through each Track.
    for (const auto trackIndex : usedTrackIndexes) {
        // Encodes each Track.
        encodeTrack(trackIndex, sourceGenerator, baseNoteIndex);
    }
}

void AkgSubsongExporter::encodeTrack(int trackIndex, SourceGenerator& sourceGenerator, int baseNoteIndex) const noexcept
{
    sourceGenerator.declareLabel(getTrackLabel(trackIndex));

    auto currentInstrument = -1; // Unreachable, to force the encoding.
    auto emptyCellCount = 0;     // How many totally empty (no notes, no effects) are accounted for.

    Track track;
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        track = subsong.getTrackRefFromIndex(trackIndex);
    });

    jassert(baseNoteIndex >= 0);
    const auto minimumOptimizedNote = baseNoteIndex;
    const auto maximumOptimizedNote = minimumOptimizedNote + optimizedNoteRange - 1;
    const auto rstNote = minimumOptimizedNote;

    // Browses through all the Cells. It is supposed to be cleansed of all the useless data.
    for (auto cellIndex = 0, cellLastIndex = TrackConstants::lastPossibleIndex; cellIndex <= cellLastIndex; ++cellIndex) {
        const auto cell(track.getCell(cellIndex));

        // Empty?
        if (cell.isEmpty()) {
            ++emptyCellCount;
            continue;
        }

        // Not empty. First, encodes the possibly stacked empty Cells.
        encodeEmptyCells(emptyCellCount, sourceGenerator);
        emptyCellCount = 0;

        const auto areEffectsPresent = cell.hasEffects();
        auto isNotePresent = cell.isNote();

        // Special case: if a Glide effect is present, the note is considered NOT present. BUT it will be used when encoding the effect.
        const auto glideEffect = cell.find(Effect::pitchGlide);
        if (glideEffect.isPresent()) {
            isNotePresent = false;
        }

        // Is there a note?
        if (isNotePresent) {
            // There should be an Instrument, since Legato have already been converted to effect!
            jassert(cell.isInstrument());

            auto note = cell.getNote().getValueRef().getNote();

            if (cell.isInstrument()) {
                // Note and instrument.
                const auto readInstrument = cell.getInstrument().getValue();
                if (cell.isRst()) {
                    // If RST, encodes a non-escaped note.
                    note = rstNote;
                }

                auto mainOpcode = 0U;
                auto mustEncodeEscapeNote = false;

                // Encodes the note. Is it within optimized range?
                if ((note >= minimumOptimizedNote) && (note <= maximumOptimizedNote)) {
                    mainOpcode |= static_cast<unsigned int>(note - minimumOptimizedNote);
                } else {
                    // No: encodes an escape note.
                    mainOpcode |= static_cast<unsigned int>(opcodeEscapeNote);
                    mustEncodeEscapeNote = true;
                }

                // Is the instrument the same as before?
                const auto sameInstrument = (currentInstrument == readInstrument);
                if (!sameInstrument) {
                    currentInstrument = readInstrument;
                    mainOpcode |= 128U;      // New Instrument, bit 7 to 1.
                }

                // Effects? Adds the effect bit.
                if (areEffectsPresent) {
                    mainOpcode |= 64U;       // Effects, bit 6 to 1.
                }

                // Encodes the opcode.
                sourceGenerator.declareByte(static_cast<int>(mainOpcode));
                // Encodes the escape note?
                if (mustEncodeEscapeNote) {
                    sourceGenerator.declareByte(note, "Escape note (" + juce::String(note) + ").");
                }

                // Encodes the instrument, if needed.
                if (!sameInstrument) {
                    sourceGenerator.declareByte(currentInstrument, "New Instrument (" + juce::String(currentInstrument) + ").");
                }
            }
        }

        // Manages the case of a no-note with effect (the nothing at all case has been handled at the very beginning). This also takes care of the legato/glide case.
        if (!isNotePresent) {
            jassert(areEffectsPresent);
            auto byteToEncode = static_cast<unsigned int>(opcodeNoNoteMaybeEffects);
            if (areEffectsPresent) {
                byteToEncode |= 64U;
            }
            sourceGenerator.declareByte(static_cast<int>(byteToEncode), "No note, but effects.");
        }


        // An effect is perhaps present.
        // The reference to its block is encoded, but the effect block itself will be encoded later.
        if (cell.hasEffects()) {
            const auto& cellEffects = cell.getEffects();
            encodeEffectBlockReferenceAndStoreEffects(cell.getNote(), cellEffects, sourceGenerator);
        }
    }

    // End of the Track. Some empty cells to encode? In that case, encodes the maximum possible wait (simpler for human read).
    if (emptyCellCount > 0) {
        encodeEmptyCells(TrackConstants::maximumCapacity, sourceGenerator);
    }

    sourceGenerator.addEmptyLine();
}

void AkgSubsongExporter::encodeEmptyCells(const int cellCount, SourceGenerator& sourceGenerator) noexcept
{
    // Nothing to encode?
    if (cellCount == 0) {
        return;
    }

    if (cellCount == 1) {
        // Optimized for 1 line.
        sourceGenerator.declareByte(opcodeNoNoteMaybeEffects, "Waits for 1 line.");
    } else if (cellCount <= 5) {
        // Optimized, from 2 to 5, encoded in bits 6/7.
        sourceGenerator.declareByte(juce::String(opcodeWaitDuring2To5Lines) + " + " + juce::String(cellCount - 2) + " * 64",
                                    "Optimized wait for " + juce::String(cellCount) + " lines.");
    } else {
        sourceGenerator.declareByte(opcodeWaitDuringXLines).declareByte(cellCount - 1, "Waits for " + juce::String(cellCount) + " lines.");
    }

    sourceGenerator.addEmptyLine();
}

void AkgSubsongExporter::encodeEffectBlockReferenceAndStoreEffects(const OptionalValue<Note> note, const CellEffects& cellEffects, SourceGenerator& sourceGenerator) const noexcept
{
    const EncodedEffectBlock encodedEffectBlock(playerConfiguration, note, cellEffects);

    // Encodes the reference. Index or relative address?
    const auto effectBlockId = encodedEffectBlock.getId();
    const auto iterator = effectBlockIdToIndex.find(effectBlockId);
    const auto encodedAsIndex = (iterator != effectBlockIdToIndex.cend());

    if (encodedAsIndex) {
        // Encodes the index. What is the index?
        const auto effectBlockIndex = iterator->second;
        jassert(effectBlockIndex < 0x80);           // Only on 7 bits.
        sourceGenerator.declareByte(juce::String(effectBlockIndex), "Index to an effect block.");
    } else {
        // Encodes the relative address (15 bits only). We know it from a previously generated map.
        jassert(effectBlockIdToOffsetFromEffectBlockTable.find(effectBlockId) != effectBlockIdToOffsetFromEffectBlockTable.cend());
        const auto offset = effectBlockIdToOffsetFromEffectBlockTable.find(effectBlockId)->second;
        jassert((offset >= 0) && (offset < 0x8000));           // Must be inferior to 64kb, because only encoded on 15 bits. But should never happen...

        sourceGenerator.declareByte("128 + " + juce::String((static_cast<unsigned int>(offset) / 256U) & 0x7fU), "7 bits of the MSB of the relative address to the effect block.");
        sourceGenerator.declareByte(juce::String(static_cast<unsigned int>(offset) & 0xffU), "8 bits of the LSB of the relative address to the effect block.");
    }
}

void AkgSubsongExporter::encodeSpecialTrack(SourceGenerator& sourceGenerator, const bool isSpeedTrack) const noexcept
{
    sourceGenerator.declareComment(isSpeedTrack ? "The speed tracks" : "The event tracks");

    for (auto specialTrackIndex : isSpeedTrack ? usedSpeedTrackIndexes : usedEventTrackIndexes) {
        const auto label = isSpeedTrack ? getSpeedTrackLabel(specialTrackIndex) : getEventTrackLabel(specialTrackIndex);
        sourceGenerator.declareLabel(label);

        SpecialTrack track;
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            track = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack);
        });

        // As soon as one track is not empty, consider it useful.
        playerConfiguration.addFlag(!track.isEmpty(), isSpeedTrack ? PlayerConfigurationFlag::speedTracks : PlayerConfigurationFlag::eventTracks);

        auto waitCount = 0;
        for (auto lineIndex = 0, lineCount = TrackConstants::maximumCapacity; lineIndex < lineCount; ++lineIndex) {
            if (const auto cell = track.getCell(lineIndex); cell.isEmpty()) {
                // No value, waits.
                ++waitCount;
            } else {
                // There is a value. Encodes the possible waits.
                // b0: 0 for data, 1 for wait.
                encodeWaitEventForSpecialTrack(sourceGenerator, waitCount);
                waitCount = 0;
                // Encodes the value. May need escape code!
                const auto value = static_cast<unsigned int>(cell.getValue());
                // The value can't be 0 (no speed, no event is 0).
                jassert(value > 0U);
                if (value < 128U) {
                    // No escape code.
                    sourceGenerator.declareByte(static_cast<int>(value << 1U), "Value: " + juce::String(value) + ".");
                } else {
                    // Escape code.
                    sourceGenerator.declareByte(0, "Escape code.");
                    sourceGenerator.declareByte(static_cast<int>(value), "Escaped value.");
                }
            }
        }

        // At the end, encodes the possible remaining waits.
        encodeWaitEventForSpecialTrack(sourceGenerator, waitCount);

        sourceGenerator.addEmptyLine();
    }
}

void AkgSubsongExporter::encodeWaitEventForSpecialTrack(SourceGenerator& sourceGenerator, const int waitCount) noexcept
{
    // Nothing to do?
    if (waitCount <= 0) {
        return;
    }
    jassert(waitCount <= TrackConstants::maximumCapacity);

    // b0: 0 for data, 1 for wait.
    sourceGenerator.declareByte(static_cast<int>((static_cast<unsigned int>(waitCount - 1) << 1U) + 1U), "Wait for " + juce::String(waitCount) + " lines.");
}

}   // namespace arkostracker
