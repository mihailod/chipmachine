#pragma once

namespace arkostracker
{

/** Constants about the Cells. */
class SpecialCellConstants
{
public:
    /** Prevents instantiation.*/
    SpecialCellConstants() = delete;

    static constexpr auto cellNoValue = 0;         // Sentinel value indicating there is no value.
    static constexpr auto minimumValueExceptEmpty = cellNoValue + 1;
    static constexpr auto maximumValue = 0xff;
};

}   // namespace arkostracker

