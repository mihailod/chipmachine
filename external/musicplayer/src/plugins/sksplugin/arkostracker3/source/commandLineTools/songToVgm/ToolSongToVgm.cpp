#include "ToolSongToVgm.h"

#include "../../export/vgm/VgmExporter.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToVgm::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const auto description = "Converts to VGM any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToVgm " + CommandLineToolHelper::getDescriptionForSubsongParameter() +" [-p <psg numbers>] "
                                     "[-n] <path to input song> <path to output VGM>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;

    CommandLineToolHelper commandLineToolHelper;

    const auto optionNoCompression = Option::buildOption("n", "noCompression");
    auto descriptorOptionNoCompression = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("Use it to avoid compression. Should not be usually used."), optionNoCompression, false);

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output VGM>"), juce::translate("Path and filename to the VGM file to create."), true);

    descriptors.push_back(&descriptorOptionNoCompression);
    descriptors.push_back(&descriptorParameterOutput);

    // Starts parsing.
    const auto parseResult = commandLineToolHelper.parseAndGetSong(argc, argv, descriptors, description);
    if (parseResult.second == nullptr) {
        return parseResult.first ? 0 : -1;
    }
    const auto song = parseResult.second;

    // Reads the possible Subsong and PSG index.
    const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
    if (subsongId.isAbsent()) {
        return -1;
    }

    // Compressed?
    const auto compress = !descriptorOptionNoCompression.isPresent();

    // Preliminary check (warning only).
    const auto checkResult = VgmExporter::performPreliminaryCheck(*song, subsongId.getValue());
    switch (checkResult.first) {
        case VgmExporter::PreliminaryCheck::tooManyPsgs: [[fallthrough]];
        case VgmExporter::PreliminaryCheck::differentPsgFrequencies:
            CommandLineToolHelper::cerr(checkResult.second);
            break;
        case VgmExporter::PreliminaryCheck::ok:
            break;
    }

    // Makes the export.
    VgmExporter exporter(song, subsongId.getValue(), compress);
    const auto result = exporter.performTask();
    if (!result.first) {
        CommandLineToolHelper::cerr(juce::translate("Export to VGM failed!"));
        return -1;
    }

    const auto outputFileString = descriptorParameterOutput.getDirectValue();
    const auto success = FileUtil::saveMemoryBlockToFile(outputFileString, result.second->getMemoryBlock());
    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to VGM successful."));
    return 0;
}

}   // namespace arkostracker
