#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/task/Task.h"
#include "../../business/song/tool/sfx/FirstNotePerInstrumentFinder.h"
#include "../../player/SongDataProvider.h"
#include "../../song/instrument/InstrumentType.h"
#include "../../song/instrument/psg/PsgPart.h"
#include "../ExportConfiguration.h"
#include "../SongExportResult.h"

namespace arkostracker
{

class ChannelOutputRegisters;
class Song;
class SourceGenerator;

/** Exports the sound effects. */
class SfxExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param song the Song. No need to optimize it.
     * @param exportConfiguration data about how to export.
     */
    SfxExporter(const Song& song, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    class SfxDataProvider final : public SongDataProvider
    {
    public:
        explicit SfxDataProvider(const Song& song, PsgPart psgPart) noexcept;

        OptionalId getInstrumentIdFromAudioThread(int instrumentIndex) const noexcept override;
        InstrumentType getInstrumentTypeFromAudioThread(const OptionalId& instrumentId) const noexcept override;
        OptionalId getExpressionIdFromAudioThread(bool isArpeggio, int expressionIndex) const noexcept override;
        ExpressionMetadata getExpressionMetadataFromAudioThread(bool isArpeggio, const OptionalId& expressionId) const noexcept override;
        PsgInstrumentFrameData getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, int cellIndex) const noexcept override;
        SampleInstrumentFrameData getSampleInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId) const noexcept override;
        int getExpressionValueFromAudioThread(bool isArpeggio, const OptionalId& expressionId, int cellIndex) const noexcept override;
        std::pair<int, float> getPsgFrequencyFromChannelFromAudioThread(int channelIndexInSong) const noexcept override;
        int getTranspositionFromAudioThread(int channelIndexInSong) const noexcept override;
        bool isEffectContextEnabled() const noexcept override;
        LineContext determineEffectContextFromAudioThread(CellLocationInPosition location) const noexcept override;
        LineContext determineEffectContextFromAudioThread(int channelIndexInSong) const noexcept override;
        SidPlayerCapability getSidPlayerCapability() const noexcept override;

    private:
        const Song& song;
        PsgPart psgPart;
    };

    /**
     * Encodes the sound effect which index is given. Nothing happens if the sfx is not to be encoded.
     * @param instrumentIndex the index of the sound effect in the instrument list (>0).
     * @param sourceGenerator the source generator.
     * @param playerConfiguration the player configuration to fill.
     */
    void encodeSoundEffect(int instrumentIndex, const FirstNotePerInstrumentFinder::ResultItem& sfxData, SourceGenerator& sourceGenerator, PlayerConfiguration& playerConfiguration) const noexcept;

    /**
     * Encodes the given Instrument Cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers to encode.
     * @param cellIndex the Cell index.
     * @param instrumentIndex the instrument index.
     * @param loopIndex the index where the sound loops, if it loops.
     * @param playerConfiguration the player configuration to fill.
     */
    void encodeRegisters(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, int cellIndex, int instrumentIndex, OptionalInt loopIndex,
                         PlayerConfiguration& playerConfiguration) const noexcept;

    /**
     * Encodes a "no software no hardware" cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     */
    static void encodeCellNoSoftwareNoHardware(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Encodes a "software only" cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     */
    static void encodeCellSoftwareOnly(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Encodes a "hardware only" cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     */
    static void encodeCellHardwareOnly(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Encodes a "software and hardware" cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     */
    static void encodeCellSoftwareAndHardware(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Shared code for "hardware only" or "software and hardware" cell.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     * @param hardwareOnly true if hardware only, false if soft and hard.
     */
    static void encodeCellSharedHardwareOrSoftwareAndHardware(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters, bool hardwareOnly,
                                                              PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Encodes the noise, if there is any.
     * @param sourceGenerator the source generator.
     * @param channelRegisters the registers.
     */
    static void encodeNoiseIfAny(SourceGenerator& sourceGenerator, const ChannelOutputRegisters& channelRegisters) noexcept;

    /**
     * @return the base label to be used before every label.
     * @param addEndSeparator true to add a "_" at the end.
     */
    juce::String getSoundEffectBaseLabel(bool addEndSeparator = false) const noexcept;

    /**
     * @return the label used for a sound effect.
     * @param soundEffectIndex the index. Should be >0.
     */
    juce::String getSoundEffectLabel(int soundEffectIndex) const noexcept;

    /**
     * @return the label used for a sound effect loop.
     * @param soundEffectIndex the index. Should be >0.
    */
    juce::String getSoundEffectLoopLabel(int soundEffectIndex) const noexcept;

    const Song& song;
    ExportConfiguration exportConfiguration;
};

}   // namespace arkostracker
