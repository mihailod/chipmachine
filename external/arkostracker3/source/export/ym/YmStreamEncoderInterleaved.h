#pragma once

#include "YmStreamEncoder.h"
#include "YmStreamEncoderNonInterleaved.h"

namespace arkostracker 
{

/** Implementation of an YM Stream Encoder, for an interleaved format. This must be done in 14 passes, unless a memory buffer is used! */
class YmStreamEncoderInterleaved final : public YmStreamEncoder
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
    YmStreamEncoderInterleaved(const std::shared_ptr<const Song>& song, const Id& subsongId, int psgIndex, bool isYm3, OptionalInt targetPsgFrequency) noexcept;

    // YmStreamEncoder method implementations.
    // ==========================================
    void generateStream(juce::OutputStream& outputStream, int iterationCount, const Location& startLocation, const Location& pastEndLocation, int digiChannel) noexcept override;

private:
    YmStreamEncoderNonInterleaved ymStreamEncoderNonInterleaved;
};

}   // namespace arkostracker
