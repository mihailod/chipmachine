#include "TrailingEffectContext.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

TrailingEffectContext::TrailingEffectContext() noexcept :
        effectToValue(),
        currentInstrumentIndex()
{
}

std::pair<TrailingEffect, int> TrailingEffectContext::declareEffect(const TrailingEffect effect, const int value) noexcept
{
    switch (effect) {
        case TrailingEffect::noEffect:
            // If current effect is a pitch, and the value is still "on", it means we can stop it.
            // We don't do the same for the hardware effect, it would "generate" too many instruments.
            if (getEffectValue(TrailingEffect::pitchUp).isPresent()) {
                invalidateEffect(TrailingEffect::pitchUp);
                return { TrailingEffect::pitchUp, 0 };
            }
            if (getEffectValue(TrailingEffect::pitchDown).isPresent()) {
                invalidateEffect(TrailingEffect::pitchDown);
                return { TrailingEffect::pitchDown, 0 };
            }

            return { TrailingEffect::noEffect, 0 };

        case TrailingEffect::reset:
            effectToValue.clear();
            setEffectAndValue(TrailingEffect::volume, 15);
            return { TrailingEffect::reset, 0 };

        case TrailingEffect::volume:
            return onTrailingEffect(TrailingEffect::volume, value);
        case TrailingEffect::arpeggioTable:
            return onTrailingEffect(TrailingEffect::arpeggioTable, value);
        case TrailingEffect::arpeggio3Notes:
            return onTrailingEffect(TrailingEffect::arpeggio3Notes, value);
        case TrailingEffect::pitchUp:
            return onPitchFound(true, value);
        case TrailingEffect::pitchDown:
            return onPitchFound(false, value);

        default:
            jassertfalse;           // Not managed?
        case TrailingEffect::hardwareEnvelope:
            // No change.
            return { effect, value };
    }
}

std::pair<TrailingEffect, int> TrailingEffectContext::onTrailingEffect(TrailingEffect trailingEffect, int value) noexcept
{
    // Don't encode the effect if it has the same value.
    if (getEffectValue(trailingEffect) == value) {
        return { TrailingEffect::noEffect, 0 };
    }
    setEffectAndValue(trailingEffect, value);
    return { trailingEffect, value };
}

std::pair<TrailingEffect, int> TrailingEffectContext::onPitchFound(bool up, int value) noexcept
{
    const auto effect = up ? TrailingEffect::pitchUp : TrailingEffect::pitchDown;

    // Clears the opposite effect. A pitch up will invalidate a pitch down, and vice-versa.
    invalidateEffect(up ? TrailingEffect::pitchDown : TrailingEffect::pitchUp);

    // Encodes it, unless a possible Pitch is present, with a different value.
    return onTrailingEffect(effect, value);
}

OptionalInt TrailingEffectContext::getCurrentVolume() const noexcept
{
    return getEffectValue(TrailingEffect::volume);
}

void TrailingEffectContext::invalidateEffect(TrailingEffect effect) noexcept
{
    effectToValue.erase(effect);
}

void TrailingEffectContext::declareNote() noexcept
{
    invalidateEffect(TrailingEffect::pitchUp);
    invalidateEffect(TrailingEffect::pitchDown);
    invalidateEffect(TrailingEffect::hardwareEnvelope);
}

OptionalInt TrailingEffectContext::getEffectValue(TrailingEffect effect) const noexcept
{
    auto iterator = effectToValue.find(effect);
    if (iterator == effectToValue.cend()) {
        return { };
    }

    return iterator->second;
}

void TrailingEffectContext::setEffectAndValue(TrailingEffect effect, int value) noexcept
{
    effectToValue[effect] = value;
}

OptionalInt TrailingEffectContext::getCurrentInstrumentIndex() const noexcept
{
    return currentInstrumentIndex;
}

void TrailingEffectContext::setCurrentInstrumentIndex(OptionalInt instrumentIndex) noexcept
{
    currentInstrumentIndex = instrumentIndex;
}

bool TrailingEffectContext::isEffectPresent(TrailingEffect effect) noexcept
{
    auto it = effectToValue.find(effect);
    if (it == effectToValue.cend()) {
        return false;
    }
    return it->second.isPresent() && (it->second.getValue() != 0);
}


}   // namespace arkostracker

