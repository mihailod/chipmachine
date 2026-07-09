#pragma once

#include "YmDecoder.h"

namespace arkostracker 
{

/**
 * Decoder of the header for YM 1, 2 and 3 (YM1 has not been released it seems and is undocumented, but I include it anyway here!).
 * There seems to be no difference between YM 2 and 3, except the header tag.
 */
class YmDecoder123 : public YmDecoder
{
public:
    /**
    * Constructor.
    * @param inputStream the Stream of the YM.
    */
    explicit YmDecoder123(juce::InputStream& inputStream) noexcept;

    // YmDecoder method implementations.
    // ===================================================
    bool acceptFormatHeader() noexcept override;
    YmMetadata parseHeader(bool& success) noexcept override;
    int getEncodedRegisterCountInFrame() const noexcept override;

protected:
    static constexpr auto tagSize = 4;                   // The size of the header tag.
    static constexpr auto registerCount = 14;            // How many registers are encoded in a frame. Only registers from 0-13 are encoded.

    bool isInterleaved() const noexcept override;
    int getPostMusicFramesDataSize() const noexcept override;
    int getIterationCount() const noexcept override;
    int getFirstFrameDataIndex() const noexcept override;

private:
    int iterationCount;
    int firstFrameDataIndex;
};

}   // namespace arkostracker
