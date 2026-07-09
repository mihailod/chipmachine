#include "SaveSourceOrBinary.h"

#include "../../../../export/playerConfiguration/PlayerConfigurationExporter.h"
#include "../../../../utils/FileUtil.h"
#include "CompileSource.h"

namespace arkostracker
{
SaveSourceOrBinary::SaveSourceOrBinary(juce::MemoryBlock pPrimaryMemoryBlock, std::vector<juce::MemoryBlock> pSecondaryMemoryBlocks,
                                       juce::File pOutputFile, bool pExportAsSeveralFiles,
                                       bool pSaveToBinary, bool pExportPlayerConfiguration,
                                       PlayerConfiguration pPlayerConfiguration,
                                       SourceGeneratorConfiguration pSourceGeneratorConfigurationForPlayerConfiguration) noexcept :
        primaryMemoryBlock(std::move(pPrimaryMemoryBlock)),
        secondaryMemoryBlocks(std::move(pSecondaryMemoryBlocks)),
        outputFile(std::move(pOutputFile)),
        saveToBinary(pSaveToBinary),
        exportAsSeveralFiles(pExportAsSeveralFiles),
        exportPlayerConfiguration(pExportPlayerConfiguration),
        playerConfiguration(std::move(pPlayerConfiguration)),
        sourceGeneratorConfigurationForPlayerConfiguration(std::move(pSourceGeneratorConfigurationForPlayerConfiguration))
{
}

bool SaveSourceOrBinary::perform() const noexcept
{
    bool success;   // NOLINT(*-init-variables)
    if (saveToBinary) {
        // Compiles the source to binary.
        // Agglomerates the sources, because cannot export in binary into several files.
        const auto aggregatedSources = aggregateSources();
        const auto binaryMemoryBlock = CompileSource::compile(aggregatedSources);
        success = (binaryMemoryBlock != nullptr);
        if (success) {
            // Saves the binary.
            success = FileUtil::saveMemoryBlockToFile(outputFile, *binaryMemoryBlock);
        }
    } else {
        // Source. Saves the data into one, or more files.
        if (exportAsSeveralFiles) {
            // Saves sources into their own file.
            success = FileUtil::saveMemoryBlockToFile(outputFile, primaryMemoryBlock);
            auto subsongIndex = 0;
            for (const auto& subsongSource : secondaryMemoryBlocks) {
                // Builds the new file name, from the one from the base file.
                const auto subsongFilenameWithoutExtension = outputFile.getFileNameWithoutExtension() + "_s" + juce::String(subsongIndex);
                const auto finalPath = outputFile.getParentDirectory().getFullPathName() +
                                       juce::File::getSeparatorString() + subsongFilenameWithoutExtension + outputFile.getFileExtension();
                const juce::File subsongFile(finalPath);

                success = success && FileUtil::saveMemoryBlockToFile(subsongFile, subsongSource);
                ++subsongIndex;
            }
        } else {
            // Saves sources into one file.
            const auto aggregatedSources = aggregateSources();
            success = FileUtil::saveMemoryBlockToFile(outputFile, aggregatedSources);
        }
    }

    // Export the Player Configuration?
    if (success && exportPlayerConfiguration) {
        const auto playerConfigurationMemoryBlock = PlayerConfigurationExporter::exportConfiguration(
                sourceGeneratorConfigurationForPlayerConfiguration,
                playerConfiguration);

        // Creates xxx_playerconfiguration.asm.
        const auto playerConfigurationOutputFile = FileUtil::buildFileWithSuffixAndExtension(outputFile, PlayerConfigurationExporter::suffixGeneratedFilename,
            sourceGeneratorConfigurationForPlayerConfiguration.getSourceFileExtension());
        success = FileUtil::saveMemoryBlockToFile(playerConfigurationOutputFile, playerConfigurationMemoryBlock);
    }

    return success;
}

juce::MemoryBlock SaveSourceOrBinary::aggregateSources() const noexcept
{
    auto aggregatedSources = primaryMemoryBlock;
    for (const auto& otherSource : secondaryMemoryBlocks) {
        aggregatedSources.append(otherSource.getData(), otherSource.getSize());
    }
    return aggregatedSources;
}

}   // namespace arkostracker
