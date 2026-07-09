#pragma once

#include "../../song/Location.h"

namespace arkostracker 
{

/** Simple holder of a start and an end Location. */
class StartEnd
{
public:
    /**
     * Constructor.
     * @param startLocation the start position.
     * @param endLocation the end position.
     */
    StartEnd(Location startLocation, Location endLocation);

    /** @return the start position. */
    Location getStartLocation() const;
    /** @return the start position. */
    Location getEndLocation() const;

private:
    Location startLocation;
    Location endLocation;
};

}   // namespace arkostracker

