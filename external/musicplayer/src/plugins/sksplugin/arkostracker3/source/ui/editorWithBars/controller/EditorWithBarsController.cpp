#include "EditorWithBarsController.h"

#include "../../../utils/PsgValues.h"

namespace arkostracker
{

int EditorWithBarsController::getDefaultNoiseValue() noexcept
{
    return 0;
}

int EditorWithBarsController::getDefaultPeriodValue() noexcept
{
    return 0;
}

int EditorWithBarsController::getDefaultPitchValue() noexcept
{
    return 0;
}

int EditorWithBarsController::getDefaultArpeggioOctaveValue() noexcept
{
    return 0;
}

int EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue() noexcept
{
    return 0;
}

PsgInstrumentCellLink EditorWithBarsController::getDefaultSoundTypeValue() noexcept
{
    return PsgInstrumentCellLink::softOnly;
}

int EditorWithBarsController::getDefaultVolumeValue() noexcept
{
    return 0;
}

int EditorWithBarsController::getDefaultHardwareEnvelopeValue() noexcept
{
    return PsgValues::defaultHardwareEnvelope;
}

int EditorWithBarsController::getDefaultHardwareRatioValue() noexcept
{
    return PsgValues::defaultRatio;
}

bool EditorWithBarsController::getDefaultSidIsActivated() noexcept
{
    return false;
}

int EditorWithBarsController::getDefaultSidBalancePercent() noexcept
{
    return PsgValues::defaultSidBalancePercent;
}

int EditorWithBarsController::getDefaultSidLowShelfVolume() noexcept
{
    return PsgValues::defaultSidLowShelfVolume;
}

int EditorWithBarsController::getDefaultSidArpeggio() noexcept
{
    return PsgValues::defaultSidArpeggio;
}

int EditorWithBarsController::getDefaultSidPitch() noexcept
{
    return PsgValues::defaultSidPitch;
}

}   // namespace arkostracker
