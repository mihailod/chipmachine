#pragma once

#include "../../../../../song/Expression.h"
#include "../../../../../song/cells/Effect.h"
#include "../../../../../utils/OptionalValue.h"

namespace arkostracker
{

/** The context for one Cell. */
class CellContext
{
public:
   /** Constructor with no data. */
   CellContext() = default;

   /** Invalidates the given effect. Asserts if effect not "invalidatable". */
   void invalidateEffect(Effect effect) noexcept;
   void invalidateEffects(const std::set<Effect>& effects) noexcept;
   /** Sets the given effect value to 0. Asserts if effect not "resettable". */
   void resetEffect(Effect effect) noexcept;
   void resetEffects(const std::set<Effect>& effects) noexcept;

   /**
    * Sets a value.
    * @param effect the effect.
    * @param value the value.
    */
   void setEffectValue(Effect effect, int value) noexcept;

   /** Sets the current volume, from 0 to 15. */
   void setVolume(int volume) noexcept;
   /** @return the volume from 0-15, or invalidated. */
   OptionalInt getCurrentVolume() const noexcept;

   OptionalInt getCurrentPitchValueFromEffect(Effect effect) const noexcept;
   OptionalInt getCurrentVolumeValueFromEffect(Effect effect) const noexcept;
   OptionalInt getCurrentSpeedValueFromEffect(Effect effect) const noexcept;
   OptionalInt getCurrentArpeggioValueFromEffect(Effect effect) const noexcept;
   OptionalInt getCurrentPitchTableValue() const noexcept;

   void setPitchUp(int value) noexcept;
   OptionalInt getCurrentPitchUp() const noexcept;

private:
   OptionalId pitchId;
   OptionalId arpeggioId;
   /** Empty if not present. */
   OptionalValue<Expression> inlineArpeggio;

   OptionalValue<int> currentVolume;            // From 0 to 15, set to Absent after a volume in/out.
   OptionalValue<int> effectVolumeInValue;
   OptionalValue<int> effectVolumeOutValue;

   OptionalValue<int> effectFastPitchUpValue;
   OptionalValue<int> effectFastPitchDownValue;
   OptionalValue<int> effectPitchUpValue;
   OptionalValue<int> effectPitchDownValue;
   OptionalValue<int> effectPitchGlideValue;

   OptionalValue<int> effectArpeggioTableValue;
   OptionalValue<int> effectArpeggio3NotesValue;
   OptionalValue<int> effectArpeggio4NotesValue;

   OptionalValue<int> effectPitchTableValue;

   OptionalValue<int> effectForceInstrumentSpeedValue;
   OptionalValue<int> effectForceArpeggioSpeedValue;
   OptionalValue<int> effectForcePitchTableSpeedValue;
};

}   // namespace arkostracker
