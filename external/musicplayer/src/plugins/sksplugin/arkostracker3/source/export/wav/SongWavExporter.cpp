#include "SongWavExporter.h"

#include <utility>

#include "../../audio/sources/PsgStreamGenerator.h"
#include "../../audio/sources/PsgsProcessor.h"
#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../player/SongPlayer.h"
#include "../../utils/FileUtil.h"
#include "DcOffsetCalculator.h"

namespace arkostracker
{

const unsigned int SongWavExporter::wavChannelCount = 2U;
const int SongWavExporter::bitsPerSample = 16;
const int SongWavExporter::bufferSize = 1024;

SongWavExporter::SongWavExporter(std::shared_ptr<Song> pSong, Id pSubsongId, const double pExportFrequencyHz, const int pAdditionalLoopCount,
                                 const OutputMix& pOutputMix, juce::File pBaseOutputFileName, const bool pExportEachChannelIndividually) noexcept :
        song(std::move(pSong)),
        subsongId(std::move(pSubsongId)),
        exportFrequencyHz(pExportFrequencyHz),
        additionalLoopCount(pAdditionalLoopCount),
        outputMix(pOutputMix),
        baseOutputFileName(std::move(pBaseOutputFileName)),
        exportEachChannelIndividually(pExportEachChannelIndividually)
{
}


// Task method implementations.
// ===============================

std::pair<bool, std::unique_ptr<bool>> SongWavExporter::performTask() noexcept
{
    // Counts how many iterations are in the Song.
    const auto[baseIterationCounter, upTo, loopToCounter] = FrameCounter::count(*song, subsongId);

    const auto metadata = song->getSubsongMetadata(subsongId);
    const auto psgs = song->getSubsongPsgs(subsongId);
    const auto replayFrequency = metadata.getReplayFrequencyHz();
    const auto sidPlayerCapability = metadata.getSidPlayerCapability();

    const auto channelPassCount = exportEachChannelIndividually ? song->getChannelCount(subsongId) : 1;

    // Adds more iterations if additional loops are present, plus for each channel pass.
    // Addition because two passes, but only approximation because there can be several iterations per buffer, so lowers it. No need to bother too much for now...
    const auto progressIterationCount = baseIterationCounter +
        (static_cast<int>((static_cast<double>(baseIterationCounter + ((baseIterationCounter - loopToCounter) * additionalLoopCount))
        * channelPassCount) * 0.7));
    auto progressIterationIndex = 0;

    // First pass: DC offset.
    DcOffsetCalculator dcOffsetCalculator(wavChannelCount);
    auto success = playAndWrite(0, psgs, replayFrequency, sidPlayerCapability, progressIterationIndex, progressIterationCount,
        [&dcOffsetCalculator](const juce::AudioSourceChannelInfo& bufferToFill) {
        // Calculates the DC offset from this buffer.
        dcOffsetCalculator.readAudioSource(bufferToFill);
        return true;
    });
    if (!success) {
        return { false, nullptr };
    }

    // Makes as many pass as there are channel to export.
    for (auto channelPass = 0; success && (channelPass < channelPassCount); ++channelPass) {
        // The filename depends on the pass.
        const auto fileName = getFileName(channelPass);
        (void)fileName.deleteFile();

        auto outputStream = std::make_unique<juce::FileOutputStream>(fileName);

        // Creates the WAV writer.
        juce::WavAudioFormat wavAudioFormat;
        const juce::StringPairArray metadataValues;     // Nothing to put inside.
        // Gives the ownership to the AudioFormatWriter. Still need a local reference to it, as it will NOT be deleted in case of failure.
        auto* localOutputStream = outputStream.release();
        const std::unique_ptr<juce::AudioFormatWriter> audioFormatWriter(wavAudioFormat.createWriterFor(localOutputStream,
                                                                                                        exportFrequencyHz, wavChannelCount,
                                                                                                        bitsPerSample, metadataValues, 0));    // No optimization.
        // Stops if the Writer couldn't be created.
        if (audioFormatWriter == nullptr) {
            delete localOutputStream;       // The output stream is NOT deleted in case of failure (cf. createWriterFor documentation).
            return { false, nullptr };
        }

        // Second pass: the song itself.
        success = playAndWrite(channelPass, psgs, replayFrequency, sidPlayerCapability, progressIterationIndex, progressIterationCount,
                                                                 [&audioFormatWriter, &dcOffsetCalculator](const juce::AudioSourceChannelInfo& bufferToFill) {
            // Writes the buffer to the Writer, it takes care of everything.
            const auto* audioSampleBuffer = bufferToFill.buffer;

            dcOffsetCalculator.applyDcOffset(bufferToFill);

            return audioFormatWriter->writeFromAudioSampleBuffer(*audioSampleBuffer, bufferToFill.startSample, bufferToFill.numSamples);
        });
    }

    return { success, nullptr };
}


// ===============================

bool SongWavExporter::playAndWrite(const int channelIndexPass, const std::vector<Psg>& psgs, const float replayFrequency, const SidPlayerCapability& sidPlayerCapability,
                                   int& progressIterationIndex, const int progressIterationCount,
                                   const std::function<bool(juce::AudioSourceChannelInfo& bufferToFill)>& applyOnBuffer) noexcept
{
    const auto psgCount = psgs.size();

    // The chain is as follows: (adapted from AudioControllerImpl)
    // - Multiple PSG generator (one per PSG, obviously).
    // - One MixerAudioSource (psgsProcessor) which mixes all the PSGs into one signal,
    //   and process the buffers to apply effects to the mixed signal (filter, stereo separation, etc.).
    // - Here, no Audio Source Player or Audio Device Manager needed.

    SongPlayer songPlayer(song);
    songPlayer.setOfflineSongEndCountBeforeMuting(additionalLoopCount + 1);
    const auto startLocation = Location(subsongId, 0);
    const auto[loopStartLocation, pastEndLocation] = song->getLoopStartAndPastEndPositions(subsongId);
    songPlayer.play(startLocation, loopStartLocation, pastEndLocation, true, true);

    PsgsProcessor psgsProcessor;
    psgsProcessor.setOutputMix(outputMix);

    // Creates the PSG Generators.
    std::vector<std::unique_ptr<PsgStreamGenerator>> psgStreamGenerators;
    psgStreamGenerators.reserve(psgCount);
    auto psgIndex = 0;
    for (const auto& psg : psgs) {
        auto psgStreamGenerator = std::make_unique<PsgStreamGenerator>(songPlayer, psg.getType(), psgIndex, replayFrequency, psg.getPsgFrequency(),
                                                                       psg.getSamplePlayerFrequency(),
                                                                       psg.getPsgMixingOutput(),
                                                                       static_cast<double>(outputMix.getChannelAVolume()) / 100.0,
                                                                       static_cast<double>(outputMix.getChannelBVolume()) / 100.0,
                                                                       static_cast<double>(outputMix.getChannelCVolume()) / 100.0,
                                                                       sidPlayerCapability);
        // Mutes all the channels in THIS PSG, but one possibly. Only if export to individual tracks.
        if (exportEachChannelIndividually) {
            std::unordered_set<int> mutedChannels;
            for (auto channelIndexPsg = 0; channelIndexPsg < PsgValues::channelCountPerPsg; ++channelIndexPsg) {
                const auto channelIndexInSong = PsgValues::getChannelIndex(channelIndexPsg, psgIndex);
                if (channelIndexInSong != channelIndexPass) {
                    mutedChannels.insert(channelIndexPsg);
                }
            }
            psgStreamGenerator->setMutedChannelIndexes(mutedChannels);
        }

        psgsProcessor.addInputSource(psgStreamGenerator.get(), false);

        psgStreamGenerators.push_back(std::move(psgStreamGenerator));
        ++psgIndex;
    }

    // Creates a small buffer where the WAV is written.
    juce::AudioSampleBuffer audioSampleBuffer(wavChannelCount, bufferSize);
    audioSampleBuffer.clear();

    juce::AudioSourceChannelInfo bufferToFill(audioSampleBuffer);

    // Prepares to play.
    psgsProcessor.prepareToPlay(bufferSize, exportFrequencyHz);

    auto mustContinue = true;
    auto success = true;
    while (mustContinue && success) {
        bufferToFill.clearActiveBufferRegion();     // This is mandatory, else the data are "stacking up".
        // Calls the player.
        psgsProcessor.getNextAudioBlock(bufferToFill);

        success = applyOnBuffer(bufferToFill);

        mustContinue = !songPlayer.hasOfflineSongEndCountReached();

        // Cancelled?
        if (isCanceled()) {
            return false;
        }

        // Notifies the progress. NOTE: this is actually only an estimation, as the iteration does not match what is produced
        // (maybe several iterations per written buffer).
        onTaskProgressed(progressIterationIndex, progressIterationCount);

        ++progressIterationIndex;
    }

    return success;
}

juce::File SongWavExporter::getFileName(const int channelPassIndex) const noexcept
{
    if (!exportEachChannelIndividually) {
        return baseOutputFileName;
    }

    return FileUtil::buildFileWithSuffix(baseOutputFileName, juce::translate(" - channel ") + juce::String(channelPassIndex + 1));
}

}   // namespace arkostracker
