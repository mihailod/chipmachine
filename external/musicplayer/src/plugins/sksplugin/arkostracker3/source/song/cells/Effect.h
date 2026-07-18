#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/**
 * The possible effects a Track can have.
 *
 * Note that future effects can be added. Implementations using effects may or may not recognize all of them.
 * Added effects should also be added to SongCellChecker::effectNumberToRank to know in which order to preferably encode them.
 */
enum class Effect : unsigned char
{
    //unknownEffect = 255,

    noEffect = 0U,               // No effect.

    pitchUp,                    // Pitch up.
    pitchDown,                  // Pitch down.
    pitchGlide,                 // Pitch glide.
    pitchTable,                 // Pitch table.
    volume,                     // Volume.
    volumeIn,                   // Volume in.
    volumeOut,                  // Volume out.
    arpeggioTable,              // Arpeggio table.
    arpeggio3Notes,             // Arpeggio with three notes (two shown).
    arpeggio4Notes,             // Arpeggio with three notes (three shown).
    reset,                      // Reset all effects (arpeggios, pitch table, volume to full).
    forceInstrumentSpeed,       // Forces the Instrument speed.
    forceArpeggioSpeed,         // Forces the Arpeggio speed.
    forcePitchTableSpeed,       // Forces the Pitch Table speed.

    fastPitchUp,
    fastPitchDown,

    legato,                     // Legato. WARNING, this is NOT supposed to be used in the UI.
    lastEffect2_0 = legato
};

class EffectUtil
{
public:
    /** Prevents instantiation. */
    EffectUtil() = delete;

    /**
     * Indicates whether the given effect is a Pitch ((fast)up/down).
     * @param effectNumber the effect number.
     */
    static bool isPitchEffect(int effectNumber) noexcept;

    /** @return how many digits are used for the given effect. (1 for volume, 2 for arp table, etc.). From 0 to 3, included. */
    static int getDigitCount(Effect effect) noexcept;
    /**
     * @return the min and max values for an effect.
     * @param effect the effect.
     */
    static std::pair<int, int> getMinMaxValues(Effect effect) noexcept;

    /**
     * @return a String for serialization for an effect.
     * @param effect the effect.
     */
    static juce::String toSerializedString(Effect effect) noexcept;

    /**
     * @return the effect from the given String. Empty if not known.
     * @param string the String.
     */
    static OptionalValue<Effect> fromSerializedString(const juce::String& string) noexcept;

    /**
     * @return a String of effect, for display, or empty if it couldn't be.
     * @param effect the effect.
     */
    static juce::String toDisplayedString(Effect effect) noexcept;
};

}   // namespace arkostracker
