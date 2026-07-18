#include "InstrumentAsWavExporter.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <utility>

#include "../../song/psg/Psg.h"
#include "../../utils/FileExtensions.h"
#include "../../utils/NoteUtil.h"
#include "InstrumentRenderer.h"

namespace arkostracker
{
InstrumentAsWavExporter::InstrumentAsWavExporter(const std::shared_ptr<Song>& pSong, Id pInstrumentId,
    juce::File pOutputFolder, juce::String pBaseOutputFileNameWithoutExtension,
    const int pFirstNote, const int pLastNote, const Psg& pPsg, const float pReplayFrequencyHz, const SidPlayerCapability& pSidPlayerCapability,
    const int pMinimumDurationMs, const OptionalInt pMaximumSize) noexcept :
        song(pSong),
        psg(pPsg),
        sidPlayerCapability(pSidPlayerCapability),
        replayFrequencyHz(pReplayFrequencyHz),
        minimumDurationMs(pMinimumDurationMs),
        maximumSize(pMaximumSize),
        instrumentIdToExport(std::move(pInstrumentId)),
        outputFolder(std::move(pOutputFolder)),
        baseOutputFileNameWithoutExtension(std::move(pBaseOutputFileNameWithoutExtension)),
        firstNote(pFirstNote),
        lastNote(pLastNote),
        emptyInstrumentId(song->getInstrumentId(0).getValue())
{
    jassert(lastNote >= firstNote);
}

std::pair<bool, std::unique_ptr<std::vector<juce::File>>> InstrumentAsWavExporter::performTask() noexcept
{
    auto success = true;
    auto outputFiles = std::make_unique<std::vector<juce::File>>();

    const auto noteCount = lastNote - firstNote + 1;

    // Generates one file per note.
    for (auto note = firstNote; !isCanceled() && success && (note <= lastNote); ++note) {
        publishTaskProgress(note - firstNote, noteCount);

        const auto renderedFile = renderNote(note);
        success = renderedFile.isPresent();

        if (success) {
            outputFiles->emplace_back(renderedFile.getValueRef());
        }
    }

    if (isCanceled()) {
        success = false;
    }

    return { success, std::move(outputFiles) };
}

OptionalValue<juce::File> InstrumentAsWavExporter::renderNote(const int note) const noexcept
{
    const auto outputFile = outputFolder.getChildFile(
        baseOutputFileNameWithoutExtension + NoteUtil::getStringFromNote(note)
        + "." + FileExtensions::wavExtensionWithoutDot);

    if (!outputFile.deleteFile()) {
        jassertfalse;
        return { };
    }

    // Creates the output stream. It is RELEASED to give its ownership to the AudioFormatWriter.
    auto fosUnique = std::make_unique<juce::FileOutputStream>(outputFile);
    auto* fos = fosUnique.release();

    juce::MemoryOutputStream outputStream;

    InstrumentRenderer instrumentRenderer(song, psg, sidPlayerCapability);
    const auto outputSampleRate = instrumentRenderer.renderNote(note, instrumentIdToExport, outputStream, replayFrequencyHz, psgSampleRate, minimumDurationMs, maximumSize);

    // Creates the WAV writer.
    juce::WavAudioFormat wavAudioFormat;
    const juce::StringPairArray metadataValues;     // Nothing to put inside.
    const std::unique_ptr<juce::AudioFormatWriter> audioFormatWriter(wavAudioFormat.createWriterFor(fos,
        outputSampleRate, wavOutputChannelCount,
        bitsPerSample, metadataValues, 0));    // No optimization.

    // Stops if the Writer couldn't be created. The FOS must be deleted here because AudioFormatWriter won't do it in case of failure.
    if (audioFormatWriter == nullptr) {
        jassertfalse;
        delete fos;
        return { };
    }

    // Writes into the writer.
    const auto sampleSize = outputStream.getDataSize() / 4;     // Because 32 bits.
    const auto* channelData = static_cast<const float*>(outputStream.getData());
    const float* const channels[] = { channelData, nullptr };
    audioFormatWriter->writeFromFloatArrays(channels, 1, static_cast<int>(sampleSize));

    audioFormatWriter->flush();

    return outputFile;
}

}   // namespace arkostracker
