#include "ToolSongToSfx.h"

#include "../../export/sfx/SfxExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToSfx::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const auto description = "Converts to sound effects any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToSoundEffects " + CommandLineToolHelper::getDescriptionForExportAs(false) + " <path to input song> <path to output AKX>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors, false);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the AKX file to create."), true);

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

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { song->getFirstSubsongId() }, baseLabel, orgAddress);

    // Makes the export.
    SfxExporter exporter(*song, exportConfiguration);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to AKX failed!"));
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
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to AKX successful."));
    return 0;
}

}   // namespace arkostracker
