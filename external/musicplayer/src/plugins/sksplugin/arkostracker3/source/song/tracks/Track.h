#pragma once

#include "../cells/Cell.h"
#include "AbstractTrack.h"

namespace arkostracker 
{

/** A Track for "music" Tracks: Tracks with notes. */
using Track = AbstractTrack<Cell>;

}   // namespace arkostracker

