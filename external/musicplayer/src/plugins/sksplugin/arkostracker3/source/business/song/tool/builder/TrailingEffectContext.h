#pragma once

#include <map>

#include "../../../../utils/OptionalValue.h"
#include "../../../../song/cells/Effect.h"

namespace arkostracker 
{

/** Effects that are used in the "trailing effect context". They can be anything used in a Tracker, such as VT2, Soundtrakker, STarKos... */
enum class TrailingEffect
{
    noEffect,

    pitchUp,
    pitchDown,

    volume,
    reset,

    arpeggioTable,
    arpeggio3Notes,

    hardwareEnvelope,
};

/**
 * A class that keeps a "context" of effect, for one channel. For example, the volume, pitch, etc.
 * The TrackCleaner removes useless effects, but it is different from the TrailingEffectContext, as some trackers (Soundtrakker, MOD, VT2) use
 * "trailing effects" (pitch is always shown as long as needed, or 0 value means continue).
 * This class does not attempt to optimize the tracks, as it is not its responsibility.
 *
 * A new instance should be created whenever a new track is browsed.
 */
class TrailingEffectContext
{
public:
    /** Constructor. */
    TrailingEffectContext() noexcept;

    /**
     * Declares an effect. It may be "pitch up" with a value or 0 (continuous) for example.
     * If there is no effect, it may signify that the previous trailing effect is over, in which case a new effect will be returned (pitch off).
     * If there is an effect and it is the same as before, or continuous, the returned effect may be No effect.
     * @param effect the effect (not encoded yet).
     * @param value the logical value.
     * @return the effect, modified or not. It may be a "no effect".
     */
    std::pair<TrailingEffect, int> declareEffect(TrailingEffect effect, int value) noexcept;

    /** Declares a note. This resets some continuous effects (pitch, hardware envelope (as a simplification), but not arpeggio). */
    void declareNote() noexcept;

    /** @return the current volume. Empty if not known. */
    OptionalInt getCurrentVolume() const noexcept;

    /** @return true if a value is present for an effect. */
    bool isEffectPresent(TrailingEffect effect) noexcept;

    /**
     * Invalidates an effect.
     * @param effect the effect to invalidate.
     */
    void invalidateEffect(TrailingEffect effect) noexcept;

    /** @return the current Instrument index. */
    OptionalInt getCurrentInstrumentIndex() const noexcept;

    /** Sets the instrument index that is currently used. */
    void setCurrentInstrumentIndex(OptionalInt instrumentIndex) noexcept;

private:
    /**
     * @return the current value related to the effect, or empty if there is none.
     * @param effect the effect.
     */
    OptionalInt getEffectValue(TrailingEffect effect) const noexcept;

    /**
     * Sets the effect and value.
     * @param effect the effect.
     * @param value the value.
     */
    void setEffectAndValue(TrailingEffect effect, int value) noexcept;

    /**
     * Manages the declaration of a Pitch.
     * @param up true if pitch up, false if down.
     * @param value the value.
     * @return the effect to encode.
     */
    std::pair<TrailingEffect, int> onPitchFound(bool up, int value) noexcept;

    /**
     * Manages "classic" trailing effect. If the value is the same as before, don't encode anything.
     * @param trailingEffect the effect.
     * @param value the value.
     * @return the effect to encode.
     */
    std::pair<TrailingEffect, int> onTrailingEffect(TrailingEffect trailingEffect, int value) noexcept;

    std::map<TrailingEffect, OptionalInt> effectToValue;

    OptionalInt currentInstrumentIndex;
};


}   // namespace arkostracker

