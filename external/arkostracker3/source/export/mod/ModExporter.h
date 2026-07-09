#pragma once

#include <juce_core/juce_core.h>

#include "../../song/Song.h"
#include "../../utils/Id.h"
#include "../../utils/task/Task.h"

namespace arkostracker
{

class ModExporter final : public Task<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to export.
     * @param minimumDurationMs the possible minimum duration in ms for PSG instruments (or 0).
     * @param outputStream where to write the data.
     */
    ModExporter(std::shared_ptr<Song> song, Id subsongId, int minimumDurationMs, juce::OutputStream& outputStream) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    static constexpr auto maximumLengthSongName = 20;
    static constexpr auto maximumLengthInstrumentName = 22;
    static constexpr auto sampleMaximumSize = 65535 - 1;    // -1 because the repeat information encoded in the sample data itself.
    static constexpr auto markerSize = 4;
    static constexpr auto maximumVolumeInTrack = 64;
    static constexpr auto defaultSpeed = 6;
    static constexpr auto minimumSpeed = 1;
    static constexpr auto maximumSpeed = 32;

    static constexpr auto maximumInstrumentCount = 31;  // 1-31.
    static constexpr auto maximumPositionCount = 128;   // 1-128.
    static constexpr auto modPatternHeight = 64;
    static constexpr auto lastInstrumentIndex = maximumInstrumentCount;

    static constexpr auto encodedCellSize = 4;

    static constexpr auto effectNumberVolume = 0xc;
    static constexpr auto effectNumberPatternBreak = 0xd;
    static constexpr auto effectNumberSpeed = 0xf;

    /** Encodes a text with 0 up to "maximum length". If too long, the text is cropped. */
    void encodePaddedText(const juce::String& text, int maximumLength) const noexcept;

    /**
     * Encodes the samples in the output stream.
     * @return true if everything went fine.
     */
    bool encodeSampleHeaderAndStoreData() noexcept;

    /**
     * Encodes the linker in the output stream.
     * @return true if everything went fine.
     */
    bool encodeLinker() noexcept;

    /** Encodes the patterns in the output stream. */
    void encodePatterns() noexcept;

    /** Encodes the sample data in the output stream. */
    void encodeSampleData() const noexcept;

    /**
     * Encodes one pattern in the output stream.
     * @param patternIndex the pattern index.
     */
    void encodePattern(int patternIndex) noexcept;

    /** @return the Amiga sample period from the given note. */
    static int getSamplePeriod(int note) noexcept;

    /** @return a 8-bit volume to be encoded, from the given AT volume (0-15). */
    static unsigned int getEncodedVolume(int atVolume) noexcept;

    /** Encodes a cell in the given pattern. */
    void encodeCell(juce::MemoryBlock& patternData, int channelIndex, int cellIndex,
        unsigned int samplePeriod, unsigned int instrumentIndex, unsigned int effectNumber, unsigned int effectValue) const noexcept;

    std::shared_ptr<Song> song;
    Id subsongId;
    Psg psg;
    int minimumDurationMs;
    juce::OutputStream& outputStream;

    std::vector<std::unique_ptr<juce::MemoryBlock>> instrumentSampleData;

    std::unordered_map<int, int> patternIndexToHeight;

    int modChannelCount;               // How many channels there are. Warning, might be different from what is inside the song (larger or smaller)!
    int lastPatternIndex;
    int firstPositionPatternIndex;
};

}   // namespace arkostracker
