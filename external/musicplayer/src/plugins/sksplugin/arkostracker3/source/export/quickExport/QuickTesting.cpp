#include "QuickTesting.h"

#include <BinaryData.h>

#include <utility>

#include "../../app/preferences/PreferencesManager.h"
#include "../../controllers/MainController.h"
#include "../../ui/export/common/task/CompileSource.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/RasmUtil.h"
#include "../akg/AkgExporter.h"
#include "../aky/AkyExporter.h"
#include "../playerConfiguration/PlayerConfigurationExporter.h"
#include "../raw/RawExporter.h"
#include "Behavior.h"

namespace arkostracker
{

QuickTesting::QuickTesting(std::shared_ptr<Song> pSong, const bool pToDsk, const PlayerType pPlayerType, juce::File pOutputFile) noexcept :
        song(std::move(pSong)),
        toDsk(pToDsk),
        playerType(pPlayerType),
        outputFile(std::move(pOutputFile))
{
}

// Task method implementations.
// ===================================================

std::pair<bool, std::unique_ptr<bool>> QuickTesting::performTask() noexcept
{
    const auto sourceGeneratorConfiguration = SourceGeneratorConfiguration::buildZ80();

    const auto subsongIds = song->getFirstSubsongId();      // The song should have been stripped.

    // Only AKY+digidrums/MOD exports samples.
    SampleEncoderFlags sampleEncoderFlags;
    if (playerType == PlayerType::modCpcOld) {
        sampleEncoderFlags = SampleEncoderFlags(
            true, 8, 128, 450
        );
    } else {
        sampleEncoderFlags = PreferencesManager::getInstance().getSampleEncoderFlags()
            .withAreSampleExported(playerType == PlayerType::akyMultiPsgDigidrums)
            .withGenerateIndexTable(true);
    }

    const ExportConfiguration exportConfiguration(sourceGeneratorConfiguration, { subsongIds }, "MyLabel", {}, sampleEncoderFlags );

    // Gets the player and the exporter.
    std::unique_ptr<Task<std::unique_ptr<SongExportResult>>> exporter;
    const char* playerData;     // NOLINT(*-init-variables)
    size_t playerSize;          // NOLINT(*-init-variables)
    const char* testerData;     // NOLINT(*-init-variables)
    size_t testerSize;          // NOLINT(*-init-variables)
    switch (playerType) {
        case PlayerType::akg:
            playerData = BinaryData::PlayerAkg_asm;
            playerSize = BinaryData::PlayerAkg_asmSize;
            testerData = BinaryData::AkgRawTesterCPC_asm;
            testerSize = BinaryData::AkgRawTesterCPC_asmSize;
            exporter = std::make_unique<AkgExporter>(*song, exportConfiguration);
            break;
        case PlayerType::aky:
            playerData = BinaryData::PlayerAky_asm;
            playerSize = BinaryData::PlayerAky_asmSize;
            testerData = BinaryData::AkyRawTesterCPC_asm;
            testerSize = BinaryData::AkyRawTesterCPC_asmSize;
            exporter = std::make_unique<AkyExporter>(song, exportConfiguration);
            break;
        case PlayerType::akyMultiPsgDigidrums:
            playerData = BinaryData::PlayerAkyMultiPsg_asm;
            playerSize = BinaryData::PlayerAkyMultiPsg_asmSize;
            testerData = BinaryData::AkyDigidrumsRawTesterCPC_asm;
            testerSize = BinaryData::AkyDigidrumsRawTesterCPC_asmSize;
            exporter = std::make_unique<AkyExporter>(song, exportConfiguration);
            break;
        case PlayerType::akyMultiPsg9Channels:
            playerData = BinaryData::PlayerAkyMultiPsg_asm;
            playerSize = BinaryData::PlayerAkyMultiPsg_asmSize;
            testerData = BinaryData::AkyPlayCity9ChannelsRawTesterCPC_asm;
            testerSize = BinaryData::AkyPlayCity9ChannelsRawTesterCPC_asmSize;
            exporter = std::make_unique<AkyExporter>(song, exportConfiguration);
            break;
        case PlayerType::akySid:
            playerData = BinaryData::PlayerAkySid_CPC_asm;
            playerSize = BinaryData::PlayerAkySid_CPC_asmSize;
            testerData = BinaryData::AkySidRawTesterCPC_asm;
            testerSize = BinaryData::AkySidRawTesterCPC_asmSize;
            exporter = std::make_unique<AkyExporter>(song, exportConfiguration);
            break;
        case PlayerType::modCpcOld: {
            playerData = BinaryData::PlayerMod_CPC_asm;
            playerSize = BinaryData::PlayerMod_CPC_asmSize;
            testerData = BinaryData::ModRawTesterCPC_asm;
            testerSize = BinaryData::ModRawTesterCPC_asmSize;
            const EncodedDataFlag encodedDataFlag(
                true, true, true, false, true, true,
                false, true, false, false,
                true, 1.25
            );
            exporter = std::make_unique<RawExporter>(*song, encodedDataFlag, exportConfiguration);
            break;
        }
        default:
            jassertfalse;       // Not managed yet!
            return { false, std::make_unique<bool>(false) };
    }

    const auto [successExporter, songData] = exporter->performTask();
    if (!successExporter) {
        return { false, std::make_unique<bool>(false) };
    }

    const auto startAddressString = juce::String(0x100);
    const auto& dskFilePath = outputFile.getFullPathName();

    // What behavior?
    const auto behavior = Behavior::buildBehavior(song, exportConfiguration, playerType, toDsk, dskFilePath, startAddressString);

    const auto musicSourceMemoryBlock = songData->getAggregatedData();
    const auto playerConfiguration = songData->getPlayerConfigurationRef();
    const auto playerConfigurationMemoryBlock = PlayerConfigurationExporter::exportConfiguration(sourceGeneratorConfiguration, playerConfiguration);

    // Gets the tester.
    juce::MemoryInputStream testerSourceInputStream(testerData, testerSize, false);
    const auto testerSourceMemoryBlock = MemoryBlockUtil::fromInputStream(testerSourceInputStream);

    juce::MemoryInputStream playerSourceInputStream(playerData, playerSize, false);
    const auto playerSourceMemoryBlock = RasmUtil::commentIncludes(MemoryBlockUtil::fromInputStream(playerSourceInputStream));

    // Concatenates all.
    const auto initialLines = behavior->buildInitialLines();

    // Starts with initial lines to set up the DSK/SNA.
    auto finalMemoryBlock = MemoryBlockUtil::fromStrings(initialLines);
    // Then the tester.
    finalMemoryBlock.append(testerSourceMemoryBlock.getData(), testerSourceMemoryBlock.getSize());
    // Then the player config and the player.
    finalMemoryBlock.append(playerConfigurationMemoryBlock.getData(), playerConfigurationMemoryBlock.getSize());
    finalMemoryBlock.append(playerSourceMemoryBlock.getData(), playerSourceMemoryBlock.getSize());
    // Then the music.
    MemoryBlockUtil::appendString(finalMemoryBlock, "MusicStart");
    finalMemoryBlock.append(musicSourceMemoryBlock.getData(), musicSourceMemoryBlock.getSize());
    // More specific behavior?
    const auto additionalMemoryBlock = behavior->buildAdditionalMemoryBlock();
    if (additionalMemoryBlock == nullptr) {
        jassertfalse;       // If nullptr, error!
        return { false, std::make_unique<bool>(false) };
    }
    if (additionalMemoryBlock->getSize() > 0) {
        finalMemoryBlock.append(additionalMemoryBlock->getData(), additionalMemoryBlock->getSize());
    }
    // Ends with lines to set up the DSK/SNA.
    const auto closingLines = behavior->buildClosingLines();
    auto closingMemoryBlock = MemoryBlockUtil::fromStrings(closingLines);
    finalMemoryBlock.append(closingMemoryBlock.getData(), closingMemoryBlock.getSize());

    //FileUtil::saveMemoryBlockToFile("~/Documents/dev/cpc/tests/test.asm", finalMemoryBlock);

    // Export to DSK via Rasm.
    const auto result = CompileSource::compile(finalMemoryBlock);

    // Checks the result.
    auto success = outputFile.existsAsFile();
    success = success && (toDsk ? (result != nullptr) : (result == nullptr));   // With SNA, nothing is generated.

    return { success, std::make_unique<bool>(success) };
}

}   // namespace arkostracker
