#include "ToolSongToRawLinear.h"

#include "../../export/rawLinear/RawLinearExport.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToRawLinear::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    const auto exportTypeAll = juce::translate("all");
    const auto exportTypePsgOnly = juce::translate("psgOnly");
    const auto exportTypeSamplesOnly = juce::translate("samplesOnly");

    // Creates the command line.
    const auto description = "Generates a file in a Raw Linear format, from any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToRawLinear "
                                     "[--exportType " + exportTypeAll + "/" + exportTypeSamplesOnly + "/" + exportTypePsgOnly + "] "
                                     + CommandLineToolHelper::getDescriptionForExportAs(false)
                                     + " <path to input song> <path to output Samples file>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    const auto optionExportType = std::make_unique<Option>(Option::buildOption("et", "exportType"));
    const auto parameterExportType = std::make_unique<Parameter>(ParameterType::string);
    const auto descriptorOptionExportType = std::make_unique<CommandLineArgumentDescriptor>(
        CommandLineArgumentDescriptor::buildArgumentWithOption(
            juce::translate("What export type (among ") + exportTypeAll + ", " + exportTypePsgOnly + ", " + exportTypeSamplesOnly + "). "
            "Default is " + exportTypeAll + ".",
            *optionExportType, false, *parameterExportType));
    descriptors.push_back(descriptorOptionExportType.get());

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors, false);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the Raw Linear file to create."),
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
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { subsongId }, baseLabel, orgAddress);

    // What types of events?
    std::set<InstrumentType> instrumentTypes;
    if (parameterExportType->isPresent()) {
        const auto eventType = parameterExportType->getValueAsString();
        if (eventType == exportTypeAll) {
            instrumentTypes.emplace(InstrumentType::psgInstrument);
            instrumentTypes.emplace(InstrumentType::sampleInstrument);
        } else if (eventType == exportTypeSamplesOnly) {
            instrumentTypes.emplace(InstrumentType::sampleInstrument);
        } else if (eventType == exportTypePsgOnly) {
            instrumentTypes.emplace(InstrumentType::psgInstrument);
        } else {
            CommandLineToolHelper::cerr(juce::translate("Export type is unknown: " + eventType + "."));
            return -1;
        }
    } else {
        instrumentTypes.emplace(InstrumentType::psgInstrument);
        instrumentTypes.emplace(InstrumentType::sampleInstrument);
    }

    // Makes the export.
    RawLinearExporter exporter(song, exportConfiguration, instrumentTypes);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to Raw Linear failed!"));
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

    CommandLineToolHelper::cout(juce::translate("Export to Raw Linear successful."));
    return 0;
}

}   // namespace arkostracker
