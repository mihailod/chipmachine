#include "EffectErrorHelper.h"

namespace arkostracker
{

juce::String EffectErrorHelper::effectErrorToDisplayableString(const EffectError effectError) noexcept
{
    // Links an EffectError to a String for display.
    static const std::map<EffectError, juce::String> effectErrorToString = {
            { EffectError::removeEffectBeforeReset, juce::translate("Any effect before reset will be ignored.") },
            { EffectError::tooManyArpeggioEffects, juce::translate("Only one arpeggio effect is possible.") },
            { EffectError::tooManyForceArpeggioSpeedEffects, juce::translate("Only one Force Arpeggio Speed effect is possible.") },
            { EffectError::tooManyForcePitchTableSpeedEffects, juce::translate("Only one Force Pitch Speed effect is possible.") },
            { EffectError::tooManyForceInstrumentSpeedEffects, juce::translate("Only one Force Instrument Speed effect is possible.") },
            { EffectError::tooManyPitchEffects, juce::translate("Only one pitch effect is possible.") },
            { EffectError::tooManyPitchTableEffects, juce::translate("Only one pitch table effect is possible.") },
            { EffectError::tooManyResetEffects, juce::translate("Only one reset effect is possible.") },
            { EffectError::tooManyVolumeEffects, juce::translate("Only one volume effect is possible.") },
            { EffectError::tooManyVolumeSlideEffects, juce::translate("Only one volume in/out is possible.") },
            { EffectError::pitchTableHidingForceSpeed, juce::translate("The pitch effect hides the forced speed.") },
            { EffectError::arpeggioTableOrDirectHidingForceSpeed, juce::translate("The arpeggio effect hides the forced speed.") },
            { EffectError::uselessEffectStopAfterReset, juce::translate("After a reset, an effect with value 0 is useless.") },
            { EffectError::volumeHidingVolumeSlide, juce::translate("The volume is hiding a volume in/out.") },
            { EffectError::volumeMustBeCombinedWithReset, juce::translate("Reset effect has an inverted volume, use it instead.") },
            { EffectError::pitchGlideAfterResetIsMeaningless, juce::translate("A pitch glide after a reset is meaningless.") },
            { EffectError::pitchGlideWithInstrumentIsForbidden, juce::translate("Remove instrument when using glide.") },
            { EffectError::pitchAndGlide0WithNewInstrumentIsUseless, juce::translate("Pitch/glide 0 with new instrument is useless.") },
            { EffectError::pitchGlide0WithNoteIsUseless, juce::translate("Glide 0 with a note is useless.") },
    };
    jassert(effectErrorToString.size() == static_cast<size_t>(EffectError::countForEditor));          // Error handling missing!

    if (const auto iterator = effectErrorToString.find(effectError); iterator != effectErrorToString.cend()) {
        return iterator->second;
    }

    jassertfalse;           // Error not found! Abnormal!
    return juce::translate("Unknown error. This is a bug :).");
}

}   // namespace arkostracker
