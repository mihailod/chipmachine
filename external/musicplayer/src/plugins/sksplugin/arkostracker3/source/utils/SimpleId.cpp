#include "SimpleId.h"

namespace arkostracker 
{

std::atomic_int SimpleId::nextValue(0);         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

int SimpleId::getNextIdentifier() noexcept
{
    return nextValue.fetch_add(1);
}


}   // namespace arkostracker

