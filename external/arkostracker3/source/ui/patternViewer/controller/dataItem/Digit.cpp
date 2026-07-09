#include "Digit.h"

namespace arkostracker 
{

Digit::Digit() noexcept:
        digitIndex(0),
        digitValue(0)
{
}

Digit::Digit(int pDigitIndex, int pDigitValue) noexcept:
        digitIndex(pDigitIndex),
        digitValue(pDigitValue)
{
}


}   // namespace arkostracker

