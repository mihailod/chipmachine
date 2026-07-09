#include "ToolSongToAkg.h"

#include "../../export/akg/AkgExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToAkg::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    constexpr auto canDisableSamples = true;
    const auto description = "Converts to AKG any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToAkg "
                                     + CommandLineToolHelper::getDescriptionForSubsongsParameter()
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs()
                                     + " " + CommandLineToolHelper::getDescriptionForExportSeveralFiles()
                                     + " " + CommandLineToolHelper::getDescriptionForSampleExport(canDisableSamples)
                                     + " <path to input song> <path to output AKG>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongsParameter(descriptors);
    commandLineToolHelper.declareExportSeveralFilesParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors);
    commandLineToolHelper.declareExportSamplesParameter(descriptors, canDisableSamples);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the AKG file to create."), true);
    descriptors.push_back(&descriptorParameterOutput);

    // Starts parsing.
    const auto parseResult = commandLineToolHelper.parseAndGetSong(argc, argv, descriptors, description);
    if (parseResult.second == nullptr) {
        return parseResult.first ? 0 : -1;
    }
    const auto song = parseResult.second;

    const auto baseLabel = commandLineToolHelper.getLabelPrefix();
    const auto orgAddress = commandLineToolHelper.getEncodingAddress();
    const auto saveToBinary = commandLineToolHelper.encodeAsBinary();
    if (!CommandLineToolHelper::checkExportAsValidityAndDisplayErr(saveToBinary, orgAddress)) {
        return -1;
    }
    const auto subsongIds = commandLineToolHelper.getSubsongIdsOrWriteError(*song);
    if (subsongIds.empty()) {
        return -1;
    }

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }

    // Builds the sample flags. Checks and gets the "sample export" parameters first.
    if (!commandLineToolHelper.checkExportSamplesValidityAndDisplayErr()) {
        return -1;
    }
    const auto sampleEncoderFlags = commandLineToolHelper.getSampleEncoderFlags();

    const ExportConfiguration exportConfiguration(*sourceConfiguration, subsongIds, baseLabel, orgAddress, sampleEncoderFlags);

    // Makes the export.
    AkgExporter exporter(*song, exportConfiguration);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to AKG failed!"));
        return -1;
    }

    const auto outputFile = FileUtil::getFileFromString(descriptorParameterOutput.getDirectValue());
    const auto exportAsSeveralFiles = commandLineToolHelper.isExportToSeveralFiles();
    const auto exportPlayerConfiguration = commandLineToolHelper.exportPlayerConfiguration();

    const auto sourceMemoryBlock = exportResult.second->getSongData();
    const auto sourceSecondaryMemoryBlocks = exportResult.second->getSubsongData();
    const auto playerConfiguration = exportResult.second->getPlayerConfigurationRef();

    // Saves to source or binary.
    const SaveSourceOrBinary saveSourceOrBinary(sourceMemoryBlock, sourceSecondaryMemoryBlocks, outputFile, exportAsSeveralFiles, saveToBinary,
                                          exportPlayerConfiguration, playerConfiguration, *sourceConfiguration);
    const auto success = saveSourceOrBinary.perform();

    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to AKG successful."));
    return 0;
}

}   // namespace arkostracker
