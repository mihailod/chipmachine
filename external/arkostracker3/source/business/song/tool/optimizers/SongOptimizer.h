#pragma once

#include <memory>

#include "../../../../utils/Id.h"

namespace arkostracker 
{

class ExportConfiguration;
class Song;

/**
 * Optimizes a Song and returns a new one. The given Song is not modified.
 *
 * This can be used by exporters, but also in the main software. However, the latter MUST unoptimize the Song
 * after because Tracks will be shared!
 *
 * The Patterns are optimized twice, at the beginning and the end, to ensure the best result.
 */
class SongOptimizer
{
public:
    /** Prevents instantiation. */
    SongOptimizer() = delete;

    /**
     * Creates a new optimized Song.
     * @param song the original Song.
     * @param optimizePatterns true to optimize the Pattens and Tracks.
     * @param normalizeCellsAndOptimizeTracks true to normalize the Cells, and optimize the Tracks (remove useless effects). Only relevant if optimizePatterns is true.
     * @param optimizeInstruments true to optimize the Instruments. This may modify the Tracks.
     * @param keepSamples true to keep the samples. Only relevant if optimizeInstruments is true.
     * @param flattenInstruments true to "flatten" the Instruments (removing the auto-spread loops). Only relevant if optimizeInstruments is true.
     * @param transferInstrumentRetrig true to remove the header retrig. Only relevant if optimizeInstruments is true.
     * @param optimizeExpressions true to optimize the Expressions. This may modify the Tracks.
     * @param flattenExpressions true to "flatten" the Expressions (removing the shifts). Only relevant if optimizeExpressions is true.
     * @param convertInlineArpeggios true to remove all inline arpeggios and replace them with Arpeggio Tables.
     * @param convertLegatoToEffect true to replace the Legato note by an effect. This must be true only for export. The main software must not show this.
     * @param subsongIdsToKeep the SubsongIds to keep.
     * @return the optimized Song, or nullptr if an error occurred.
     */
    static std::unique_ptr<Song> optimize(const Song& song, bool optimizePatterns, bool normalizeCellsAndOptimizeTracks,
                                          bool optimizeInstruments, bool keepSamples, bool flattenInstruments, bool transferInstrumentRetrig,
                                          bool optimizeExpressions, bool flattenExpressions,
                                          bool convertInlineArpeggios, bool convertLegatoToEffect,
                                          const std::vector<Id>& subsongIdsToKeep) noexcept;

    /**
     * Creates a new optimized Song, with full optimizations. Ideal for exports.
     * @param song the original Song.
     * @param subsongIdsToKeep the SubsongIds to keep.
     * @param keepSamples true to keep the samples.
     * @return the optimized Song, or nullptr if an error occurred.
     */
    static std::unique_ptr<Song> optimize(const Song& song, const std::vector<Id>& subsongIdsToKeep, bool keepSamples = false) noexcept;

    /**
     * Creates a new optimized Song, with full optimizations. Ideal for exports.
     * IMPORTANT: the export configuration is only useful to get the Subsong Ids. However, as new songs are generated,
     * the IDs it contains will be overwritten.
     * @param song the original Song.
     * @param exportConfiguration the configuration to use. Its SubsongIds will be replaced.
     * @return the optimized Song, or nullptr if an error occurred.
     */
    static std::unique_ptr<Song> optimize(const Song& song, ExportConfiguration& exportConfiguration) noexcept;
};

}   // namespace arkostracker
