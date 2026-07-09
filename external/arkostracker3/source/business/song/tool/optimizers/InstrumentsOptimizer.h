#pragma once

#include <memory>

#include "../../../../song/Song.h"

namespace arkostracker 
{

/**
 * Creates a new Song from the given one, with Instruments optimized, possibly removed (Tracks modified as well).
 * The samples can also be removed if wanted.
 * All the Subsongs may be modified.
 *
 * It is advised to optimize the Patterns first before calling this. That was only the useful Tracks will remain, and their Instrument.
 */
class InstrumentsOptimizer
{
public:
    /**
     * Constructor.
     * @param originalSong the original Song to optimize.
     * @param keepSamples true to keep the samples, false to remove them.
     * @param flatten should be true for exports. It will apply (and delete) the auto-loops. Use false for non-export, but some optimizations will be skipped.
     * @param transferInstrumentRetrig if true, remove the Instrument Retrig and put it in the Instrument itself. Some exporters require that.
     */
    InstrumentsOptimizer(const Song& originalSong, bool keepSamples, bool flatten, bool transferInstrumentRetrig) noexcept;

    /**
     * Optimizes the Song. Must be called only once.
     * @return the new Song.
     */
    std::unique_ptr<Song> optimize() noexcept;

private:
    /** Makes sure all the Instruments exist. Illegal instruments are replaced by RST. */
    void checkInstrumentsValidity() const noexcept;

    const Song& originalSong;
    std::unique_ptr<Song> newSong;
    const bool keepSamples;
    const bool flatten;
    const bool transferInstrumentRetrig;
};

}   // namespace arkostracker
