#include "CellContext.h"

#include "../../../../../utils/PsgValues.h"

namespace arkostracker
{
void CellContext::invalidateEffect(const Effect effect) noexcept
{
    switch (effect) {
        case Effect::pitchUp:
            effectPitchUpValue = { };
            break;
        case Effect::pitchDown:
            effectPitchDownValue = { };
            break;
        case Effect::pitchGlide:
            effectPitchGlideValue = { };
            break;
        case Effect::fastPitchUp:
            effectFastPitchUpValue = { };
            break;
        case Effect::fastPitchDown:
            effectFastPitchDownValue = { };
            break;
        case Effect::volume:
            currentVolume = { };
            break;
        case Effect::volumeIn:
            effectVolumeInValue = { };
            break;
        case Effect::volumeOut:
            effectVolumeOutValue = { };
            break;
        case Effect::forceInstrumentSpeed:
            effectForceInstrumentSpeedValue = { };
            break;
        case Effect::forceArpeggioSpeed:
            effectForceArpeggioSpeedValue = { };
            break;
        case Effect::forcePitchTableSpeed:
            effectForcePitchTableSpeedValue = { };
            break;
        case Effect::arpeggioTable:
            effectArpeggioTableValue = { };
            break;
        case Effect::arpeggio3Notes:
            effectArpeggio3NotesValue = { };
            break;
        case Effect::arpeggio4Notes:
            effectArpeggio4NotesValue = { };
            break;
        case Effect::pitchTable:
            effectPitchTableValue = { };
            break;
        case Effect::reset:
        case Effect::legato:
        case Effect::noEffect:
        default:
            jassertfalse; // Not supposed to happen.
            break;
    }
}

void CellContext::invalidateEffects(const std::set<Effect>& effects) noexcept
{
    for (const auto& effect : effects) {
        invalidateEffect(effect);
    }
}

void CellContext::setEffectValue(const Effect effect, const int value) noexcept
{
    switch (effect) {
        case Effect::pitchUp:
            effectPitchUpValue = value;
            break;
        case Effect::pitchDown:
            effectPitchDownValue = value;
            break;
        case Effect::pitchGlide:
            effectPitchGlideValue = value;
            break;
        case Effect::fastPitchUp:
            effectFastPitchUpValue = value;
            break;
        case Effect::fastPitchDown:
            effectFastPitchDownValue = value;
            break;
        case Effect::volumeIn:
            effectVolumeInValue = value;
            break;
        case Effect::volumeOut:
            effectVolumeOutValue = value;
            break;
        case Effect::volume:
            currentVolume = value;
            break;
        case Effect::forceInstrumentSpeed:
            effectForceInstrumentSpeedValue = value;
            break;
        case Effect::forceArpeggioSpeed:
            effectForceArpeggioSpeedValue = value;
            break;
        case Effect::forcePitchTableSpeed:
            effectForcePitchTableSpeedValue = value;
            break;
        case Effect::arpeggioTable:
            effectArpeggioTableValue = value;
            break;
        case Effect::arpeggio3Notes:
            effectArpeggio3NotesValue = value;
            break;
        case Effect::arpeggio4Notes:
            effectArpeggio4NotesValue = value;
            break;
        case Effect::pitchTable:
            effectPitchTableValue = value;
            break;
        case Effect::reset:
        case Effect::legato:
        case Effect::noEffect:
        default:
            jassertfalse; // Not supposed to happen.
            break;
    }
}

void CellContext::resetEffects(const std::set<Effect>& effects) noexcept
{
    for (const auto& effect : effects) {
        resetEffect(effect);
    }
}

void CellContext::resetEffect(const Effect effect) noexcept
{
    switch (effect) {
        case Effect::pitchUp:
            effectPitchUpValue = 0;
            break;
        case Effect::pitchDown:
            effectPitchDownValue = 0;
            break;
        case Effect::pitchGlide:
            effectPitchGlideValue = 0;
            break;
        case Effect::fastPitchUp:
            effectFastPitchUpValue = 0;
            break;
        case Effect::fastPitchDown:
            effectFastPitchDownValue = 0;
            break;
        case Effect::volumeIn:
            effectVolumeInValue = 0;
            break;
        case Effect::volumeOut:
            effectVolumeOutValue = 0;
            break;
        case Effect::forceInstrumentSpeed:
            effectForceInstrumentSpeedValue = 0;
            break;
        case Effect::forceArpeggioSpeed:
            effectForceArpeggioSpeedValue = 0;
            break;
        case Effect::forcePitchTableSpeed:
            effectForcePitchTableSpeedValue = 0;
            break;
        case Effect::arpeggioTable:
            effectArpeggioTableValue = 0;
            break;
        case Effect::arpeggio3Notes:
            effectArpeggio3NotesValue = 0;
            break;
        case Effect::arpeggio4Notes:
            effectArpeggio4NotesValue = 0;
            break;
        case Effect::pitchTable:
            effectPitchTableValue = 0;
            break;
        case Effect::reset:
        case Effect::legato:
        case Effect::noEffect:
        case Effect::volume:
        default:
            jassertfalse; // Not supposed to happen.
            break;
    }
}

void CellContext::setVolume(const int volume) noexcept
{
    jassert((volume >= PsgValues::minimumVolume) && (volume <= PsgValues::maximumVolumeNoHard));
    currentVolume = volume;
}

OptionalInt CellContext::getCurrentVolume() const noexcept
{
    return currentVolume;
}

void CellContext::setPitchUp(const int value) noexcept
{
    effectPitchUpValue = value;
}

OptionalInt CellContext::getCurrentPitchUp() const noexcept
{
    return effectPitchUpValue;
}

OptionalInt CellContext::getCurrentPitchValueFromEffect(const Effect effect) const noexcept
{
    switch (effect) {
        case Effect::pitchUp:
            return effectPitchUpValue;
        case Effect::pitchDown:
            return effectPitchDownValue;
        case Effect::pitchGlide:
            return effectPitchGlideValue;
        case Effect::fastPitchUp:
            return effectFastPitchUpValue;
        case Effect::fastPitchDown:
            return effectFastPitchDownValue;
        case Effect::volume:
        case Effect::volumeIn:
        case Effect::volumeOut:
        case Effect::noEffect:
        case Effect::pitchTable:
        case Effect::arpeggioTable:
        case Effect::arpeggio3Notes:
        case Effect::arpeggio4Notes:
        case Effect::reset:
        case Effect::forceInstrumentSpeed:
        case Effect::forceArpeggioSpeed:
        case Effect::forcePitchTableSpeed:
        case Effect::legato:
        default:
            jassertfalse; // Not supposed to happen.
            return { };
    }
}

OptionalInt CellContext::getCurrentVolumeValueFromEffect(const Effect effect) const noexcept
{
    switch (effect) {
        case Effect::volume:
            return currentVolume;
        case Effect::volumeIn:
            return effectVolumeInValue;
        case Effect::volumeOut:
            return effectVolumeOutValue;
        case Effect::noEffect:
        case Effect::pitchUp:
        case Effect::pitchDown:
        case Effect::pitchGlide:
        case Effect::pitchTable:
        case Effect::arpeggioTable:
        case Effect::arpeggio3Notes:
        case Effect::arpeggio4Notes:
        case Effect::reset:
        case Effect::forceInstrumentSpeed:
        case Effect::forceArpeggioSpeed:
        case Effect::forcePitchTableSpeed:
        case Effect::fastPitchUp:
        case Effect::fastPitchDown:
        case Effect::legato:
        default:
            jassertfalse; // Not supposed to happen.
            return { };
    }
}

OptionalInt CellContext::getCurrentSpeedValueFromEffect(const Effect effect) const noexcept
{
    switch (effect) {
        case Effect::forceInstrumentSpeed:
            return effectForceInstrumentSpeedValue;
        case Effect::forceArpeggioSpeed:
            return effectForceArpeggioSpeedValue;
        case Effect::forcePitchTableSpeed:
            return effectForcePitchTableSpeedValue;
        case Effect::volume:
        case Effect::volumeIn:
        case Effect::volumeOut:
        case Effect::noEffect:
        case Effect::pitchUp:
        case Effect::pitchDown:
        case Effect::pitchGlide:
        case Effect::pitchTable:
        case Effect::arpeggioTable:
        case Effect::arpeggio3Notes:
        case Effect::arpeggio4Notes:
        case Effect::reset:
        case Effect::fastPitchUp:
        case Effect::fastPitchDown:
        case Effect::legato:
        default:
            jassertfalse; // Not supposed to happen.
            return { };
    }
}

OptionalInt CellContext::getCurrentArpeggioValueFromEffect(const Effect effect) const noexcept
{
    switch (effect) {
        case Effect::arpeggioTable:
            return effectArpeggioTableValue;
        case Effect::arpeggio3Notes:
            return effectArpeggio3NotesValue;
        case Effect::arpeggio4Notes:
            return effectArpeggio4NotesValue;
        case Effect::forceInstrumentSpeed:
        case Effect::forceArpeggioSpeed:
        case Effect::forcePitchTableSpeed:
        case Effect::volume:
        case Effect::volumeIn:
        case Effect::volumeOut:
        case Effect::noEffect:
        case Effect::pitchUp:
        case Effect::pitchDown:
        case Effect::pitchGlide:
        case Effect::pitchTable:
        case Effect::reset:
        case Effect::fastPitchUp:
        case Effect::fastPitchDown:
        case Effect::legato:
        default:
            jassertfalse; // Not supposed to happen.
            return { };
    }
}

OptionalInt CellContext::getCurrentPitchTableValue() const noexcept
{
    return effectPitchTableValue;
}

} // namespace arkostracker
