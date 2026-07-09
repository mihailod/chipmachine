#pragma once

#include "YmDecoder.h"

namespace arkostracker 
{

/** Decoder of the header for YM 4, 5 and 6, because there are only a few differences. */
class YmDecoder456 final : public YmDecoder
{
public:

    /**
     * Constructor.
     * @param inputStream the Stream of the YM.
     */
    explicit YmDecoder456(juce::InputStream& inputStream) noexcept;

    // YmDecoder method implementations.
    // ===================================================
    bool acceptFormatHeader() noexcept override;
    YmMetadata parseHeader(bool& success) noexcept override;
    int getEncodedRegisterCountInFrame() const noexcept override;

protected:
    static constexpr auto tagSize = 12;                  // The size of the header tag.

    bool isInterleaved() const noexcept override;
    int getPostMusicFramesDataSize() const noexcept override;
    int getIterationCount() const noexcept override;
    int getFirstFrameDataIndex() const noexcept override;

private:
    /**
     * Reads the digidrum data.
     * @param digidrumCount how many digidrums there are.
     * @param ymMetadata the YM Metadata to fill.
     * @param areSignedSamples true if the samples are signed.
     * @param are4BitsSamples true if the samples are 4 bits only.
     * @return true if everything went fine.
     */
    bool readDigidrums(int digidrumCount, YmMetadata& ymMetadata, bool areSignedSamples, bool are4BitsSamples) const noexcept;

    bool interleaved;                               // True if the encoding of the registers is interleaved or not.
    int iterationCount;                             // The iteration count.
    int firstFrameDataIndex;
};

}   // namespace arkostracker
