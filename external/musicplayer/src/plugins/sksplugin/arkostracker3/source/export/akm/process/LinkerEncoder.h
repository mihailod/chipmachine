#pragma once

#include <juce_core/juce_core.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../utils/Id.h"

namespace arkostracker 
{

class SourceGenerator;
class Song;
class SongExportResult;

/**
 * Encodes the Linker for the AKM format.
 */
class LinkerEncoder
{
public:
    /**
     * Constructor.
     * @param sourceGenerator the source generator.
     * @param songExportResult to fill the status report.
     * @param song the Song.
     * @param subsongId the ID of the Subsong.
     * @param subsongLabel the label of the Subsong, used for the looping of the linker.
     * @param trackIndexLabel the label for the Track Index table.
     * @param getTrackLabel function to get the label of a Track from its index.
     */
    LinkerEncoder(SourceGenerator& sourceGenerator, SongExportResult& songExportResult,
                  const Song& song, Id subsongId, juce::String subsongLabel, juce::String trackIndexLabel,
                  std::function<juce::String(int trackIndex)> getTrackLabel) noexcept;

    /** Encodes the Linker. */
    void encodeLinker() noexcept;

private:
    const static unsigned int minimumTrackCountForReference;                         // Minimum times a Track must be used for it to be worth referenced.
    const static juce::String loopLabelSuffix;

    /**
     * Checks that the Speed Track has none Speed item, or only one at the beginning. Else, emits a warning.
     * @param speedTrackIndex the index of the Speed Track.
     * @return the speed, or 0 if there is none.
     */
    int checkSpeedTrackAndGetSpeed(int speedTrackIndex) const noexcept;

    /**
     * Calculates the occurrence of each Track, filling internal maps.
     * @param patterns the patterns of the Song.
     */
    void calculateMostUsedTrackIndexes(const std::vector<Pattern>& patterns) noexcept;

    /**
     * Encodes a Track reference or offset.
     * @param channelIndex the channel index, only for display.
     * @param trackIndex the track index to encode.
     */
    void encodeTrackInPattern(int channelIndex, int trackIndex) noexcept;

    /** Encodes the Track indexes. */
    void encodeTrackIndexes() const noexcept;

    SourceGenerator& sourceGenerator;
    SongExportResult& songExportResult;
    const Song& song;
    Id subsongId;
    juce::String subsongLabel;
    juce::String trackIndexLabel;
    std::function<juce::String(int trackIndex)> getTrackLabel;

    /** The track indexes to their occurrence, from most to least used. Only contained the ones that are worth reference. */
    std::vector<std::pair<int, unsigned int>> sortedMostUsedTrackIndexesToOccurrence;
};


}   // namespace arkostracker

