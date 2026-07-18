#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker
{

class YmReader;

/** Converts any given YM into YM6, interleaved (regs per regs). It can also convert the target frequencies, if wanted. */
class YmGenerator
{
public:
    /**
     * Constructor.
     * @param inputYm the input YM. May be invalid.
     * @param outputStream the output stream.
     * @param targetPsgFrequencyHz if present, the frequency of the target PSG, in Hz (1000000 for CPC for example), or absent to keep the original one.
     */
    YmGenerator(const juce::MemoryBlock& inputYm, juce::MemoryOutputStream& outputStream, OptionalInt targetPsgFrequencyHz);

    /**
     * Performs the convertion.
     * @return true if everything went fine.
     */
    bool convert();

private:
    void encodeDigidrums(const YmReader& ymReader) const noexcept;

    juce::MemoryInputStream inputYmStream;
    juce::MemoryOutputStream& outputStream;
    OptionalInt optionalTargetPsgFrequencyHz;
};

}   // namespace arkostracker
