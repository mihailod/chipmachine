#include "QuickAkgExport.h"

#include <BinaryData.h>

#include <utility>

#include "../../song/Song.h"
#include "../../ui/export/common/task/CompileSource.h"
#include "../../ui/mainView/operation/loadSong/LoadSong.h"
#include "../../utils/FileUtil.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/RasmUtil.h"
#include "../akg/AkgExporter.h"
#include "../sfx/SfxExporter.h"

namespace arkostracker
{

QuickAkgPlayerExport::QuickAkgPlayerExport(const int pAddress, const bool pWithSfx) noexcept :
        address(pAddress),
        withSfx(pWithSfx)
{
}

std::pair<bool, std::unique_ptr<juce::MemoryBlock>> QuickAkgPlayerExport::performTask() noexcept
{
    // Adds the ORG and the SFX flag, if needed.
    juce::MemoryBlock sourceMemoryBlock;
    if (withSfx) {
        RasmUtil::addVariableDeclaration(sourceMemoryBlock, "PLY_AKG_MANAGE_SOUND_EFFECTS");
    }
    MemoryBlockUtil::appendString(sourceMemoryBlock, "    ORG " + juce::String(address) + "\n");

    // Gets the player BASIC interruption wrapper.
    const auto interruptionWrapperSourceMemoryBlock = juce::MemoryBlock(BinaryData::AkgWrapperBasicInterruptionsCPC_asm, BinaryData::AkgWrapperBasicInterruptionsCPC_asmSize);
    sourceMemoryBlock.append(interruptionWrapperSourceMemoryBlock.getData(), interruptionWrapperSourceMemoryBlock.getSize());

    // Gets the player.
    const auto playerSourceMemoryBlock = RasmUtil::commentIncludes(juce::MemoryBlock(BinaryData::PlayerAkg_asm, BinaryData::PlayerAkg_asmSize));
    sourceMemoryBlock.append(playerSourceMemoryBlock.getData(), playerSourceMemoryBlock.getSize());

    // Adds the possible SFX player.
    if (withSfx) {
        const auto sfxPlayerSourceMemoryBlock = juce::MemoryBlock(BinaryData::PlayerAkg_SoundEffects_asm, BinaryData::PlayerAkg_SoundEffects_asmSize);
        sourceMemoryBlock.append(sfxPlayerSourceMemoryBlock.getData(), sfxPlayerSourceMemoryBlock.getSize());
    }

    // Compiles the player.
    auto binaryMemoryBlock = CompileSource::compile(sourceMemoryBlock);
    const auto success = (binaryMemoryBlock != nullptr);
    jassert(success);

    return { success, std::move(binaryMemoryBlock) };
}


// =================================================================================

QuickSfxExport::QuickSfxExport(const Song& pSong, const int pAddress) noexcept :
        song(pSong),
        address(pAddress)
{
}

std::pair<bool, std::unique_ptr<juce::MemoryBlock>> QuickSfxExport::performTask() noexcept
{
    const ExportConfiguration exportConfiguration(
        SourceGeneratorConfiguration::buildZ80(),
        { song.getFirstSubsongId() },
        "mySfx",
        address
    );

    // Exports the SFX.
    SfxExporter exporterTask(song, exportConfiguration);
    const auto [success, result] = exporterTask.performTask();
    if (success && (result != nullptr)) {
        const auto sourceMemoryBlock = result->getAggregatedData();

        // Compiles the source.
        auto binaryMemoryBlock = CompileSource::compile(sourceMemoryBlock);
        if (binaryMemoryBlock != nullptr) {
            return { true, std::move(binaryMemoryBlock) };
        }
    }

    jassertfalse;
    return { false, nullptr };
}



// =================================================================================

QuickLoadAndCompileSfxExport::QuickLoadAndCompileSfxExport(const juce::File& pFile, const int pAddress) noexcept :
        file(pFile),
        address(pAddress)
{
}

std::pair<bool, std::unique_ptr<juce::MemoryBlock>> QuickLoadAndCompileSfxExport::performTask() noexcept
{
    // Loads the Song. It should be a simple AT3 file.
    SongLoader songLoader;
    const auto loadResult = songLoader.loadSong(file, true);
    if ((loadResult->status == SongLoader::ImportStatus::ok) && (loadResult->song != nullptr)) {
        const auto& song = *loadResult->song;

        // Exports the SFXs.
        const ExportConfiguration exportConfiguration(
            SourceGeneratorConfiguration::buildZ80(), { song.getFirstSubsongId() }, "sfxLabel", address
        );
        SfxExporter sfxExporter(song, exportConfiguration);
        const auto& [sfxSuccess, sfxResult] = sfxExporter.performTask();
        if (sfxSuccess) {
            const auto sfxSource = sfxResult->getAggregatedData();

            // Compiles the SFX source.
            auto sfxBinary = CompileSource::compile(sfxSource);
            const auto sfxCompilationSuccess = (sfxBinary != nullptr);
            jassert(sfxCompilationSuccess);
            return { sfxCompilationSuccess, std::move(sfxBinary) };
        }
    }

    jassertfalse;
    return { false, nullptr };
}


// =================================================================================

QuickExport::QuickExport(juce::File pOutputDskFile, const int pPlayerAddress, const int pMusicAddress,
    std::shared_ptr<const Song> pSong, const OptionalInt pSfxAddress, juce::File pSfxFile) noexcept :
        outputDskFile(std::move(pOutputDskFile)),
        playerAddress(pPlayerAddress),
        musicAddress(pMusicAddress),
        song(std::move(pSong)),
        sfxAddress(pSfxAddress),
        sfxFile(std::move(pSfxFile))
{
}

std::pair<bool, std::unique_ptr<bool>> QuickExport::performTask() noexcept
{
    const auto isSfx = sfxAddress.isPresent();

    // Compiles the player.
    QuickAkgPlayerExport quickAkgPlayerExport(playerAddress, isSfx);
    const auto [playerSuccess, playerMemoryBlock] = quickAkgPlayerExport.performTask();
    if (!playerSuccess) {
        jassertfalse;
        return { false, nullptr };
    }
    const auto tempFilePlayer = juce::File::createTempFile("bin");
    auto success = FileUtil::saveMemoryBlockToFile(tempFilePlayer, *playerMemoryBlock);

    // Compiles the music.
    const auto exportConfiguration = ExportConfiguration(
        SourceGeneratorConfiguration::buildZ80(), song->getSubsongIds(), "mySong", musicAddress
    );
    AkgExporter akgExporter(*song, exportConfiguration);
    const auto [successSong, musicResult] = akgExporter.performTask();
    if (!success || !successSong || (musicResult == nullptr)) {
        jassertfalse;
        return { false, nullptr };
    }
    const auto musicSourceMemoryBlock = musicResult->getAggregatedData();
    const auto musicBinaryMemoryBlock = CompileSource::compile(musicSourceMemoryBlock);
    if (musicBinaryMemoryBlock == nullptr) {
        jassertfalse;
        return { false, nullptr };
    }
    const auto tempFileMusic = juce::File::createTempFile("bin");
    success = FileUtil::saveMemoryBlockToFile(tempFileMusic, *musicBinaryMemoryBlock);

    // Compiles the SFX, if any.
    std::unique_ptr<juce::File> tempFileSfx;
    if (success && isSfx) {
        QuickLoadAndCompileSfxExport quickLoadAndCompileSfxExport(sfxFile, sfxAddress.getValue());
        auto [sfxSuccess, sfxMemoryBlock] = quickLoadAndCompileSfxExport.performTask();
        if (!sfxSuccess) {
            jassertfalse;
            return { false, nullptr };
        }
        // Saves the file.
        tempFileSfx = std::make_unique<juce::File>(juce::File::createTempFile("bin"));
        success = FileUtil::saveMemoryBlockToFile(*tempFileSfx, *sfxMemoryBlock);
    }

    // Adds the basic loader.
    std::unique_ptr<juce::File> tempBasicFile;
    if (success) {
        tempBasicFile = std::make_unique<juce::File>(juce::File::createTempFile("bas"));

        // Extracts the DSK where the loaders are and stores it as a real file.
        const auto basicLoaderMemoryBlock = juce::MemoryBlock(BinaryData::BasicCpcInterruptionsTemplate_dsk, BinaryData::BasicCpcInterruptionsTemplate_dskSize);
        const auto tempDskFile = juce::File::createTempFile("dsk");
        success = FileUtil::saveMemoryBlockToFile(tempDskFile, basicLoaderMemoryBlock);
        jassert(success);

        const auto basicFileInDsk = isSfx ? juce::String("WITHSFX.BAS") : "NOSFX.BAS";

        std::vector<juce::String> lines;
        lines.emplace_back(" EDSK GETFILE, '" + tempDskFile.getFullPathName() + "', '" + basicFileInDsk + "', '" + tempBasicFile->getFullPathName() + "'");
        CompileSource::compile(MemoryBlockUtil::fromStrings(lines));

        // Gets the loader in the memory block.
        auto basicLoader = MemoryBlockUtil::fromFile(*tempBasicFile);
        success = (basicLoader != nullptr);

        // Modifies the addresses.
        if (success) {
            // There are the original values in the Basic programs, to be replaced with our newly generated ones.
            constexpr auto musicAddressInBasic = 0x7000;
            constexpr auto sfxAddressInBasic = 0x9000;
            constexpr auto playerAddressInBasic = 0x9500;
            constexpr auto memoryAddressInBasic = 0x6fff;
            const auto memoryAddress = musicAddress - 1;
            constexpr auto variableMnemonic = 0x1c;
            constexpr auto basicHeaderLength = 0x80;        // Makes sure to skip the header, just in case of junk code.

            success = success && MemoryBlockUtil::replaceBytes(*basicLoader, basicHeaderLength,
                { variableMnemonic, NumberUtil::getByte(memoryAddressInBasic, 0), NumberUtil::getByte(memoryAddressInBasic, 1) },
                { variableMnemonic, NumberUtil::getByte(memoryAddress, 0), NumberUtil::getByte(memoryAddress, 1) }
            );
            jassert(success);
            success = success && MemoryBlockUtil::replaceBytes(*basicLoader, basicHeaderLength,
                { variableMnemonic, NumberUtil::getByte(musicAddressInBasic, 0), NumberUtil::getByte(musicAddressInBasic, 1) },
                { variableMnemonic, NumberUtil::getByte(musicAddress, 0), NumberUtil::getByte(musicAddress, 1) }
            );
            jassert(success);
            success = success && MemoryBlockUtil::replaceBytes(*basicLoader, basicHeaderLength,
                { variableMnemonic, NumberUtil::getByte(playerAddressInBasic, 0), NumberUtil::getByte(playerAddressInBasic, 1) },
                { variableMnemonic, NumberUtil::getByte(playerAddress, 0), NumberUtil::getByte(playerAddress, 1) }
            );
            if (isSfx) {
                success = success && MemoryBlockUtil::replaceBytes(*basicLoader, basicHeaderLength,
                  { variableMnemonic, NumberUtil::getByte(sfxAddressInBasic, 0), NumberUtil::getByte(sfxAddressInBasic, 1) },
                  { variableMnemonic, NumberUtil::getByte(sfxAddress.getValue(), 0), NumberUtil::getByte(sfxAddress.getValue(), 1) }
                );
                jassert(success);
            }
            jassert(success);
        }

        // Saves the file back.
        if (success) {
            success = FileUtil::saveMemoryBlockToFile(*tempBasicFile, *basicLoader);
        }

        (void)tempDskFile.deleteFile();

        jassert(success);
    }

    // Generates the DSK.
    if (success) {
        const auto outputDskFileString = outputDskFile.getFullPathName();

        std::vector<juce::String> lines;
        lines.emplace_back(" EDSK CREATE, '" + outputDskFileString + "', data, 41, interlaced, overwrite");
        lines.push_back(" EDSK PUTFILE,'" + outputDskFileString + "', '" + tempFilePlayer.getFullPathName() + "', 'player.bin', AMSDOS, BINARY, LOAD=" + juce::String(playerAddress));
        lines.push_back(" EDSK PUTFILE,'" + outputDskFileString + "', '" + tempFileMusic.getFullPathName() + "', 'music.bin', AMSDOS, BINARY, LOAD=" + juce::String(musicAddress));
        if (tempFileSfx != nullptr) {
            lines.push_back(" EDSK PUTFILE,'" + outputDskFileString + "', '" + tempFileSfx->getFullPathName() + "', 'sfx.bin', AMSDOS, BINARY, LOAD=" + juce::String(sfxAddress.getValue()));
        }
        lines.push_back(" EDSK PUTFILE,'" + outputDskFileString + "', '" + tempBasicFile->getFullPathName() + "', 'play.bas'");

        CompileSource::compile(MemoryBlockUtil::fromStrings(lines));
        success = outputDskFile.existsAsFile();
    }

    // Deletes the temp files.
    (void)tempFilePlayer.deleteFile();
    (void)tempFileMusic.deleteFile();
    if (tempFileSfx != nullptr) {
        (void)tempFileSfx->deleteFile();
    }
    if (tempBasicFile != nullptr) {
        (void)tempBasicFile->deleteFile();
    }

    return { success, std::make_unique<bool>(success) };
}

}       // namespace arkostracker