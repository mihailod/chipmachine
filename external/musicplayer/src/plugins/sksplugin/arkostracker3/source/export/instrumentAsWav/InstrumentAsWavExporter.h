#pragma once

#include <juce_core/juce_core.h>

#include "../../song/Song.h"
#include "../../utils/Id.h"
#include "../../utils/task/Task.h"

namespace arkostracker
{

class SongController;

class InstrumentAsWavExporter final : public Task<std::unique_ptr<std::vector<juce::File>>>
{
public:

    /**
     * Constructor.
     * @param song the Song.
     * @param instrumentId the ID of the instrument to export. Must be valid.
     * @param outputFolder the output folder where to create the output file(s). Must exist.
     * @param baseOutputFileNameWithoutExtension the base output file name, without the note being appended ("mySound" for example).
     * @param firstNote the first note to export.
     * @param lastNote the last note. Must be at least equal to the first note.
     * @param psg the PSG which data to use.
     * @param sidPlayerCapability the SID player capabilities.
     * @param replayFrequencyHz the replay frequency to use, in Hz.
     * @param minimumDurationMs a possible minimum duration, for PSG only. 0 if not used.
     * @param maximumSize if present, a maximum size of the raw sample size, in bytes. Useful for MOD export, which only handles 64kb samples.
     */
    InstrumentAsWavExporter(const std::shared_ptr<Song>& song, Id instrumentId, juce::File outputFolder,
        juce::String baseOutputFileNameWithoutExtension, int firstNote, int lastNote, const Psg& psg,
        float replayFrequencyHz, const SidPlayerCapability& sidPlayerCapability, int minimumDurationMs, OptionalInt maximumSize) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<std::vector<juce::File>>> performTask() noexcept override;

private:
    static constexpr auto wavOutputChannelCount = 1;
    static constexpr auto bitsPerSample = 8;
    static constexpr auto psgSampleRate = 44100;
    static constexpr auto bufferSize = 512;

    static constexpr auto channelIndex = 0;

    /**
     * Creates a file for one note, for the Instrument.
     * @param note the note.
     * @return the generated file, or absent if an error occurred.
     */
    OptionalValue<juce::File> renderNote(int note) const noexcept;

    const std::shared_ptr<Song> song;
    Psg psg;
    SidPlayerCapability sidPlayerCapability;
    float replayFrequencyHz;
    int minimumDurationMs;
    OptionalInt maximumSize;

    Id instrumentIdToExport;
    juce::File outputFolder;
    juce::String baseOutputFileNameWithoutExtension;
    int firstNote;
    int lastNote;

    Id emptyInstrumentId;
};

}   // namespace arkostracker
