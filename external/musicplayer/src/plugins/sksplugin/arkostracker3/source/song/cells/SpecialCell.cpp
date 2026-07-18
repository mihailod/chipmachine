#include "SpecialCell.h"

#include <juce_core/juce_core.h>

#include "SpecialCellConstants.h"

namespace arkostracker 
{

SpecialCell::SpecialCell() noexcept :
        value(SpecialCellConstants::cellNoValue)
{
}

SpecialCell::SpecialCell(const int pValue) noexcept :
        value(pValue)
{
}

SpecialCell SpecialCell::buildEmptySpecialCell() noexcept
{
    return SpecialCell(SpecialCellConstants::cellNoValue);
}

SpecialCell SpecialCell::buildSpecialCell(const int value) noexcept
{
    jassert((value >= 0) && (value <= 255));

    return SpecialCell(value);
}

bool SpecialCell::isEmpty() const noexcept
{
    return (value == SpecialCellConstants::cellNoValue);
}

int SpecialCell::getValue() const noexcept
{
    return value;
}

SpecialCell SpecialCell::mix(const SpecialCell& overwritingSpecialCell) const noexcept
{
    // If the overwriting Cell is empty, don't use it.
    if (overwritingSpecialCell.isEmpty()) {
        return *this;
    }

    return overwritingSpecialCell;
}

void SpecialCell::clear() noexcept
{
    value = SpecialCellConstants::cellNoValue;
}

bool SpecialCell::operator==(const SpecialCell& cell) const
{
    return (value == cell.value);
}

bool SpecialCell::operator!=(const SpecialCell& cell) const
{
    return !operator==(cell);
}

}   // namespace arkostracker
