#include "../../utils/OptionalValue.h"
#include "Dimension.h"

namespace arkostracker 
{

const int Dimension::minimumValue = 50;

Dimension::Dimension(const OptionalInt& pDesiredSize, int pMinimum, int pMaximum) noexcept :
        minimum(pMinimum),
        maximum(pMaximum),
        desiredSize(pDesiredSize)
{
}

Dimension Dimension::buildFree() noexcept
{
    return Dimension(OptionalInt());
}

Dimension Dimension::buildLocked(int height) noexcept
{
    return Dimension(height, height, height);
}

int Dimension::getMinimum() const noexcept
{
    return minimum;
}

int Dimension::getMaximum() const noexcept
{
    return maximum;
}

const OptionalInt& Dimension::getDesiredSize() const noexcept
{
    return desiredSize;
}

bool Dimension::isLocked() const noexcept
{
    return desiredSize.isPresent() && (minimum == maximum) && (desiredSize.getValue() == minimum);
}

bool Dimension::operator==(const Dimension& rhs) const
{
    return (minimum == rhs.minimum) &&
           (maximum == rhs.maximum) &&
           (desiredSize == rhs.desiredSize);
}

bool Dimension::operator!=(const Dimension& rhs) const
{
    return !(rhs == *this);
}


}   // namespace arkostracker

