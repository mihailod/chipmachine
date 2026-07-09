#pragma once

#include "AbstractTrack.h"
#include "../cells/SpecialCell.h"

namespace arkostracker 
{

/** Represents both the Speed and Event Track. */
using SpecialTrack = AbstractTrack<SpecialCell>;

}   // namespace arkostracker

