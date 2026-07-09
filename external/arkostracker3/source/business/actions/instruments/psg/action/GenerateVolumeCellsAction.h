#pragma once

#include "../../../../../song/instrument/psg/PsgInstrumentCell.h"

namespace arkostracker
{

/** Functor to generate software Cells with volume change. */
class GenerateVolumeCellsAction
{
public:
    /**
     * Constructor.
     * @param startVolume the start volume (0 or 15 for example).
     * @param step how much to increase every time (1 or -1 for example).
     */
    GenerateVolumeCellsAction(int startVolume, double step) noexcept;

    PsgInstrumentCell operator()(int iterationIndex, const PsgInstrumentCell& cell) noexcept;

private:
    const double startVolume;
    const double step;

    double currentVolume;
};

}   // namespace arkostracker
