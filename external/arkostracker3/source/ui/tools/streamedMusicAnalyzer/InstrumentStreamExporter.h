#pragma once

#include <juce_core/juce_core.h>

#include "../../../player/PsgRegisters.h"
#include "../../../song/instrument/Instrument.h"

namespace arkostracker 
{

class InstrumentStreamExporter
{
public:
    /** Prevents instantiation. */
    InstrumentStreamExporter() = delete;

    /**
     * Generates an Instrument
     * @param name the name if the Instrument.
     * @param channelIndex the channel index in the PSG.
     * @param psgRegisterFrames the PSG register frames.
     * @param isLooping true if the sound should be looping.
     * @param encodeAsForcedPeriods true to encode periods as fixed ones, false to encode as arpeggios+shifts.
     * @param sourcePsgFrequencyHz the frequency in Hz of the original PSG.
     * @param targetPsgFrequencyHz the frequency in Hz of the target PSG.
     * @param referenceFrequencyHz the reference frequency in Hz of the target PSG (440hz most of the time).
     * @return an Instrument.
     */
    static std::unique_ptr<Instrument> exportStreamAsInstrument(const juce::String& name, int channelIndex, const std::vector<PsgRegisters>& psgRegisterFrames,
                                                                bool isLooping, bool encodeAsForcedPeriods, int sourcePsgFrequencyHz, int targetPsgFrequencyHz,
                                                                float referenceFrequencyHz) noexcept;
};

}   // namespace arkostracker
