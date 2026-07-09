#pragma once

#include <vector>

#include "Tip.h"

namespace arkostracker
{

/** Contains all the Tips. */
class Tips
{
public:
    /** @return all the tips. */
    static const std::vector<Tip>& getTips() noexcept;
};

}   // namespace arkostracker
