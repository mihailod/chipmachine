#pragma once

#include <memory>

#include "../../../../song/Song.h"

namespace arkostracker 
{

/**
 * Creates a new Song from the given one, with Expressions optimized, possibly removed (Tracks modified as well).
 * All the Subsongs may be modified.
 *
 * The inline Arpeggios should have been removed, but if found, they are not modified.
 * It is advised to optimize the Patterns first before calling this. That was only the useful Tracks will remain, and their Instrument.
 */
class ExpressionsOptimizer
{
public:
    /**
     * Constructor.
     * @param originalSong the original Song to optimize.
     * @param flatten should be true for exports. It will apply (and delete) the auto-loops. Use false for non-export, but some optimizations will be skipped.
     */
    ExpressionsOptimizer(const Song& originalSong, bool flatten) noexcept;

    /**
     * Optimizes the Song. Must be called only once.
     * @return the new Song.
     */
    std::unique_ptr<Song> optimize() noexcept;

private:
    /**
     * Optimizes the Song. Must be called only once for a type of Expression.
     * @param isArpeggio true if Arpeggio, false if Pitch
     * @return the new Song.
     */
    void optimize(bool isArpeggio) noexcept;

    /**
     * Makes sure all the Expressions exist. Illegal expressions are replaced by 0.
     * @param isArpeggio true if Arpeggio, false if Pitch
     */
    void checkExpressionsValidity(bool isArpeggio) noexcept;

    const Song& originalSong;
    std::unique_ptr<Song> newSong;
    const bool flatten;
};

}   // namespace arkostracker
