#pragma once

#include <juce_core/juce_core.h>

#include "../SongImporter.h"
#include "../../business/song/tool/builder/SongBuilder.h"

namespace arkostracker 
{

/** Imports a WYZ Tracker music file. */
class WyzSongImporter : public SongImporter
{
public:
    /** Constructor. */
    WyzSongImporter();

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    class FxCell
    {
    public:
        FxCell(int pFxId, int pBaseTrackIndex, int pLineIndex) noexcept :
                fxId(pFxId),
                baseTrackIndex(pBaseTrackIndex),
                lineIndex(pLineIndex)
        {
        }

        const int fxId;             /** According to the original Pattern. */
        const int baseTrackIndex;   /** 0, 1, 2 must be added. */
        const int lineIndex;
    };

    static const int wyzRstNoteValue;
    static const int fxCellNote;

    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param songNode the node of the Song.
     */
    void parse(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Patterns and Positions and fills the current SongBuilder.
     * @param songNode the top node of the Song.
     */
    void parsePatternsAndPositions(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses a Cell and fills the current SubsongBuilder.
     * @param trackIndex the track index.
     * @param channelIndex the channel index (0-2).
     * @param cellIndex the cell index.
     * @param cellNode the cell node to parse.
     */
    void parseCell(int trackIndex, int channelIndex, int cellIndex, const juce::XmlElement& cellNode);

    /**
     * Parses the Instruments and fills the current SongBuilder.
     * @param songNode the top node of the Song.
     */
    void parseInstruments(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Effects, converted to Instruments and fills the current SongBuilder.
     * @param songNode the top node of the Song.
     */
    void parseFxs(const juce::XmlElement& songNode) const noexcept;

    /**
     * Writes the FXs as real cells.
     * @param fxChannel the channel where the FX are declared (>=0).
     */
    void encodeFxs(int fxChannel) const noexcept;

    std::unique_ptr<SongBuilder> songBuilder;

    int highestInstrumentIndex;                             // Not counting the FXs.

    std::vector<FxCell> fxCells;                            //Found FXs, to be added once the Instruments are all read.
    std::vector<int> currentInstrumentPerChannel;           // Links a Channel to an Instrument, as there can be "ghost" notes.
};

}   // namespace arkostracker