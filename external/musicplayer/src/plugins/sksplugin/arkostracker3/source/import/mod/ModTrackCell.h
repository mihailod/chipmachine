#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Represents a Cell inside a Track of a MOD format. This is a raw view of the data from the binary MOD file. */
class ModTrackCell
{
public:
   explicit ModTrackCell(int sampleNumber = 0, int samplePeriod = 0, int effectNumberAndValue = 0) noexcept;

   static ModTrackCell buildFromMemoryBlock(const juce::MemoryBlock& memoryBlock, int offset) noexcept;

   /** @return the sample number, or 0 if there is none. */
   int getSampleNumber() const noexcept;
   /** @return the sample period, or 0 if there is none. */
   int getSamplePeriod() const noexcept;
   /** @return the effect and value, or 0 if there is none. */
   int getEffectNumberAndValue() const noexcept;

   /** @return the left digit of the effect, from 0 to 15. */
   int getEffectNumber() const noexcept;
   /** @return the two right digits of the effect, from 0 to 255. The most significant quartet may actually be an effect, if the primary effect is 14. */
   int getEffectValue() const noexcept;

   /** Indicates whether a note is present. */
   bool isNotePresent() const noexcept;
   /** Indicates whether an effect (non 0) is present. */
   bool isEffectPresent() const noexcept;

   /** Indicates whether there is note data in the Cell. */
   bool isEmpty() const noexcept;

private:
   static constexpr auto cellSize = 4;      // The size of a MOD cell, in bytes.

   int sampleNumber;                        // The sample number, or 0 if there is none.
   int samplePeriod;                        // The sample period, or 0 if there is none.
   int effectNumberAndValue;                // The effect and value, or 0 if there is none.
};

}   // namespace arkostracker

