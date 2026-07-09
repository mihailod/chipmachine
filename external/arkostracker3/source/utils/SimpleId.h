#pragma once

#include <atomic>

namespace arkostracker 
{

/**
 * An ultra-simple mean to have an identifier as an Int. Starts at 0 and increases every time.
 * Can be used through all the application.
 * This is thread safe.
 */
class SimpleId
{
public:
    /** Prevents instantiation. */
    SimpleId() = delete;

    /** @return the next identifier (starts at 0). */
    static int getNextIdentifier() noexcept;

private:
    static std::atomic_int nextValue; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};


}   // namespace arkostracker

