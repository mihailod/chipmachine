#include "ToolSongToSamples.h"

#include "../../export/samples/SampleExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToSamples::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    constexpr auto canDisableSamples = false;
    constexpr auto canIgnoreUselessAndGenerateIndexTable = true;

    // Creates the command line.
    const auto description = "Generates a file with the samples from any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToSamples "
                                     + CommandLineToolHelper::getDescriptionForSampleExport(canDisableSamples, canIgnoreUselessAndGenerateIndexTable)
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs(false)
                                     + " <path to input song> <path to output Samples file>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareExportSamplesParameter(descriptors, canDisableSamples, canIgnoreUselessAndGenerateIndexTable);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors, false);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the Samples file to create."),
            true);

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

    const auto subsongId = song->getFirstSubsongId();       // Subsong index does not matter.

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const auto sampleEncoderFlags = commandLineToolHelper.getSampleEncoderFlags();
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { subsongId }, baseLabel, orgAddress, sampleEncoderFlags);

    // Makes the export.
    constexpr auto pitch = 0.0;
    SampleExporter exporter(*song, exportConfiguration, pitch);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to samples failed!"));
        return -1;
    }

    const auto outputFile = FileUtil::getFileFromString(descriptorParameterOutput.getDirectValue());
    constexpr auto exportAsSeveralFiles = false;
    constexpr auto exportPlayerConfiguration = false;

    const auto sourceMemoryBlock = exportResult.second->getAggregatedData();
    const auto playerConfiguration = exportResult.second->getPlayerConfigurationRef();

    // Saves to source or binary.
    const SaveSourceOrBinary saveSourceOrBinary(sourceMemoryBlock, {}, outputFile, exportAsSeveralFiles, saveToBinary, exportPlayerConfiguration,
                                          playerConfiguration, *sourceConfiguration);
    const auto success = saveSourceOrBinary.perform();

    if (!success) {
        CommandLineToolHelper::cerr(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to samples successful."));
    return 0;
}

}   // namespace arkostracker
