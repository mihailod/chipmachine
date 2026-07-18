#pragma once

namespace arkostracker 
{

/** Simple generic holder of the data of an digit index and digit value. */
class Digit
{
public:
    /** Default constructor, useful for Optionals. */
    Digit() noexcept;

    /**
     * Constructor.
     * @param digitIndex the index of the digit.
     * @param digitValue the digit value (0-15).
     */
    Digit(int digitIndex, int digitValue) noexcept;

    const int digitIndex;
    const int digitValue;
};



}   // namespace arkostracker

