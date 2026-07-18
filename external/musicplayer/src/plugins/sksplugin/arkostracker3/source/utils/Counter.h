#pragma once

namespace arkostracker 
{

/**
 * An ultra-simple mean to have an identifier as an unsigned int. Does NOT start at 0 (so that objects can init their counter to 0) and increases every time.
 * Can be used through all the application.
 * This is NOT thread safe.
 */
class Counter
{
public:
    /** Prevents instantiation. */
    Counter() = delete;

    /** @return the next counter (does not start at 0). */
    static unsigned int getNextCounter() noexcept;

private:
    static unsigned int nextValue; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};


}   // namespace arkostracker

