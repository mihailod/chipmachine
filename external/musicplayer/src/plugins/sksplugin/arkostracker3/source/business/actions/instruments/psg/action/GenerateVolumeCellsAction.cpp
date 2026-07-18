#include "GenerateVolumeCellsAction.h"

namespace arkostracker
{

GenerateVolumeCellsAction::GenerateVolumeCellsAction(const int pStartVolume, const double pStep) noexcept :
        startVolume(static_cast<double>(pStartVolume)),
        step(pStep),
        currentVolume(startVolume)
{
    jassert(!juce::exactlyEqual(step, 0.0));
}

PsgInstrumentCell GenerateVolumeCellsAction::operator()(const int iterationIndex, const PsgInstrumentCell& /*cell*/) noexcept
{
    // Resets the curve.
    if (iterationIndex == 0) {
        currentVolume = startVolume;
    }

    const auto cell = PsgInstrumentCell::buildSoftwareCell(static_cast<int>(currentVolume));

    currentVolume += step;

    return cell;
}

}   // namespace arkostracker
