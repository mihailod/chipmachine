#include "Behavior.h"

#include <utility>

#include "../events/EventsExporter.h"
#include "../samples/SampleExporter.h"
#include "../../utils/MemoryBlockUtil.h"
#include "BinaryData.h"

namespace arkostracker
{

Behavior::Behavior(const std::shared_ptr<const Song>& pSong, ExportConfiguration pExportConfiguration, const PlayerType pPayerType, const bool pToDsk,
    juce::String pDskFilePath, juce::String pStartAddressString, std::unique_ptr<InnerBehavior> pInnerBehavior) :
        song(pSong),
        exportConfiguration(std::move(pExportConfiguration)),
        playerType(pPayerType),
        toDsk(pToDsk),
        dskFilePath(std::move(pDskFilePath)),
        startAddressString(std::move(pStartAddressString)),
        innerBehavior(std::move(pInnerBehavior))
{
}

std::unique_ptr<Behavior> Behavior::buildBehavior(const std::shared_ptr<const Song>& song, const ExportConfiguration& exportConfiguration, const PlayerType playerType,
    const bool toDsk, juce::String dskFilePath, juce::String startAddressString)
{
    std::unique_ptr<InnerBehavior> innerBehavior = nullptr;
    // For digidrums, Events and Samples must be exported.
    if (playerType == PlayerType::akyMultiPsgDigidrums) {
        innerBehavior = std::make_unique<InnerBehaviorAkyDigidrums>(song, exportConfiguration);
    }

    if (toDsk) {
        return std::make_unique<BehaviorDsk>(song, exportConfiguration, playerType, toDsk, std::move(dskFilePath), std::move(startAddressString), std::move(innerBehavior));
    }
    return std::make_unique<BehaviorSna>(song, exportConfiguration, playerType, toDsk, std::move(dskFilePath), std::move(startAddressString), std::move(innerBehavior));
}

std::unique_ptr<juce::MemoryBlock> Behavior::buildAdditionalMemoryBlock()
{
    if (innerBehavior == nullptr) {
        return std::make_unique<juce::MemoryBlock>();
    }
    return innerBehavior->buildAdditionalMemoryBlock();
}


// BehaviorDsk method implementations.
// =================================================

BehaviorDsk::BehaviorDsk(const std::shared_ptr<const Song>& pSong, const ExportConfiguration& pExportConfiguration, const PlayerType pPlayerType, const bool pToDsk,
    juce::String pDskFilePath, juce::String pStartAddressString, std::unique_ptr<InnerBehavior> pInnerBehavior) :
        Behavior(pSong, pExportConfiguration, pPlayerType, pToDsk, std::move(pDskFilePath), std::move(pStartAddressString), std::move(pInnerBehavior))
{
}

std::vector<juce::String> BehaviorDsk::buildInitialLines()
{
    std::vector<juce::String> lines;
    lines.emplace_back(" EDSK create, '" + dskFilePath + "', data, 41, interlaced, overwrite");
    lines.push_back(" org " + startAddressString);
    return lines;
}

std::vector<juce::String> BehaviorDsk::buildClosingLines()
{
    std::vector<juce::String> lines;
    lines.emplace_back("EndAddress");
    lines.push_back(" EDSK savefile, '" + dskFilePath + "', '" + "play.bin" + "', " + startAddressString + ", EndAddress - " + startAddressString);

    return lines;
}


// BehaviorSna method implementations.
// =================================================

BehaviorSna::BehaviorSna(const std::shared_ptr<const Song>& pSong, const ExportConfiguration& pExportConfiguration, const PlayerType pPlayerType, const bool pToDsk,
    juce::String pDskFilePath, juce::String pStartAddressString, std::unique_ptr<InnerBehavior> pInnerBehavior) :
        Behavior(pSong, pExportConfiguration, pPlayerType, pToDsk, std::move(pDskFilePath), std::move(pStartAddressString), std::move(pInnerBehavior))
{
}

std::vector<juce::String> BehaviorSna::buildInitialLines()
{
    std::vector<juce::String> lines;
    lines.emplace_back(" buildsna force, '" + dskFilePath + "'");
    lines.emplace_back(" bankset 0");
    lines.emplace_back(" org " + startAddressString);
    lines.emplace_back(" run $");
    return lines;
}

std::vector<juce::String> BehaviorSna::buildClosingLines()
{
    return { };
}


// InnerBehaviorAkyDigidrums method implementations.
// =================================================

Behavior::InnerBehaviorAkyDigidrums::InnerBehaviorAkyDigidrums(std::shared_ptr<const Song> pSong, ExportConfiguration pExportConfiguration) :
        song(std::move(pSong)),
        exportConfiguration(std::move(pExportConfiguration))
{
}

std::unique_ptr<juce::MemoryBlock> Behavior::InnerBehaviorAkyDigidrums::buildAdditionalMemoryBlock()
{
    auto memoryBlock = std::make_unique<juce::MemoryBlock>();

    const auto digiChannel = song->getDigiChannel(exportConfiguration.getFirstSubsongId());
    MemoryBlockUtil::appendString(*memoryBlock, "DIGICHANNEL_INDEX = " + juce::String(digiChannel));

    // Loads the digidrums player.
    juce::MemoryInputStream testerSourceInputStream(BinaryData::PlayerAkyMultiPsg_Digidrums_asm, BinaryData::PlayerAkyMultiPsg_Digidrums_asmSize, false);
    const auto digidrumPlayerSourceMemoryBlock = MemoryBlockUtil::fromInputStream(testerSourceInputStream);
    memoryBlock->append(digidrumPlayerSourceMemoryBlock.getData(), digidrumPlayerSourceMemoryBlock.getSize());

    // Builds the Events.
    constexpr auto typeToExport = EventsExporter::Type::samples;
    EventsExporter eventsExporter(song, { typeToExport }, exportConfiguration);
    const auto [eventsSuccess, eventsResult] = eventsExporter.performTask();
    if (!eventsSuccess) {
        jassertfalse;
        return nullptr;
    }
    const auto eventsData = eventsResult->getAggregatedData();
    MemoryBlockUtil::appendString(*memoryBlock, "MusicEvents");
    memoryBlock->append(eventsData.getData(), eventsData.getSize());

    // Builds the Samples.
    constexpr auto pitch = 0.0;
    SampleExporter sampleExporter(*song, exportConfiguration, pitch);
    const auto [samplesSuccess, samplesResult] = sampleExporter.performTask();
    if (!samplesSuccess) {
        jassertfalse;
        return nullptr;
    }
    const auto samplesData = samplesResult->getAggregatedData();
    MemoryBlockUtil::appendString(*memoryBlock, "MusicSamples");
    memoryBlock->append(samplesData.getData(), samplesData.getSize());

    return memoryBlock;
}

}   // namespace arkostracker
