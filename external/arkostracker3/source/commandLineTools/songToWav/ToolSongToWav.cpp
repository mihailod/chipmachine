#include "ToolSongToWav.h"

#include "../../controllers/model/OutputMix.h"
#include "../../export/wav/SongWavExporter.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StringUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

const juce::String ToolSongToWav::parameterMixingFlat = "flat";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ToolSongToWav::parameterMixingCpc = "cpc";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ToolSongToWav::parameterMixingCustom = "custom:";     // Start of the expression. NOLINT(cert-err58-cpp, *-statically-constructed-objects)

int ToolSongToWav::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    const juce::String description = "Converts to WAV any song that can be loaded into Arkos Tracker 3.\nUsage: SongToWav "
                                     "[-s <subsong number>] [-f <frequency>] [-b <bit rate>] [-c <channel count>] "
                                     "[-l <additional loop count>] [-i] "
                                     "<path to input song> <path to output WAV>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;

    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);

    auto optionFrequency = Option::buildOption("f", "frequency");
    Parameter parameterFrequency(ParameterType::integer);
    auto descriptorOptionFrequency = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("The output frequency, in Hz. 44100 is default."), optionFrequency, false, parameterFrequency);

    auto optionAdditionalLoopCount = Option::buildOption("l", "loopcount");
    Parameter parameterAdditionalLoopCount(ParameterType::integer);
    auto descriptorOptionAdditionalLoopCount = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("How many additional loop to add. 0 is default."), optionAdditionalLoopCount, false, parameterAdditionalLoopCount);

    auto optionExportEachChannelIndividually = Option::buildShortOption("i");
    auto descriptorExportEachChannelIndividually = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("If present, each channel is exported individually in its own file."), optionExportEachChannelIndividually, false);

    auto optionMixing = Option::buildOption("m", "mix");
    Parameter parameterMixing(ParameterType::string);
    auto descriptorOptionMixing = CommandLineArgumentDescriptor::buildArgumentWithOption(translate("How to mix: \""
            + parameterMixingFlat + "\" (default), \""
            + parameterMixingCpc + "\" (100, " +
            juce::String(PsgValues::cpcStereoChannelMiddleRatio * 100.0F) +
            ", 100), \""
            + parameterMixingCustom +
            "<left>,<center>,<right>\" (in percent)."),
            optionMixing, false, parameterMixing);

    auto optionStereoSeparation = Option::buildOption("x", "stereoseparation");
    Parameter parameterStereoSeparation(ParameterType::integer);
    auto descriptorOptionStereoSeparation = CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("Stereo separation, from 0 (mono) to 100 (full stereo, default)."), optionStereoSeparation, false, parameterStereoSeparation);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(juce::translate("<path to output WAV>"),
                                                                                                 juce::translate("Path and filename to the WAV file to create."), true);

    descriptors.push_back(&descriptorOptionFrequency);
    descriptors.push_back(&descriptorOptionAdditionalLoopCount);
    descriptors.push_back(&descriptorOptionMixing);
    descriptors.push_back(&descriptorOptionStereoSeparation);
    descriptors.push_back(&descriptorExportEachChannelIndividually);
    descriptors.push_back(&descriptorParameterOutput);

    // Starts parsing.
    const auto parseResult = commandLineToolHelper.parseAndGetSong(argc, argv, descriptors, description);
    if (parseResult.second == nullptr) {
        return parseResult.first ? 0 : -1;
    }
    const auto song = parseResult.second;

    // Gets the result of the command line.
    const auto inputFileString = commandLineToolHelper.getInputSong();
    const auto outputFileString = descriptorParameterOutput.getDirectValue();

    // Reads the possible frequency.
    auto frequency = 44100;
    constexpr auto minimumFrequency = 1000;
    constexpr auto maximumFrequency = 192000;
    auto success = true;
    if (parameterFrequency.isPresent()) {
        frequency = parameterFrequency.getValueAsIntegerWithFailureTest(success);
        if (!success || (frequency < minimumFrequency) || (frequency > maximumFrequency)) {
            CommandLineToolHelper::cerr("The frequency must be from " + juce::String(minimumFrequency) + " and " + juce::String(maximumFrequency) + " Hz.");
            return -1;
        }
    }

    // Reads the possible loop count.
    auto additionalLoopCount = 0;
    if (parameterAdditionalLoopCount.isPresent()) {
        additionalLoopCount = parameterAdditionalLoopCount.getValueAsIntegerWithFailureTest(success);
        if (!success || (additionalLoopCount < 0)) {
            CommandLineToolHelper::cerr("The additional loop count must be from 0 or more.");
            return -1;
        }
    }

    // Reads the possible stereo separation.
    auto stereoSeparation = 100;
    if (parameterStereoSeparation.isPresent()) {
        stereoSeparation = parameterStereoSeparation.getValueAsIntegerWithFailureTest(success);
        if (!success || (stereoSeparation < 0) || (stereoSeparation > 100)) {
            CommandLineToolHelper::cerr("Invalid stereo separation value.");
            return -1;
        }
    }

    // Reads the possible mixing output.
    auto volumeLeft = 100;
    auto volumeCenter = 100;
    auto volumeRight = 100;
    success = readMixingOptions(parameterMixing, volumeLeft, volumeCenter, volumeRight);
    if (!success) {
        return -1;
    }
    const OutputMix outputMix(100, volumeLeft, volumeCenter, volumeRight, false, stereoSeparation);

    // Reads the possible subsong index.
    const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
    if (subsongId.isAbsent()) {
        return -1;
    }

    // Export each channel individually?
    const auto exportEachChannelIndividually = descriptorExportEachChannelIndividually.isPresent();


    // Makes the export.
    SongWavExporter songWavExporter(song, subsongId.getValue(), frequency, additionalLoopCount, outputMix, outputFileString, exportEachChannelIndividually);

    return CommandLineToolHelper::performTaskWithNoResult(songWavExporter, juce::translate("Export to WAV successful."), juce::translate("Export to WAV failed!"));
}


/**
 * Reads the mixing options. In case of error, an error message will be displayed.
 * @param parameterMixing the mixing parameter, containing what the user typed.
 * @param volumeLeft the left volume with a default value, may be modified.
 * @param volumeCenter the center volume with a default value, may be modified.
 * @param volumeRight the right volume with a default value, may be modified.
 * @return false if an error occurred.
 */
bool ToolSongToWav::readMixingOptions(const Parameter& parameterMixing, int& volumeLeft, int& volumeCenter, int& volumeRight) noexcept
{
    // Nothing to do if the mixing parameter is not present.
    if (!parameterMixing.isPresent()) {
        return true;
    }

    const auto value = parameterMixing.getValueAsString();
    if (value == parameterMixingFlat) {
        return true;            // Nothing to do, the input volume are already flat.
    }

    if (value == parameterMixingCpc) {
        volumeLeft = 100;
        volumeCenter = static_cast<int>(PsgValues::cpcStereoChannelMiddleRatio * 100.0F);
        volumeRight = 100;
        return true;
    }

    if (!value.startsWith(parameterMixingCustom)) {
        CommandLineToolHelper::cerr(juce::translate("Unknown mixing option."));
        return false;
    }

    // Custom: reads the 3 values, comma separated.
    const auto values = value.substring(parameterMixingCustom.length());
    auto volumes = StringUtil::split(values, ",");
    if (volumes.size() != 3U) {
        CommandLineToolHelper::cerr(juce::translate("Three percents must be given."));
        return false;
    }
    volumeLeft = volumes.at(0U).getIntValue();
    volumeCenter = volumes.at(1U).getIntValue();
    volumeRight = volumes.at(2U).getIntValue();
    constexpr auto percentMax = 100;
    constexpr auto percentMin = 0;
    static_assert(percentMax == 100);       // Else, need to correct the texts below!
    static_assert(percentMin == 0);         // Else, need to correct the texts below!

    if ((volumeLeft < percentMin) || (volumeLeft > percentMax)) {
        CommandLineToolHelper::cerr(juce::translate("The left volume must be between 0 and 100."));
        return false;
    }
    if ((volumeCenter < percentMin) || (volumeCenter > percentMax)) {
        CommandLineToolHelper::cerr(juce::translate("The center volume must be between 0 and 100."));
        return false;
    }
    if ((volumeRight < percentMin) || (volumeRight > percentMax)) {
        CommandLineToolHelper::cerr(juce::translate("The right volume must be between 0 and 100."));
        return false;
    }

    return true;
}

}   // namespace arkostracker
