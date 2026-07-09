#pragma once

#include <juce_core/juce_core.h>

#include <utility>

#include "../../song/Song.h"
#include "../../utils/task/Task.h"
#include "../sourceGenerator/SourceGeneratorConfiguration.h"

namespace arkostracker
{

class FapResult
{
public:
    FapResult(juce::MemoryBlock pFapData, juce::MemoryBlock pConstantSourceData, const int pBufferSize, const int pPlayTimeInNops,
        const int pRegisterCountToPlay, const bool pIsR12Constant) :
            fapData(std::move(pFapData)),
            constantSourceData(std::move(pConstantSourceData)),
            bufferSize(pBufferSize),
            playTimeInNops(pPlayTimeInNops),
            registerCountToPlay(pRegisterCountToPlay),
            isR12Constant(pIsR12Constant)
    {
    }

    const juce::MemoryBlock fapData;
    const juce::MemoryBlock constantSourceData;
    const int bufferSize;
    const int playTimeInNops;
    const int registerCountToPlay;
    const bool isR12Constant;
};

/** Encoder to the FAP format, as a binary file. */
class FapExporter final : public Task<std::unique_ptr<FapResult>>
{
public:
    static const juce::String constantSourceFileSuffix;

    /**
     * Constructor to convert a Song to FAP.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to export.
     * @param sourceConfiguration the source configuration for the constant source.
     * @param baseLabel the possible base label.
     * @param targetPsgFrequency the possible PSG frequency to apply.
     */
    FapExporter(const std::shared_ptr<Song>& song, Id subsongId, SourceGeneratorConfiguration sourceConfiguration, juce::String baseLabel,
        OptionalInt targetPsgFrequency) noexcept;

    /**
     * Constructor to convert a YM to FAP.
     * @param ym the YM data. It can be zipped! May be in any format or any interleave mode. The frequency may be changed if wanted.
     * @param sourceConfiguration the source configuration for the constant source.
     * @param baseLabel the possible base label.
     * @param targetPsgFrequency the possible PSG frequency to apply.
     */
    FapExporter(juce::MemoryBlock ym, SourceGeneratorConfiguration sourceConfiguration, juce::String baseLabel, OptionalInt targetPsgFrequency) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<FapResult>> performTask() noexcept override;
    void onAskedToCancelTask() noexcept override;

private:
    /**
     * Builds a FAP song.
     * @param ymMemoryBlock the YM. Must be valid.
     * @return a success boolean, and the FAP data.
     */
    std::pair<bool, std::unique_ptr<FapResult>> performTask(const juce::MemoryBlock& ymMemoryBlock) const noexcept;

    /** @return a source containing the constants of FAP. */
    std::unique_ptr<juce::MemoryBlock> buildConstantSourceMemoryBlock(int bufferSize, int playTimeInNops, int registerCountToPlay, bool isR12Constant) const noexcept;

    std::shared_ptr<Song> song;             // Nullptr if not used.
    juce::MemoryBlock inputYm;              // Empty if not used. It may be zipped!
    OptionalInt forcedTargetPsgFrequency;
    Id subsongId;
    SourceGeneratorConfiguration sourceConfiguration;
    juce::String baseLabel;
    std::unique_ptr<Task<std::unique_ptr<juce::MemoryOutputStream>>> ymExporter;

    std::unique_ptr<juce::MemoryBlock> outputStream;
};

}   // namespace arkostracker
