#pragma once

#include "../../../../../song/Expression.h"
#include "../../../../../utils/FpFloat.h"
#include "../../../../../utils/OptionalValue.h"

namespace arkostracker
{

/** The context for one line of a channel. */
class LineContext
{
public:
   /** Constructor with no data. */
   LineContext() noexcept;

   /** Constructor with no inline arpeggio. */
   LineContext(OptionalId pitchId, OptionalId arpeggioId, int volume) noexcept;

   /** @return a copy of this object with another arpeggioId. */
   LineContext withArpeggioId(const OptionalId& arpeggioId) const noexcept;
   /** @return a copy of this object with another PitchId. */
   LineContext withPitchId(const OptionalId& pitchId) const noexcept;
   /** @return a copy of this object with another Inline Arpeggio. */
   LineContext withArpeggioInline(const OptionalValue<Expression>& expression) const noexcept;
   /** @return a copy of this object with another volume (0-15). */
   LineContext withVolume(int volume, int decimalPart = 0) const noexcept;

   /** Sets the pitch from its id. */
   void setPitchId(const Id& pitchId) noexcept;
   /** Sets the arpeggio from its id. It also resets the inline arpeggio. */
   void setArpeggioIdAndResetInline(const Id& arpeggioId) noexcept;
   /** Sets the arpeggio inline. It also resets the arpeggio table. */
   void setInlineArpeggioAndResetArpeggioTable(const Expression& inlineArpeggio) noexcept;
   /** Sets the volume. Must be between 0 and 15. */
   void setVolume(const FpFloat& volume) noexcept;

   /** @return the possible pitch id. */
   OptionalId getPitchId() const noexcept;
   /** @return the possible arpeggio id. */
   OptionalId getArpeggioId() const noexcept;
   /** @return the possible inline arpeggio. */
   OptionalValue<Expression> getArpeggioInline() const noexcept;
   /** @return the volume. */
   FpFloat getVolume() const noexcept;

   /** @return true if the value are set (volume does not count). */
   bool isFull() const noexcept;

   bool operator==(const LineContext& other) const noexcept;
   bool operator!=(const LineContext& other) const noexcept;

private:
   OptionalId pitchId;
   OptionalId arpeggioId;
   /** Empty if not present. */
   OptionalValue<Expression> inlineArpeggio;
   /** From 0 to 15. */
   FpFloat volume;
};

}   // namespace arkostracker
