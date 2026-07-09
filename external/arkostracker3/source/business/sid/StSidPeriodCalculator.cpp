#include "StSidPeriodCalculator.h"

#include "../../utils/NumberUtil.h"

namespace arkostracker
{

std::pair<int, int> StSidPeriodCalculator::calculate(int period) noexcept
{
    period = NumberUtil::correctNumber(period, 1, 0xfff);

    // That black magic comes from ggn 68000 code.

    static const std::map<int, std::pair<int, int>> periodToDivisorAndPredivisor = {
        { 0x68, { 4, 1 } },
        { 0x105, { 0xa, 2 } },
        { 0x1a2, { 0x10, 3 } },
        { 0x51a, { 0x32, 4 } },
        { 0x688, { 0x40, 5 } },
        { 0xa35, { 0x64, 6 } },
        //{ 0xeef, { 0xc8, 7 } },
        { 0x1000, { 0xc8, 7 } },
    };

    // Finds the line which period is above the input period.
    auto predivisor = 0;
    auto divisor = 0;
    for (const auto& [foundPeriod, stuff] : periodToDivisorAndPredivisor) {
        if (period < foundPeriod) {
            predivisor = stuff.second;
            divisor = stuff.first;
            break;
        }
    }
    jassert(predivisor > 0);

    auto value = static_cast<unsigned int>(160822 / divisor);
    value = value * static_cast<unsigned int>(period);
    value += 0x2000;
    value = value << 2U;
    value = value >> 16U;

    return { predivisor, value };
}

}   // namespace arkostracker