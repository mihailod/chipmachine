#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include "../../utils/task/Task.h"
#include "../../controllers/model/OutputMix.h"
#include "../../song/Song.h"

namespace arkostracker
{

class PsgRegisters;

/** Exports one Subsong as a WAV stream. */
class SongWavExporter final : public Task<std::unique_ptr<bool>>      // The result is only boiler-plate.
{
public:
    /**
     * Constructor.
     * @param song the Song to read.
     * @param subsongId the ID of the Subsong to read. Must be valid.
     * @param exportFrequencyHz the frequency of the export, in Hz (44100 for example).
     * @param additionalLoopCount how many loop to record after the song is over.
     * @param outputMix how to mix the output.
     * @param baseOutputFileName the file to write to. It will be deleted. If exporting each channel, it is used as a base name.
     * @param exportEachChannelIndividually true to export each channel individually, in stereo.
     */
    SongWavExporter(std::shared_ptr<Song> song, Id subsongId, double exportFrequencyHz, int additionalLoopCount,
                    const OutputMix& outputMix, juce::File baseOutputFileName, bool exportEachChannelIndividually) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    static const unsigned int wavChannelCount;
    static const int bitsPerSample;
    static const int bufferSize;

    /**
     * Plays the song (with loops if wanted), filling the buffer in the lambda.
     * @param channelIndexPass the index of the channel for this pass, relevant only if export each track individually.
     * @param psgs the PSGs of the Subsong.
     * @param replayFrequency the replay frequency in Hz.
     * @param sidPlayerCapability the SID player capabilities.
     * @param progressIterationIndex the iteration index. This method will make it evolve.
     * @param progressIterationCount the iteration count.
     * @param applyOnBuffer the method to apply on the buffer (writing it into a WAV file, etc.).
     * @return true if success.
     */
    bool playAndWrite(int channelIndexPass, const std::vector<Psg>& psgs, float replayFrequency, const SidPlayerCapability& sidPlayerCapability, int& progressIterationIndex,
        int progressIterationCount, const std::function<bool(juce::AudioSourceChannelInfo& bufferToFill)>& applyOnBuffer) noexcept;

    /**
     * @return the full filename of the file to generate.
     * @param channelPassIndex the channel pass index (>=0). Used only if export each channel individually.
     * */
    juce::File getFileName(int channelPassIndex) const noexcept;

    std::shared_ptr<Song> song;
    Id subsongId;
    double exportFrequencyHz;
    int additionalLoopCount;
    OutputMix outputMix;
    juce::File baseOutputFileName;
    bool exportEachChannelIndividually;
};

}   // namespace arkostracker
