#include "FapExporter.h"

#include <utility>

#include "../../../thirdParty/fap/FapCrunch.h"
#include "../../utils/FileExtensions.h"
#include "../../utils/FileUtil.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/ZipHelper.h"
#include "../sourceGenerator/SourceGenerator.h"
#include "../ym/YmExporter.h"
#include "../ym/YmGenerator.h"

namespace arkostracker
{

const juce::String FapExporter::constantSourceFileSuffix = "_fap_constants";        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

FapExporter::FapExporter(const std::shared_ptr<Song>& pSong, Id pSubsongId, SourceGeneratorConfiguration pSourceConfiguration, juce::String pBaseLabel,
    const OptionalInt pTargetPsgFrequency) noexcept :
        song(pSong),
        inputYm(),
        forcedTargetPsgFrequency(pTargetPsgFrequency),
        subsongId(std::move(pSubsongId)),
        sourceConfiguration(std::move(pSourceConfiguration)),
        baseLabel(std::move(pBaseLabel)),
        ymExporter(),
        outputStream()
{
}

FapExporter::FapExporter(juce::MemoryBlock pYm, SourceGeneratorConfiguration pSourceConfiguration, juce::String pBaseLabel, const OptionalInt pTargetPsgFrequency) noexcept :
        song(nullptr),
        inputYm(std::move(pYm)),
        forcedTargetPsgFrequency(pTargetPsgFrequency),
        subsongId(),
        sourceConfiguration(std::move(pSourceConfiguration)),
        baseLabel(std::move(pBaseLabel)),
        ymExporter(),
        outputStream()
{
}

std::pair<bool, std::unique_ptr<FapResult>> FapExporter::performTask() noexcept
{
    // If the YM (possibly zipped) is given, use it.
    if (!inputYm.isEmpty()) {
        const auto unzippedYm = ZipHelper::unzip(inputYm);
        return performTask(unzippedYm);
    }

    // A song is given. First, converts to YM. Must be interleaved, slower, but FAP requires that.
    jassert(song != nullptr);
    ymExporter = std::make_unique<YmExporter>(song, subsongId, 0, true, false);
    ymExporter->setProgressListener(this);  // Needed because inner task.
    const auto [ymSuccess, ymMemoryOutputStream] = ymExporter->performTask();

    if (!ymSuccess) {
        jassertfalse;
        return { false, nullptr };
    }

    return performTask(ymMemoryOutputStream->getMemoryBlock());
}

std::pair<bool, std::unique_ptr<FapResult>> FapExporter::performTask(const juce::MemoryBlock& originalYmMemoryBlock) const noexcept
{
    // Converts the original YM into a valid one, possibly changing its frequency.
    juce::MemoryOutputStream generatedYmOutputStream;
    YmGenerator ymGenerator(originalYmMemoryBlock, generatedYmOutputStream, forcedTargetPsgFrequency);
    if (!ymGenerator.convert()) {
        jassertfalse;
        return { false, nullptr };
    }
    const auto ymMemoryBlock = generatedYmOutputStream.getMemoryBlock();

    // Creates a temp file with the content.
    const auto inputFile = juce::File::createTempFile(FileExtensions::ymExtensionWithoutDot);
    const auto& inputFileFullPath = inputFile.getFullPathName();
    auto success = FileUtil::saveMemoryBlockToFile(inputFileFullPath, ymMemoryBlock);
    if (!success) {
        jassertfalse;
        return { false, nullptr };
    }

    const auto tempFolder = juce::File::getSpecialLocation(juce::File::tempDirectory);
    const auto outputFile = juce::File(tempFolder.getFullPathName() + juce::File::getSeparatorChar() + "fapTempAt3.fap");

    const std::vector arguments = {
        inputFile.getFullPathName().toStdString(),
        outputFile.getFullPathName().toStdString(),
    };

    // Then, converts to FAP.
    // With thanks from https://stackoverflow.com/a/39883532.
    std::vector<char*> argv;
    argv.push_back(nullptr);        // A dummy value for the program name.
    for (const auto& arg : arguments) {
        argv.push_back(const_cast<char*>(arg.data()));      // NOLINT(*-pro-type-const-cast)
    }
    argv.push_back(nullptr);        // End of the list.

    const auto fapResult = fapCrunch(static_cast<int>(argv.size() - 1), argv.data());
    success = (fapResult == 0);
    jassert(success);

    const auto inputFileStream = std::make_unique<juce::FileInputStream>(outputFile);
    success = success && inputFileStream->openedOk();
    jassert(success);

    auto generatedOutputMemoryBlock = MemoryBlockUtil::fromInputStream(*inputFileStream);
    const auto resultBufferSize = getBufferSize();
    const auto resultPlayTimeInNops = getPlayTimeInNops();
    const auto resultRegisterCountToPlay = getRegisterCountToPlay();
    const auto r12Constant = isR12Constant();

    const auto generatedConstantSourceMemoryBlock = buildConstantSourceMemoryBlock(resultBufferSize, resultPlayTimeInNops, resultRegisterCountToPlay, r12Constant);

    // Clean.
    (void)inputFile.deleteFile();
    (void)outputFile.deleteFile();

    auto result = std::make_unique<FapResult>(
        generatedOutputMemoryBlock, *generatedConstantSourceMemoryBlock,     // Duplicates the buffer. Who cares.
        resultBufferSize, resultPlayTimeInNops, resultRegisterCountToPlay, r12Constant
    );

    return { success, std::move(result) };
}

void FapExporter::onAskedToCancelTask() noexcept
{
    if (ymExporter != nullptr) {
        ymExporter->askToCancelTask();
    }
}

std::unique_ptr<juce::MemoryBlock> FapExporter::buildConstantSourceMemoryBlock(const int bufferSize, const int playTimeInNops,
    const int registerCountToPlay, const bool isR12Constant) const noexcept
{
    const auto sourceOutputStream = std::make_unique<juce::MemoryOutputStream>();
    SourceGenerator sourceGenerator(sourceConfiguration, *sourceOutputStream);

    sourceGenerator.declareComment("This file contains the constants of the FAP export.");
    sourceGenerator.declareComment("It has been generated by Arkos Tracker 3.");
    sourceGenerator.addEmptyLine();

    sourceGenerator.declareConstant(baseLabel + "FapBufferSize", juce::String(bufferSize));
    sourceGenerator.declareConstant(baseLabel + "FapPlayTimeInNops", juce::String(playTimeInNops));
    sourceGenerator.declareConstant(baseLabel + "FapRegisterCountToPlay", juce::String(registerCountToPlay));
    sourceGenerator.declareConstant(baseLabel + "FapIsR12Constant", juce::String(NumberUtil::boolToInt(isR12Constant)));

    return std::make_unique<juce::MemoryBlock>(sourceOutputStream->getMemoryBlock());
}

}   // namespace arkostracker
