#include "ToolSongToYm.h"

#include <juce_core/juce_core.h>

#include "../../export/ym/YmExporter.h"
#include "../../export/ym/YmGenerator.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineArgumentDescriptor.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToYm::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const auto description = "Converts to YM any YM or any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToYm " + CommandLineToolHelper::getDescriptionForcedPsgFrequencyParameter() + " "
                                     + "[-s <subsong number>] [-p <psg number>] [-n] [-y] <path to input song> <path to output YM>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;

    CommandLineToolHelper commandLineToolHelper;

    const auto optionNonInterleaved = Option::buildOption("n", "non-interleaved");
    auto descriptorOptionNonInterleaved = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("Non-interleaved. Should be used only for specific needs. Default is interleaved (best for YM6, required by YM3)."), optionNonInterleaved, false);

    const auto optionIsYm3 = Option::buildOption("y", "ym3");
    auto descriptorOptionIsYm3 = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("Export to YM3. Should be used only for specific needs. Does not support non-interleaved mode. Default is YM6."), optionIsYm3, false);

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareForcedPsgFrequencyParameter(descriptors);
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declarePsgParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output YM>"), juce::translate("Path and filename to the YM file to create."), true);

    descriptors.push_back(&descriptorOptionNonInterleaved);
    descriptors.push_back(&descriptorOptionIsYm3);
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

    // Checks the possible forced PSG frequency.
    const auto targetPsgFrequency = commandLineToolHelper.getForcedPsgFrequency();
    if (!commandLineToolHelper.checkTargetPsgFrequencyAndDisplayErr()) {
        return -1;
    }

    // Not interleaved?
    const auto interleaved = !descriptorOptionNonInterleaved.isPresent();
    const auto isYm3 = descriptorOptionIsYm3.isPresent();

    if (isYm3 && !interleaved) {
        CommandLineToolHelper::cerr(juce::translate("YM3 does not support non-interleaved encoding! Remove the -n option."));
        return -1;
    }

    // Makes the export.
    std::pair<bool, std::unique_ptr<juce::MemoryOutputStream>> result;
    if (isSong) {
        // Reads the possible Subsong and PSG index.
        const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
        if (subsongId.isAbsent()) {
            return -1;
        }
        const auto psgIndex = commandLineToolHelper.getPsgIndexOrWriteError(*song, subsongId.getValueRef());
        if (psgIndex.isAbsent()) {
            return -1;
        }

        const auto exporter = std::make_unique<YmExporter>(song, subsongId.getValue(), psgIndex.getValue(), interleaved, isYm3, targetPsgFrequency);
        result = exporter->performTask();
    } else {
        // YM.
        jassert(parsedYm != nullptr);
        if (!commandLineToolHelper.checkSubsongIdAndPsgIndexAbsentElseDisplayErr()) {
            return -1;
        }

        auto outputStream = std::make_unique<juce::MemoryOutputStream>();
        YmGenerator ymGenerator(*parsedYm, *outputStream, targetPsgFrequency);
        const auto success = ymGenerator.convert();
        result = std::make_pair(success, std::move(outputStream));
    }

    if (!result.first) {
        CommandLineToolHelper::cerr(juce::translate("Export to YM failed!"));
        return -1;
    }

    const auto outputFileString = descriptorParameterOutput.getDirectValue();
    const auto success = FileUtil::saveMemoryBlockToFile(outputFileString, result.second->getMemoryBlock());
    if (!success) {
        CommandLineToolHelper::cout(juce::translate("Saving failed!"));
        return -1;
    }

    CommandLineToolHelper::cout(juce::translate("Export to YM successful."));
    return 0;
}

}   // namespace arkostracker
