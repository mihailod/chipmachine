#pragma once

#include "YmDecoder123.h"

namespace arkostracker 
{

/**
 * Decoder of the header for YM 3b. The only difference with 3 is the loop encoded at the end.
 * The implementation for the YM 1, 2, 3 is used, only the header reading is different.
 */
class YmDecoder3b final : public YmDecoder123
{
public:
    /**
    * Constructor.
    * @param inputStream the Stream of the YM.
    */
    explicit YmDecoder3b(juce::InputStream& inputStream) noexcept;

    // Ym123Decoder method overrides.
    // ===================================================
    bool acceptFormatHeader() noexcept override;
    YmMetadata parseHeader(bool& success) noexcept override;

protected:
    int getIterationCount() const noexcept override;
    int getFirstFrameDataIndex() const noexcept override;

private:
    int iterationCount;
    int firstFrameDataIndex;
};

}   // namespace arkostracker
