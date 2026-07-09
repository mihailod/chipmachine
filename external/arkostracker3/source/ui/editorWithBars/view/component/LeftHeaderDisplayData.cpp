#include "LeftHeaderDisplayData.h"

namespace arkostracker 
{

LeftHeaderDisplayData::LeftHeaderDisplayData(const bool pPlusEnabled, const bool pMinusEnabled, const OptionalBool pAutoSpreadOn, const bool pHidden) noexcept :
        plusEnabled(pPlusEnabled),
        minusEnabled(pMinusEnabled),
        autoSpreadOn(pAutoSpreadOn),
        hidden(pHidden)
{
}

LeftHeaderDisplayData::LeftHeaderDisplayData() noexcept :
        plusEnabled(false),
        minusEnabled(false),
        autoSpreadOn(),
        hidden()
{
}

bool LeftHeaderDisplayData::isPlusEnabled() const noexcept
{
    return plusEnabled;
}

bool LeftHeaderDisplayData::isMinusEnabled() const noexcept
{
    return minusEnabled;
}

const OptionalBool& LeftHeaderDisplayData::getAutoSpreadOn() const noexcept
{
    return autoSpreadOn;
}

bool LeftHeaderDisplayData::isHidden() const noexcept
{
    return hidden;
}

bool LeftHeaderDisplayData::operator==(const LeftHeaderDisplayData& rhs) const noexcept
{
    return plusEnabled == rhs.plusEnabled &&
           minusEnabled == rhs.minusEnabled &&
           autoSpreadOn == rhs.autoSpreadOn &&
           hidden == rhs.hidden;
}

bool LeftHeaderDisplayData::operator!=(const LeftHeaderDisplayData& rhs) const noexcept
{
    return !(rhs == *this);
}

}   // namespace arkostracker
