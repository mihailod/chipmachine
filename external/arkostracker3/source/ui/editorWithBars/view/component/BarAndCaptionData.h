#pragma once

#include "BarCaptionDisplayedData.h"
#include "BarData.h"

namespace arkostracker 
{

/** Simple holder of a Bar and Caption data. */
class BarAndCaptionData {
public:
    BarAndCaptionData(const BarData& pBarData, BarCaptionDisplayedData pCaptionData) noexcept :
            barData(pBarData),
            captionData(std::move(pCaptionData))
    {
    }

    const BarData& getBarData() const noexcept
    {
        return barData;
    }

    const BarCaptionDisplayedData& getCaptionData() const noexcept
    {
        return captionData;
    }

private:
    BarData barData;
    BarCaptionDisplayedData captionData;
};

}   // namespace arkostracker
