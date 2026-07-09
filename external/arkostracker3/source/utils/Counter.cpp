#include "Counter.h"

namespace arkostracker 
{

unsigned int Counter::nextValue(10000);         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

unsigned int Counter::getNextCounter() noexcept
{
    return ++nextValue;
}


}   // namespace arkostracker

