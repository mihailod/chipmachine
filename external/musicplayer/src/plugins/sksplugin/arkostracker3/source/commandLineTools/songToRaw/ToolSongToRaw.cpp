#include "ToolSongToRaw.h"

#include "../../export/raw/RawExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToRaw::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    // Creates the command line.
    constexpr auto canDisableSamples = false;
    const auto description = "Generates a file in the raw export format, from any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToRaw "
                                     + CommandLineToolHelper::getDescriptionForSubsongParameter()
                                     + " [--dontEncodeSongSubsongMetadata] [--dontEncodeReferenceTables] [--dontEncodeSpeedTracks] [--dontEncodeEventTracks] [--dontEncodeInstruments] [--dontEncodeArpeggios] [--dontEncodePitches] [--dontEncodeEffects] [--dontEncodeEmptyLinesAsRLE] [--dontEncodetranspositionsInLinker] [--dontEncodeHeightsInLinker] "
                                     + " " + CommandLineToolHelper::getDescriptionForSampleExport(canDisableSamples)
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs()
                                     + " <path to input song> <path to output Raw file>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);

    // All the optional flags for the export.
    const auto optionDontEncodeSongSubsongMetadata = Option::buildLongOption("dontEncodeSongSubsongMetadata");
    const auto optionDontEncodeReferenceTables = Option::buildLongOption("dontEncodeReferenceTables");
    const auto optionDontEncodeSpeedTracks = Option::buildLongOption("dontEncodeSpeedTracks");
    const auto optionDontEncodeEventTracks = Option::buildLongOption("dontEncodeEventTracks");
    const auto optionDontEncodeInstruments = Option::buildLongOption("dontEncodeInstruments");
    const auto optionDontEncodeArpeggios = Option::buildLongOption("dontEncodeArpeggios");
    const auto optionDontEncodePitches = Option::buildLongOption("dontEncodePitches");
    const auto optionDontEncodeEffects = Option::buildLongOption("dontEncodeEffects");
    const auto optionDontEncodeEmptyLinesAsRLE = Option::buildLongOption("dontEncodeEmptyLinesAsRLE");
    const auto optionDontEncodeTranspositionsInLinker = Option::buildLongOption("dontEncodetranspositionsInLinker");
    const auto optionDontEncodeHeightsInLinker = Option::buildLongOption("dontEncodeHeightsInLinker");
    auto descriptorDontEncodeSongSubsongMetadata = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the song and subsong metadata are not encoded."), optionDontEncodeSongSubsongMetadata, false);
    auto descriptorDontEncodeReferenceTables = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the reference tables are not encoded."), optionDontEncodeReferenceTables, false);
    auto descriptorDontEncodeSpeedTracks = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the speed tracks are not encoded."), optionDontEncodeSpeedTracks, false);
    auto descriptorDontEncodeEventTracks = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the event tracks are not encoded."), optionDontEncodeEventTracks, false);
    auto descriptorDontEncodeInstruments = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the instruments are not encoded."), optionDontEncodeInstruments, false);
    auto descriptorDontEncodeArpeggios = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the arpeggios are not encoded."), optionDontEncodeArpeggios, false);
    auto descriptorDontEncodePitches = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the pitches are not encoded."), optionDontEncodePitches, false);
    auto descriptorDontEncodeEffects = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the effects are not encoded."), optionDontEncodeEffects, false);
    auto descriptorDontEncodeEmptyLinesAsRLE = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, empty lines are not encoded in RLE format."), optionDontEncodeEmptyLinesAsRLE, false);
    auto descriptorDontEncodeTranspositionsInLinker = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the transpositions are not encoded in the linker."), optionDontEncodeTranspositionsInLinker, false);
    auto descriptorDontEncodeHeightsInLinker = CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the heights are not encoded in the linker."), optionDontEncodeHeightsInLinker, false);
    descriptors.push_back(&descriptorDontEncodeSongSubsongMetadata);
    descriptors.push_back(&descriptorDontEncodeReferenceTables);
    descriptors.push_back(&descriptorDontEncodeSpeedTracks);
    descriptors.push_back(&descriptorDontEncodeEventTracks);
    descriptors.push_back(&descriptorDontEncodeInstruments);
    descriptors.push_back(&descriptorDontEncodeArpeggios);
    descriptors.push_back(&descriptorDontEncodePitches);
    descriptors.push_back(&descriptorDontEncodeEffects);
    descriptors.push_back(&descriptorDontEncodeEmptyLinesAsRLE);
    descriptors.push_back(&descriptorDontEncodeTranspositionsInLinker);
    descriptors.push_back(&descriptorDontEncodeHeightsInLinker);

    commandLineToolHelper.declareExportSamplesParameter(descriptors, canDisableSamples);
    commandLineToolHelper.declareExportAsParameter(descriptors);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path to output Raw file."),
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

    // Reads the possible Subsong index.
    const auto subsongId = commandLineToolHelper.getSubsongIdOrWriteError(*song);
    if (subsongId.isAbsent()) {
        return -1;
    }

    // Reads the source profile or custom one.
    const auto sourceConfiguration = commandLineToolHelper.getSourceConfigurationOrDisplayErr();
    if (sourceConfiguration == nullptr) {
        return -1;
    }
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { subsongId.getValue() }, baseLabel, orgAddress,
        SampleEncoderFlags());

    // Builds the raw encoded flags.
    const EncodedDataFlag encodedDataFlag(
        !descriptorDontEncodeSongSubsongMetadata.isPresent(),
        !descriptorDontEncodeReferenceTables.isPresent(),
        !descriptorDontEncodeSpeedTracks.isPresent(),
        !descriptorDontEncodeEventTracks.isPresent(),
        !descriptorDontEncodeInstruments.isPresent(),
        !descriptorDontEncodeArpeggios.isPresent(),
        !descriptorDontEncodePitches.isPresent(),
        !descriptorDontEncodeEffects.isPresent(),
        !descriptorDontEncodeEmptyLinesAsRLE.isPresent(),
        !descriptorDontEncodeTranspositionsInLinker.isPresent(),
        !descriptorDontEncodeHeightsInLinker.isPresent(),
        0.0
    );

    // Makes the export.
    RawExporter exporter(*song, encodedDataFlag, exportConfiguration);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to RAW failed!"));
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

    CommandLineToolHelper::cout(juce::translate("Export to RAW successful."));
    return 0;
}

}   // namespace arkostracker
