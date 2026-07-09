#pragma once

#include "YmStreamEncoder.h"

namespace arkostracker 
{

/** Implementation of an YM Stream Encoder, for a non-interleaved format. This can be done in one pass (every register of a frame encoded, one frame after the other). */
class YmStreamEncoderNonInterleaved final : public YmStreamEncoder
{
public:
    /**
     * Constructor.
     * @param song the Song to read.
     * @param subsongId the ID of the Subsong to read. Must be valid.
     * @param psgIndex the index of the unique PSG to get the data from. Must be valid.
     * @param isYm3 true if YM3, false if YM6.
     * @param targetPsgFrequency the possibly forced target PSG frequency.
     */
    YmStreamEncoderNonInterleaved(std::shared_ptr<const Song> song, Id subsongId, int psgIndex, bool isYm3, OptionalInt targetPsgFrequency) noexcept;

    // YmStreamEncoder method implementations.
    // ==========================================
    void generateStream(juce::OutputStream& outputStream, int iterationCount, const Location& startLocation, const Location& pastEndLocation, int digiChannel) noexcept override;
};

}   // namespace arkostracker
