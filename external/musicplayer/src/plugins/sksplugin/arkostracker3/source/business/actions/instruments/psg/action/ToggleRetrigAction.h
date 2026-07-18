#pragma once

#include "../../../../../song/instrument/psg/PsgInstrumentCell.h"

namespace arkostracker
{

/** Functor to change toggle the Retrig state of a Cell. */
class ToggleRetrigAction
{
public:
    PsgInstrumentCell operator()(int iterationIndex, const PsgInstrumentCell& cell) noexcept;
};

}   // namespace arkostracker
