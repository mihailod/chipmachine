#pragma once

#include <juce_core/juce_core.h>

#include "../../../../export/playerConfiguration/PlayerConfiguration.h"
#include "../../../../export/sourceGenerator/SourceGeneratorConfiguration.h"
#include "../../../../utils/OptionalValue.h"

namespace arkostracker
{

/**
 * Takes care of saving the given source. It might also be compiled into Z80 (the source must be Z80!).
 * As a bonus, the configuration player can also be exported, as source.
 * Fill a possible ErrorReport.
 * This has no UI, so can be used by command line tools.
 */
class SaveSourceOrBinary
{
public:
    /**
     * Constructor.
     * @param primaryMemoryBlock the source to save/compile. It must already have the ORG if wanted (should be present if binary).
     * @param secondaryMemoryBlocks the possible other (subsong?) files, if relevant.
     * @param outputFile the base file to save. May already exist, will be overwritten without prompt. Player configuration and subsong files derive from it.
     * @param exportAsSeveralFiles true to save to different files.
     * @param saveToBinary true to save to binary, false to save as source. Always false if export as several files.
     * @param exportPlayerConfiguration true to export the player configuration.
     * @param playerConfiguration the player configuration.
     * @param sourceGeneratorConfigurationForPlayerConfiguration the source configuration, only used for exporting the player configuration.
     */
    SaveSourceOrBinary(juce::MemoryBlock primaryMemoryBlock,
                       std::vector<juce::MemoryBlock> secondaryMemoryBlocks,
                       juce::File outputFile,
                       bool exportAsSeveralFiles,
                       bool saveToBinary,
                       bool exportPlayerConfiguration, PlayerConfiguration playerConfiguration,
                       SourceGeneratorConfiguration sourceGeneratorConfigurationForPlayerConfiguration) noexcept;

    /**
     * Performs the save.
     * @return true if everything went fine.
     */
    bool perform() const noexcept;

private:
    /** Aggregates the sources. */
    juce::MemoryBlock aggregateSources() const noexcept;

    const juce::MemoryBlock primaryMemoryBlock;
    const std::vector<juce::MemoryBlock> secondaryMemoryBlocks;
    const juce::File outputFile;
    const bool saveToBinary;
    const bool exportAsSeveralFiles;
    const OptionalInt address;
    bool exportPlayerConfiguration;
    PlayerConfiguration playerConfiguration;
    SourceGeneratorConfiguration sourceGeneratorConfigurationForPlayerConfiguration;
};

}   // namespace arkostracker
