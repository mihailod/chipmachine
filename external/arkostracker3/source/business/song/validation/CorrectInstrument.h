#pragma once

namespace arkostracker
{

class Instrument;

/** Corrects the given instrument. */
class CorrectInstrument
{
public:
    /** Prevents instantiation. */
    CorrectInstrument() = delete;

    /**
     * Corrects the given instrument.
     * @param instrument the instrument. Will be modified if needed.
     */
    static void correctInstrument(Instrument& instrument) noexcept;

private:
    /** Corrects the instrument, which MUST be a PSG instrument. */
    static void correctPsgInstrument(Instrument& instrument) noexcept;
    /** Corrects the instrument, which MUST be a Sample instrument. */
    static void correctSampleInstrument(Instrument& instrument) noexcept;
};

}   // namespace arkostracker
