#include "Effect.h"

namespace arkostracker
{

/**
 * Indicates whether the given effect is a Pitch ((fast)up/down).
 * @param effectNumber the effect number.
 */
bool EffectUtil::isPitchEffect(const int effectNumber) noexcept {
    switch (effectNumber) {
        case static_cast<int>(Effect::pitchUp):
        case static_cast<int>(Effect::pitchDown):
        case static_cast<int>(Effect::fastPitchUp):
        case static_cast<int>(Effect::fastPitchDown):
            return true;
        default:
            return false;
    }
}

/** @return how many digits are used for the given effect. (1 for volume, 2 for arp table, etc.). From 0 to 3, included. */
int EffectUtil::getDigitCount(const Effect effect) noexcept {
    switch (effect) {
        default:
            jassertfalse;           // Unknown effect??
        case Effect::noEffect:
            return 0;
        case Effect::volume:
        case Effect::reset:
            return 1;
        case Effect::pitchTable:
        case Effect::arpeggioTable:
        case Effect::arpeggio3Notes:
        case Effect::forceArpeggioSpeed:
        case Effect::forcePitchTableSpeed:
        case Effect::forceInstrumentSpeed:
        case Effect::legato:
            return 2;
        case Effect::volumeIn:
        case Effect::volumeOut:
        case Effect::pitchUp:
        case Effect::pitchDown:
        case Effect::pitchGlide:
        case Effect::fastPitchUp:
        case Effect::fastPitchDown:
        case Effect::arpeggio4Notes:
            return 3;
    }
}

std::pair<int, int> EffectUtil::getMinMaxValues(const Effect effect) noexcept {
    const auto digitCount = getDigitCount(effect);
    const auto maxValue = (1U << (4U * static_cast<unsigned int>(digitCount))) - 1;
    return { 0, maxValue };
}

juce::String EffectUtil::toSerializedString(const Effect effect) noexcept
{
    switch (effect) {
        case Effect::legato:        // Never encoded.
        default:
            jassertfalse;           // Not supposed to happen.
        case Effect::noEffect:
            return "noEffect";

        case Effect::pitchUp:
            return "pitchUp";
        case Effect::pitchDown:
            return "pitchDown";
        case Effect::pitchGlide:
            return "pitchGlide";
        case Effect::pitchTable:
            return "pitchTable";
        case Effect::volume:
            return "volume";
        case Effect::volumeIn:
            return "volumeIn";
        case Effect::volumeOut:
            return "volumeOut";
        case Effect::arpeggioTable:
            return "arpeggioTable";
        case Effect::arpeggio3Notes:
            return "arpeggio3Notes";
        case Effect::arpeggio4Notes:
            return "arpeggio4Notes";
        case Effect::reset:
            return "reset";
        case Effect::forceInstrumentSpeed:
            return "forceInstrumentSpeed";
        case Effect::forceArpeggioSpeed:
            return "forceArpeggioSpeed";
        case Effect::forcePitchTableSpeed:
            return "forcePitchTableSpeed";
        case Effect::fastPitchUp:
            return "fastPitchUp";
        case Effect::fastPitchDown:
            return "fastPitchDown";
    }
}

OptionalValue<Effect> EffectUtil::fromSerializedString(const juce::String& string) noexcept
{
    if (string == "noEffect") {
        return Effect::noEffect;
    }
    if (string == "pitchUp") {
        return Effect::pitchUp;
    }
    if (string == "pitchDown") {
        return Effect::pitchDown;
    }
    if (string == "pitchGlide") {
        return Effect::pitchGlide;
    }
    if (string == "pitchTable") {
        return Effect::pitchTable;
    }
    if (string == "volume") {
        return Effect::volume;
    }
    if (string == "volumeIn") {
        return Effect::volumeIn;
    }
    if (string == "volumeOut") {
        return Effect::volumeOut;
    }
    if (string == "arpeggioTable") {
        return Effect::arpeggioTable;
    }
    if (string == "arpeggio3Notes") {
        return Effect::arpeggio3Notes;
    }
    if (string == "arpeggio4Notes") {
        return Effect::arpeggio4Notes;
    }
    if (string == "reset") {
        return Effect::reset;
    }
    if (string == "forceInstrumentSpeed") {
        return Effect::forceInstrumentSpeed;
    }
    if (string == "forceArpeggioSpeed") {
        return Effect::forceArpeggioSpeed;
    }
    if (string == "forcePitchTableSpeed") {
        return Effect::forcePitchTableSpeed;
    }
    if (string == "fastPitchUp") {
        return Effect::fastPitchUp;
    }
    if (string == "fastPitchDown") {
        return Effect::fastPitchDown;
    }

    return { };
}

/**
 * @return a String of effect, for display, or empty if it couldn't be.
 * @param effect the effect.
 */
juce::String EffectUtil::toDisplayedString(const Effect effect) noexcept
{
    switch (effect) {
        case Effect::legato:        // Never encoded.
        default:
            jassertfalse;           // Not supposed to happen.
            return "";
        case Effect::noEffect:
            return "No effect";
        case Effect::pitchUp:
            return "Pitch up";
        case Effect::pitchDown:
            return "Pitch down";
        case Effect::pitchGlide:
            return "Pitch glide";
        case Effect::pitchTable:
            return "Pitch table";
        case Effect::volume:
            return "Volume";
        case Effect::volumeIn:
            return "Volume in";
        case Effect::volumeOut:
            return "Volume out";
        case Effect::arpeggioTable:
            return "Arpeggio table";
        case Effect::arpeggio3Notes:
            return "Arpeggio 3 notes";
        case Effect::arpeggio4Notes:
            return "Arpeggio 4 notes";
        case Effect::reset:
            return "Reset";
        case Effect::forceInstrumentSpeed:
            return "Force instrument speed";
        case Effect::forceArpeggioSpeed:
            return "Force arpeggio speed";
        case Effect::forcePitchTableSpeed:
            return "Force pitch table speed";
        case Effect::fastPitchUp:
            return "Fast pitch up";
        case Effect::fastPitchDown:
            return "Fast pitch down";
    }
}

}   // namespace arkostracker
