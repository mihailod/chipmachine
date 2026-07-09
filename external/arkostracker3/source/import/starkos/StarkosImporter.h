#pragma once

#include "../SongImporter.h"

#include "../../business/song/tool/builder/SongBuilder.h"

namespace arkostracker 
{

/** Imports a Starkos (.SKS) file. */
class StarkosImporter final : public SongImporter
{
public:
    /** Constructor. */
    StarkosImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    static const juce::String headerTag;                        // Tag to identify the song format.
    static constexpr auto headerTagSize = 10;                    // The size of the tag above.
    static constexpr auto offsetAuthor = headerTagSize;
    static constexpr auto sizeAuthor = 10;
    static constexpr auto offsetComments = offsetAuthor + sizeAuthor;
    static constexpr auto sizeComments = 32;
    static constexpr auto offsetDigichannel = offsetComments + sizeComments;
    static constexpr auto offsetLinker = 58;
    static constexpr auto songMinimumSize = offsetLinker + 2;        // Allows to get the metadata and the pattern count.

    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param musicData the music data.
     */
    void parse(const juce::MemoryBlock& musicData) const noexcept;

    /**
     * Converts the given player frequency index to a frequency in Hz. If unknown, uses 50hz.
     * @param playerFrequencyIndex the frequency index (0=13hz, 1=25, 2=50... 5=300).
     * @return the frequency in Hz.
     */
    static float convertPlayerFrequencyIndexToHz(int playerFrequencyIndex) noexcept;


    /**
     * Parses the SKS to read the linker. The Patterns themselves are not read yet.
     * @param subsongBuilder to build the Subsong.
     * @param musicData the data of the song.
     * @param readOffset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     */
    void readLinker(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) const noexcept;

    /**
     * Parses the SKS to read and creates the Instruments.
     * @param musicData the data of the song.
     * @param readOffset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     */
    void readInstruments(const juce::MemoryBlock& musicData, juce::int64& readOffset) const noexcept;

    /**
     * Parses the SKS to read and create the Special Tracks.
     * @param subsongBuilder to build the Subsong.
     * @param musicData the data of the song.
     * @param readOffset the offset of the first Special Tracks. It must be modified to reach the first byte after it.
     */
    static void readSpecialTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) noexcept;

    /**
     * Parses the SKS to read and create the Tracks.
     * @param subsongBuilder to build the Subsong.
     * @param musicData the data of the song.
     * @param readOffset the offset of the first Tracks. It must be modified to reach the first byte after it.
     */
    static void readTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
};

}   // namespace arkostracker
