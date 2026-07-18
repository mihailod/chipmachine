#include "ToolSongToFap.h"

#include "../../ProjectInfo.h"
#include "../../export/fap/FapExporter.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToFap::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    const auto* fapVersion = projectinfo::fapVersion;

    // Creates the command line.
    constexpr auto showExportAsBinary = false;
    constexpr auto showExportPlayerConfig = false;

    const auto description = juce::String("Converts to FAP any YM or song that can be loaded into Arkos Tracker 3. FAP version: ") + fapVersion + ".\n"
                                     "Usage: SongToFap " + CommandLineToolHelper::getDescriptionForcedPsgFrequencyParameter() +
                                     " " + CommandLineToolHelper::getDescriptionForSubsongParameter() +
                                     " " + CommandLineToolHelper::getDescriptionForExportAs(showExportPlayerConfig, showExportAsBinary) +
                                     " <path to input song> <path to output FAP>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;

    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareForcedPsgFrequencyParameter(descriptors);
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors, showExportPlayerConfig, showExportAsBinary);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output FAP>"), juce::translate("Path and filename to the FAP file to create."), true);

    descriptors.push_back(&descriptorParameterOutput);

    // Starts parsing. Can be a YM.
    auto parseResult = commandLineToolHelper.parseAndGetSong(argc, argv, descriptors, description, true);
    const auto song = std::get<1>(parseResult);
    auto parsedYm = std::move(std::get<2>(parseResult));
    // If success but there is no song/YM, only shows the help, no error.
    if ((song == nullptr) && (parsedYm == nullptr)) {
        return std::get<0>(parseResult) ? 0 : -1;
    }

    const auto isSong = (song != nullptr);

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const auto baseLabel = commandLineToolHelper.getLabelPrefix();

    // Checks the possible forced PSG frequency.
    const auto targetPsgFrequency = commandLineToolHelper.getForcedPsgFrequency();
    if (!commandLineToolHelper.checkTargetPsgFrequencyAndDisplayErr()) {
        return -1;
    }

    // Makes the export.
    std::unique_ptr<FapExporter> exporter;
    if (isSong) {
        // Reads the possible Subsong and PSG index.
        const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
        if (subsongId.isAbsent()) {
            return -1;
        }
        exporter = std::make_unique<FapExporter>(song, subsongId.getValue(), *sourceConfiguration, baseLabel, targetPsgFrequency);
    } else {
        // YM.
        jassert(parsedYm != nullptr);
        if (!commandLineToolHelper.checkSubsongIdAndPsgIndexAbsentElseDisplayErr()) {
            return -1;
        }
        exporter = std::make_unique<FapExporter>(*parsedYm, *sourceConfiguration, baseLabel, targetPsgFrequency);
    }

    const auto& [fapSuccess, fapResult] = exporter->performTask();
    const auto& [fapData, constantSourceMemoryBlock, bufferSize, playTimeInNops, registerCountToPlay, isR12Constant] = *fapResult;
    if (!fapSuccess) {
        CommandLineToolHelper::cerr(juce::translate("Export to FAP failed!"));
        return -1;
    }
    if (bufferSize <= 0) {
        CommandLineToolHelper::cerr(juce::translate("Buffer size is illegal, export failed!"));
        return -1;
    }

    const auto outputFileString = descriptorParameterOutput.getDirectValue();
    auto success = FileUtil::saveMemoryBlockToFile(outputFileString, fapData);

    // Also saves the constants source file.
    if (success) {
        const auto outputFile = FileUtil::getFileFromString(outputFileString);
        const auto constantFileToSaveTo = FileUtil::buildFileWithSuffixAndExtension(outputFile, FapExporter::constantSourceFileSuffix, sourceConfiguration->getSourceFileExtension());
        success = FileUtil::saveMemoryBlockToFile(constantFileToSaveTo, constantSourceMemoryBlock);
    }

    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to FAP successful."));
    return 0;
}

}   // namespace arkostracker
