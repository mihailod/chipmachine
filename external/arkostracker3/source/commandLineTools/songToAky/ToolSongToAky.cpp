#include "ToolSongToAky.h"

#include "../../export/aky/AkyExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToAky::execute(int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const auto description = "Converts to AKY any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToAky "
                                     + CommandLineToolHelper::getDescriptionForSubsongParameter()
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs()
                                     + " <path to input song> <path to output AKY>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the AKY file to create."), true);
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

    // Reads the possible Subsong and PSG index.
    const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
    if (subsongId.isAbsent()) {
        return -1;
    }

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { subsongId.getValue() }, baseLabel, orgAddress);

    // Makes the export.
    AkyExporter exporter(song, exportConfiguration);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to AKY failed!"));
        return -1;
    }

    const auto outputFile = FileUtil::getFileFromString(descriptorParameterOutput.getDirectValue());
    constexpr auto exportAsSeveralFiles = false;
    const auto exportPlayerConfiguration = commandLineToolHelper.exportPlayerConfiguration();

    const auto sourceMemoryBlock = exportResult.second->getAggregatedData();
    const auto playerConfiguration = exportResult.second->getPlayerConfigurationRef();

    // Saves to source or binary.
    const SaveSourceOrBinary saveSourceOrBinary(sourceMemoryBlock, {}, outputFile, exportAsSeveralFiles, saveToBinary, exportPlayerConfiguration,
                                          playerConfiguration, *sourceConfiguration);
    const auto success = saveSourceOrBinary.perform();

    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to AKY successful."));
    return 0;
}

}   // namespace arkostracker
