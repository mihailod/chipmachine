#pragma once

#include <juce_core/juce_core.h>

#include "NoteAndShift.h"

namespace arkostracker 
{

/**
 * Class to calculate PSG periods.
 */
class PsgPeriod
{
public:
    /**
     * Constructor.
     * @param psgFrequencyHz the frequency of the PSG, in Hz (1000000 for a CPC for example).
     * @param referenceFrequencyHz the reference frequency (440 hz most of the time).
     */
    PsgPeriod(int psgFrequencyHz, float referenceFrequencyHz) noexcept;

    /** Finds the closest note, with a possible shift, for the given period. */
    NoteAndShift findNoteAndShift(int period) noexcept;

    /**
     * @return the period.
     * @param psgFrequency the frequency of the PSG, in Hz (1 000 000 for the CPC).
     * @param referenceFrequency the reference frequency, in Hz (440 most of time).
     * @param noteIndex the index of the note (0 = c-0, etc.). May be out of boundaries, will be corrected.
     */
    static int getPeriod(int psgFrequency, float referenceFrequency, int noteIndex) noexcept;

    /**
     * @return the frequency from the given period.
     * @param psgFrequency the frequency of the PSG, in Hz (1 000 000 for the CPC).
     * @param period the period.
     */
    static double getFrequency(int psgFrequency, int period) noexcept;

private:
    std::map<juce::uint16, int> periodToNoteIndex;
    std::map<int, juce::uint16> noteIndexToPeriod;
};

}   // namespace arkostracker
