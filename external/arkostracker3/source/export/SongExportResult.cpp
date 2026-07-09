#include "SongExportResult.h"

namespace arkostracker 
{

SongExportResult::SongExportResult() noexcept :
        songOutputStream(),
        subsongOutputStreams(),
        playerConfiguration(),
        errorReport()
{
}

SongExportResult::SongExportResult(std::unique_ptr<juce::MemoryOutputStream> pSongOutputStream, PlayerConfiguration pPlayerConfiguration) noexcept :
        songOutputStream(std::move(pSongOutputStream)),
        subsongOutputStreams(),
        playerConfiguration(std::move(pPlayerConfiguration)),
        errorReport()
{
}

void SongExportResult::setSongStream(std::unique_ptr<juce::MemoryOutputStream> outputStream) noexcept
{
    songOutputStream = std::move(outputStream);
}

void SongExportResult::setPlayerConfiguration(const PlayerConfiguration& pPlayerConfiguration) noexcept
{
    playerConfiguration = pPlayerConfiguration;
}

void SongExportResult::addFlag(PlayerConfigurationFlag flag) noexcept
{
    playerConfiguration.addFlag(flag);
}

void SongExportResult::addFlag(bool addFlag, PlayerConfigurationFlag flag) noexcept
{
    playerConfiguration.addFlag(addFlag, flag);
}

void SongExportResult::addSubsongStream(std::unique_ptr<juce::MemoryOutputStream> outputStream) noexcept
{
    subsongOutputStreams.push_back(std::move(outputStream));
}

juce::MemoryBlock SongExportResult::getSongData() const noexcept
{
    return songOutputStream->getMemoryBlock();
}

std::vector<juce::MemoryBlock> SongExportResult::getSubsongData() const noexcept
{
    std::vector<juce::MemoryBlock> outputBlocks;
    if (!subsongOutputStreams.empty()) {
        outputBlocks.reserve(subsongOutputStreams.size());

        for (const auto& streams : subsongOutputStreams) {
            outputBlocks.push_back(streams->getMemoryBlock());
        }
    }

    return outputBlocks;
}

juce::MemoryBlock SongExportResult::getAggregatedData() const noexcept
{
    const auto subsongsMemoryBlock = getSubsongData();
    auto data = getSongData();

    for (const auto& subsongMemoryBlock : subsongsMemoryBlock) {
        data.append(subsongMemoryBlock.getData(), subsongMemoryBlock.getSize());
    }

    return data;
}

void SongExportResult::addWarning(const juce::String& text, OptionalInt lineNumber) noexcept
{
    errorReport.addWarning(text, lineNumber);
}

void SongExportResult::addError(const juce::String& text, OptionalInt lineNumber) noexcept
{
    errorReport.addError(text, lineNumber);
}

bool SongExportResult::isOk() const noexcept
{
    return errorReport.isOk();
}

const ErrorReport& SongExportResult::getErrorReportRef() noexcept
{
    return errorReport;
}

const PlayerConfiguration& SongExportResult::getPlayerConfigurationRef() noexcept
{
    return playerConfiguration;
}


}   // namespace arkostracker

