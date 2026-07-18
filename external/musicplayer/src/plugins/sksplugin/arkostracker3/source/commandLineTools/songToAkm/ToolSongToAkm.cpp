#include "ToolSongToAkm.h"

#include "../../export/akm/AkmExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToAkm::execute(int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const auto description = "Converts to AKM any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToAkm "
                                     + CommandLineToolHelper::getDescriptionForSubsongsParameter()
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs()
                                     + " " + CommandLineToolHelper::getDescriptionForExportSeveralFiles()
                                     + " " + CommandLineToolHelper::getDescriptionForTreatWarningAsError()
                                     + " <path to input song> <path to output AKM>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongsParameter(descriptors);
    commandLineToolHelper.declareExportSeveralFilesParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors);
    commandLineToolHelper.declareTreatWarningAsError(descriptors);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the AKM file to create."), true);
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

    // Reads the possible Subsongs.
    const auto subsongIds = commandLineToolHelper.getSubsongIdsOrWriteError(*song);
    if (subsongIds.empty()) {
        return -1;
    }

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const ExportConfiguration exportConfiguration(*sourceConfiguration, subsongIds, baseLabel, orgAddress);

    // Makes the export.
    AkmExporter exporter(*song, exportConfiguration);
    const auto exportResult = exporter.performTask();
    // Shows the warnings.
    auto success = CommandLineToolHelper::displayAndCheckStatusReport(exportResult.second->getErrorReportRef(), commandLineToolHelper.isWarningTreatedAsError());
    if (!success) {
        return -1;
    }

    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to AKM failed!"));
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
    success = saveSourceOrBinary.perform();

    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to AKM successful."));
    return 0;
}

}   // namespace arkostracker
