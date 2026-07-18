#pragma once

#include <memory>

#include "../../player/PsgRegistersProvider.h"
#include "../../player/channel/ChannelPlayer.h"
#include "../../song/Song.h"

namespace arkostracker
{

class InstrumentRenderer final : PsgRegistersProvider,
                                 SongDataProvider
{
public:
    InstrumentRenderer(const std::shared_ptr<Song>& song, const Psg& psg, const SidPlayerCapability& sidPlayerCapability) noexcept;

    /**
     * Renders a note, as a 32-bit, little-endian float.
     * @param note the note.
     * @param instrumentId the ID of the instrument.
     * @param outputStream where to encode the data.
     * @param replayFrequencyHz the replay frequency in Hz to use.
     * @param psgSampleRate the sample rate for generating the PSG flux (44100 for ex).
     * @param minimumDurationMs the minimum duration is ms (0 possible).
     * @param maximumSize the possible maximum size.
     * @return the sample replay frequency.
     */
    int renderNote(int note, const Id& instrumentId, juce::OutputStream& outputStream,
                    float replayFrequencyHz, int psgSampleRate, int minimumDurationMs, OptionalInt maximumSize) noexcept;

    /** Marks the task as "canceled". */
    void setCanceled() noexcept;

private:
    static constexpr auto channelIndex = 0;

    /** Renders a note, as a 8-bit, float, for a Sample Instrument. */
    static void renderNoteForSampleInstrument(const Song& song, int note, const Id& instrumentId,
        juce::OutputStream& outputStream, OptionalInt maximumSize) noexcept;

    /** Renders a note, as a 8-bit, float, for a PSG Instrument. */
    void renderNoteForPsgInstrument(int note, const Id& instrumentId, juce::OutputStream& outputStream,
        float replayFrequencyHz, int psgSampleRate, int minimumDurationMs) noexcept;

    /**
     * @return the sample rate to use for the exported file
     * @param song the Song.
     * @param instrumentId the ID of the instrument.
     * @param instrumentType the type of the instrument.
     * @param psgSampleRate the PSG sample rate.
     */
    static int getOutputSampleRate(const Song& song, const Id& instrumentId, InstrumentType instrumentType, int psgSampleRate) noexcept;

    // PsgRegistersProvider method implementations.
    // ================================================
    std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> getNextRegisters(int psgIndex) noexcept override;

    // SongDataProvider method implementations.
    // ================================================
    OptionalId getInstrumentIdFromAudioThread(int instrumentIndex) const noexcept override;
    InstrumentType getInstrumentTypeFromAudioThread(const OptionalId& instrumentId) const noexcept override;
    OptionalId getExpressionIdFromAudioThread(bool isArpeggio, int expressionIndex) const noexcept override;
    ExpressionMetadata getExpressionMetadataFromAudioThread(bool isArpeggio, const OptionalId& expressionId) const noexcept override;
    PsgInstrumentFrameData getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, int cellIndex) const noexcept override;
    SampleInstrumentFrameData getSampleInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId) const noexcept override;
    int getExpressionValueFromAudioThread(bool isArpeggio, const OptionalId& expressionId, int cellIndex) const noexcept override;
    std::pair<int, float> getPsgFrequencyFromChannelFromAudioThread(int channelIndexInSong) const noexcept override;
    int getTranspositionFromAudioThread(int channelIndexInSong) const noexcept override;
    bool isEffectContextEnabled() const noexcept override;
    LineContext determineEffectContextFromAudioThread(CellLocationInPosition location) const noexcept override;
    LineContext determineEffectContextFromAudioThread(int channelIndexInSong) const noexcept override;
    SidPlayerCapability getSidPlayerCapability() const noexcept override;

    const std::shared_ptr<Song> song;
    Psg psg;
    SidPlayerCapability sidPlayerCapability;
    Id emptyInstrumentId;
    int iterationCount;

    ChannelPlayer channelPlayer;
    std::atomic_bool canceled;
};

}   // namespace arkostracker
