#include "StartEnd.h"

#include <utility>

namespace arkostracker 
{

StartEnd::StartEnd(Location pStartLocation, Location pEndLocation) :
        startLocation(std::move(pStartLocation)),
        endLocation(std::move(pEndLocation))
{
}

Location StartEnd::getStartLocation() const
{
    return startLocation;
}

Location StartEnd::getEndLocation() const
{
    return endLocation;
}


}   // namespace arkostracker

