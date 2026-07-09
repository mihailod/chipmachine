#pragma once

#include "../../../../song/CellLocationInPosition.h"

namespace arkostracker
{

class Song;

/** Browses the whole song for Cells in error. */
class ErrorChecker
{
public:
    /** Prevents instantiation. */
    ErrorChecker() = delete;

    /** Finds all the errors in the whole Songs, and the error messages. */
    static std::vector<std::pair<CellLocationInPosition, juce::String>> findErrors(const Song& song) noexcept;

private:

};

}   // namespace arkostracker
