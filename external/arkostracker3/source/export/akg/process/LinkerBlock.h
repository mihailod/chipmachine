#pragma once

#include "../../../song/subsong/Pattern.h"
#include "AkgPattern.h"

namespace arkostracker 
{

/**
 * Block from the Linker which holds all the elements from a Pattern, EXCEPT the tracks, as they are encoded separately in the Linker.
 * This class is useful to encode each uniquely.
 */
class LinkerBlock
{
public:
    /**
        Constructor.
        @param pattern the Pattern used to populate the data of the LinkerBlock. Everything is used, except for the Tracks indexes.
    */
    LinkerBlock(AkgPattern pattern) noexcept;

    /** Equality operator. */
    bool operator==(const LinkerBlock& other) const noexcept;
    /** Inequality operator. */
    bool operator!=(const LinkerBlock& other) const noexcept;

    /** Returns the height of the Pattern. */
    int getHeight() const noexcept;
    /** Returns the transposition related to the given channel index. */
    int getTransposition(int channelIndex) const noexcept;
    /** Returns the Speed Track index. */
    int getSpeedTrackIndex() const noexcept;
    /** Returns the Event Track index. */
    int getEventTrackIndex() const noexcept;

private:
    AkgPattern originalPattern;        // The Pattern used to populate this LinkerBlock.
};

}   // namespace arkostracker

