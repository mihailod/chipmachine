#pragma once

namespace arkostracker 
{

class Instrument;
class Song;

/**
 * Can be used for the Song, or with individual Instrument (see transformInstrumentIfNeeded).
 *
 * Parses the Instruments, modifies the PSG ones with a Retrig in the Instrument header, so that
 * it is applied to the first line. A first line is generated, the loop is modified if needed, a last line
 * is added if needed at the loop end.
 *
 * Also, as an optimization, if an Instrument Retrig is set but the first line is not an hardware sound, it is removed.
 *
 * Warning, the Song WILL be modified.
 */
class RetrigInInstrumentHeaderNormalizer
{
public:

    /**
     * Constructor.
     * @param song the Song. It will be modified.
     */
    explicit RetrigInInstrumentHeaderNormalizer(Song& song) noexcept;

    /** Applies the transformation. */
    void apply() noexcept;

    /** Transforms the given Instrument, but only if it is required. If not a PSG Instrument, nothing is done. */
    static void transformInstrumentIfNeeded(Instrument& instrument) noexcept;

    Song& song;                // The Song.
};


}   // namespace arkostracker

