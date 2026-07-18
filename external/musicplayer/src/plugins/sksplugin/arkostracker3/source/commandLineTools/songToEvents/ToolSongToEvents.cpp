#include "ToolSongToEvents.h"

#include "../../export/events/EventsExporter.h"
#include "../../ui/export/common/task/SaveSourceOrBinary.h"
#include "../../utils/FileUtil.h"
#include "../utils/CommandLineToolHelper.h"

namespace arkostracker
{

int ToolSongToEvents::execute(int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    const auto guiInit = CommandLineToolHelper::initJuce();

    const auto eventTypeAll = juce::translate("all");
    const auto eventTypeSample = juce::translate("sample");
    const auto eventTypeEvent = juce::translate("event");

    // Creates the command line.
    const auto description = "Generates a file in the Events export format, from any song that can be loaded into Arkos Tracker 3.\n"
                                     "Usage: SongToEvents "
                                     "[--eventType " + eventTypeAll + "/" + eventTypeSample + "/" + eventTypeEvent + "] "
                                     + CommandLineToolHelper::getDescriptionForSubsongParameter()
                                     + " " + CommandLineToolHelper::getDescriptionForExportAs(false)
                                     + " <path to input song> <path to output Events file>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;
    CommandLineToolHelper commandLineToolHelper;

    const auto optionEventType = std::make_unique<Option>(Option::buildOption("et", "eventType"));
    const auto parameterEventType = std::make_unique<Parameter>(ParameterType::string);
    const auto descriptorOptionEventType = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(
                juce::translate("What type of events to export (among ") + eventTypeAll + ", " + eventTypeSample + ", " + eventTypeEvent + "). Default is " + eventTypeAll + ".",
                *optionEventType, false, *parameterEventType));
    descriptors.push_back(descriptorOptionEventType.get());

    // Uses the helper to declare common parameters.
    commandLineToolHelper.declareSubsongParameter(descriptors);
    commandLineToolHelper.declareInputSongParameter(descriptors);
    commandLineToolHelper.declareExportAsParameter(descriptors, false);

    auto descriptorParameterOutput = CommandLineArgumentDescriptor::buildArgumentWithDirectValue(
            juce::translate("<path to output file>"), juce::translate("Path and filename to the Events file to create."),
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
    const ExportConfiguration exportConfiguration(*sourceConfiguration, { subsongId.getValue() }, baseLabel, orgAddress);

    // What types of events?
    std::set<EventsExporter::Type> typesToExport;
    if (parameterEventType->isPresent()) {
        const auto eventType = parameterEventType->getValueAsString();
        if (eventType == eventTypeAll) {
            typesToExport.emplace(EventsExporter::Type::samples);
            typesToExport.emplace(EventsExporter::Type::events);
        } else if (eventType == eventTypeEvent) {
            typesToExport.emplace(EventsExporter::Type::events);
        } else if (eventType == eventTypeSample) {
            typesToExport.emplace(EventsExporter::Type::samples);
        } else {
            CommandLineToolHelper::cerr(juce::translate("Event type is unknown: " + eventType + "."));
            return -1;
        }
    } else {
        typesToExport.emplace(EventsExporter::Type::samples);
        typesToExport.emplace(EventsExporter::Type::events);
    }

    // Makes the export.
    EventsExporter exporter(song, typesToExport, exportConfiguration);
    const auto exportResult = exporter.performTask();
    if (!exportResult.first || (exportResult.second == nullptr) || !exportResult.second->isOk()) {
        CommandLineToolHelper::cerr(juce::translate("Export to events failed!"));
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

    CommandLineToolHelper::cout(juce::translate("Export to events successful."));
    return 0;
}

}   // namespace arkostracker
