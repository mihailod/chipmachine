#pragma once

#include <memory>

#include "../SongExportResult.h"
#include "../../song/Song.h"
#include "../../utils/task/Task.h"
#include "../ExportConfiguration.h"
#include "process/EncodedEffectBlock.h"

namespace arkostracker 
{

class SourceGenerator;

/**
 * Exports a Song to AKG format, as source.
 * The input Song will be optimized, nothing to do before.
 * Note: the samples are skipped if the export configuration tells so, but the optimizer is most likely supposed to remove them.
 */
class AkgExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:

    /**
     * Constructor.
     * @param originalSong the original Song. No need to optimize it.
     * @param exportConfiguration data about how to export.
     */
    AkgExporter(const Song& originalSong, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    static const size_t fastEffectCount;
    static const juce::String startLabelString;                 // Label for the start of Song/Subsongs.
    static const juce::String arpeggioTableLabelString;         // Label for the Arpeggio Table.
    static const juce::String arpeggioLabelString;              // Label for an Arpeggio.
    static const juce::String pitchTableLabelString;            // Label for the Pitch Table.
    static const juce::String pitchLabelString;                 // Label for a Pitch.
    static const juce::String instrumentTableLabelString;       // Label for an Instrument Table.
    static const juce::String instrumentLabelString;            // Label for an Instrument.
    static const juce::String emptyInstrumentLabel;             // Label for an Instrument.
    static const juce::String effectBlockTableLabelString;      // Label for a the Effect Blocks Table.
    static const juce::String effectBlockLabelString;           // Label for an Effect Block.
    static const juce::String loopSuffixLabelString;		    // Suffix, in a Label, to mark a loop.

    /** Holder of an ID of an EffectBlock and its weight. */
    class EffectBlockIdAndWeight
    {
    public:
        /** Constructor. */
        EffectBlockIdAndWeight(juce::String pId, const int pWeight) noexcept :
                id(std::move(pId)),
                weight(pWeight)
        {
        }
        /** Default constructor. Only declared for the resize operation, but no use. */
        EffectBlockIdAndWeight() noexcept : id(juce::String()), weight(-1)
        {
            jassertfalse;
        }

        const juce::String& getId() const noexcept
        {
            return id;
        }

        int getWeight() const noexcept
        {
            return weight;
        }

    private:
        juce::String id;
        int weight;
    };

    /** Predicate to sort the EffectBlockIdAndWeight from heaviest to lightest. */
    class SortHeaviestEffectBlockIdsPredicate
    {
    public:
        bool operator()(const EffectBlockIdAndWeight& left, const EffectBlockIdAndWeight& right) const {
            return (left.getWeight() > right.getWeight());
        }
    };

    /**
     * @return the base label to be used before every label.
     * @param subsongIndex if >=0, adds the subsong index after it.
    */
    juce::String getBaseLabel(int subsongIndex = -1) const noexcept;

    /**
     * @return the label for the Arpeggio table.
     */
    juce::String getArpeggioTableLabel() const noexcept;

    /**
     * @return the label for an Arpeggio.
     * @param arpeggioIndex the index of the Arpeggio.
     */
    juce::String getArpeggioLabel(int arpeggioIndex) const noexcept;

    /** @return the label for the Pitch table. */
    juce::String getPitchTableLabel() const noexcept;

    /**
     * @return the label for an Pitch.
     * @param pitchIndex the index of the Pitch.
     */
    juce::String getPitchLabel(int pitchIndex) const noexcept;

    /** @return the label for the Instrument table. */
    juce::String getInstrumentTableLabel() const noexcept;

    /** @return the label for the EffectBlock table. */
    juce::String getEffectBlockTableLabel() const noexcept;

    /**
     * @return the label for an Instrument.
     * @param instrumentIndex the index of the Instrument.
     */
    juce::String getInstrumentLabel(int instrumentIndex) const noexcept;

    /**
     * @return the label for an Effect Block. It is NOT related to any Subsong.
     * @param encodedEffectBlock the Encoded Effect Block.
     */
    juce::String getEffectBlockLabel(const EncodedEffectBlock& encodedEffectBlock) const noexcept;

    /**
     * @return the label for an Effect Block. It is NOT related to any Subsong.
     * @param encodedEffectBlockId the Encoded Effect Block id.
     */
    juce::String getEffectBlockLabel(const juce::String& encodedEffectBlockId) const noexcept;

    /**
     * Encodes the Arpeggio or Pitch table, and then Arpeggios/Pitches themselves.
     * @return false if an Expression couldn't be found.
     */
    bool encodeExpressionTableAndExpressions(SourceGenerator& sourceGenerator, bool isArpeggio) const noexcept;

    /**
     * Encodes an Expression.
     * @param sourceGenerator the source generator where to declare the data.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param expressionIndex the index of the Expression to encode.
     * @return false if the Expression couldn't be found.
     */
    bool encodeExpression(SourceGenerator& sourceGenerator, bool isArpeggio, int expressionIndex) const noexcept;

    /** @returns the speed, increased of 1. If 256, it becomes 0. */
    static int getIncreasedSpeed(int speed) noexcept;

    /**
     * Encodes the Instrument table and the Instruments.
     * @param sourceGenerator the source generator where to declare the data.
     * @param playerConfiguration the Player Configuration to fill.
     * @return true if everything went fine. False if the sample was too big (>65535).
     */
    bool encodeInstrumentTableAndInstruments(SourceGenerator& sourceGenerator, PlayerConfiguration& playerConfiguration) const noexcept;

    /**
     * Encodes a Sample Instrument. It does not check for the right to do so.
     * @param sourceGenerator the source generator where to declare the data.
     * @param instrumentIndex the index of the Instrument. It must be a Sample Instrument.
     * @param instrument the Instrument.
     * @return true if everything went fine. False if the sample was too big (>65535).
     */
    bool encodeSampleInstrument(SourceGenerator& sourceGenerator, int instrumentIndex, const Instrument& instrument) const noexcept;

    /**
     * Encodes an Instrument. It must be called on a PSG Instrument.
     * @param sourceGenerator the source generator where to declare the data.
     * @param playerConfiguration the Player Configuration to fill.
     * @param instrumentIndex the index of the Instrument.
     * @param instrument the Instrument.
     */
    void encodePsgInstrument(SourceGenerator& sourceGenerator, PlayerConfiguration& playerConfiguration,
                             int instrumentIndex, const Instrument& instrument) const noexcept;

    /** Generates the Effect Blocks of all the Subsongs, builds the Effect Blocks set. */
    void generateEffectBlocks(PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Increases the reference counter of this Effect Block.
     * @param encodedEffectBlock the Effect Block.
     */
    void increaseEffectBlockReferenceCounter(const EncodedEffectBlock& encodedEffectBlock) noexcept;

    /** Builds a table linked each Effect Block to its offset from the EffectBlock Table. Useful to encode them as relative address in the Subsong. */
    void buildEffectBlocksOffset() noexcept;

    /** Builds the set to know which EffectBlocks are the most used and "heavy" in memory. */
    void buildEffectBlockWeight() noexcept;

    /** Encodes the possible Effect Block indexes table, if there are any indexes used. */
    void encodeEffectBlockIndexesTable(SourceGenerator& sourceGenerator) const noexcept;

    /** Encodes the Effect Blocks. The generateEffectBlocks must have been called before. */
    void encodeEffectBlocks(SourceGenerator& sourceGenerator) const noexcept;

    /**
     * Encodes the given Effect Block.
     * @param sourceGenerator the source generator where to declare the data.
     * @param encodedEffectBlock the Effect Block to encode.
     */
    void encodeEffectBlock(SourceGenerator& sourceGenerator, const EncodedEffectBlock& encodedEffectBlock) const noexcept;

    const Song& originalSong;
    std::unique_ptr<Song> song;
    ExportConfiguration exportConfiguration;

    std::vector<EncodedEffectBlock> effectBlocks;   // All the Effect blocks from all the encoded Subsongs. They are in no particular order, but they are encoded in ONE order.
    std::map<juce::String, int> effectBlockIdToReferenceCounter;         // Links a generated effect block id to how many times it has been used.
    std::map<juce::String, int> effectBlockIdToIndex;                    // Stores the ids of all Effect Blocks that must be encoded as indexes.
    std::vector<juce::String> indexToEffectBlockId;                      // Stores the ids of all Effect Blocks that must be encoded as indexes (their position being the index).
    std::map<juce::String, int> effectBlockIdToOffsetFromEffectBlockTable;  // Links the ids of all Effect Blocks to their offset, in bytes, from the Effect Block table.
};

}   // namespace arkostracker
