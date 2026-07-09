#include "ModTrackCell.h"

#include "../../utils/MemoryBlockUtil.h"

namespace arkostracker
{

ModTrackCell::ModTrackCell(const int pSampleNumber, const int pSamplePeriod, const int pEffectNumberAndValue) noexcept :
   sampleNumber(pSampleNumber),
   samplePeriod(pSamplePeriod),
   effectNumberAndValue(pEffectNumberAndValue)
{
}

int ModTrackCell::getSampleNumber() const noexcept
{
   return sampleNumber;
}

int ModTrackCell::getSamplePeriod() const noexcept
{
   return samplePeriod;
}

int ModTrackCell::getEffectNumberAndValue() const noexcept
{
   return effectNumberAndValue;
}

int ModTrackCell::getEffectNumber() const noexcept
{
    return static_cast<int>(((static_cast<unsigned int>(effectNumberAndValue) >> 8U) & 0xfU));
}

int ModTrackCell::getEffectValue() const noexcept
{
    return static_cast<int>((static_cast<unsigned int>(effectNumberAndValue) & 0xffU));
}

bool ModTrackCell::isNotePresent() const noexcept
{
   return (samplePeriod > 0);
}

bool ModTrackCell::isEffectPresent() const noexcept
{
    return (getEffectNumber() > 0);
}

bool ModTrackCell::isEmpty() const noexcept
{
    return ((samplePeriod == 0) && (sampleNumber == 0) && (effectNumberAndValue == 0));
}

ModTrackCell ModTrackCell::buildFromMemoryBlock(const juce::MemoryBlock& memoryBlock, const int offset) noexcept
{
   // Makes sure the block has the right size.
   const auto success = MemoryBlockUtil::isMemoryBlockLongEnough(memoryBlock, offset, 4);
   if (!success) {
      return ModTrackCell();
   }

   // Reads the bytes.
   unsigned char bytes[cellSize]; // NOLINT(*-avoid-c-arrays)
   memoryBlock.copyTo(bytes, offset, static_cast<size_t>(cellSize)); // NOLINT(*-pro-bounds-array-to-pointer-decay, *-no-array-decay)

   const auto byte1 = bytes[0];
   const auto byte2 = bytes[1];
   const auto byte3 = bytes[2];
   const auto byte4 = bytes[3];

   //   byte1     byte2     byte3     byte4
   // 7654-3210 7654-3210 7654-3210 7654-3210
   // wwww xxxxxxxxxxxxxx yyyy zzzzzzzzzzzzzz

   // wwwwyyyy(8 bits) is the sample for this channel / division
   // xxxxxxxxxxxx(12 bits) is the sample's period (or effect parameter)
   // zzzzzzzzzzzz(12 bits) is the effect for this channel / division
   const auto sampleNumber = static_cast<int>((byte1 & 0xf0U) | ((static_cast<unsigned int>(byte3) >> 4U) & 0xfU));
   const auto samplePeriod = static_cast<int>(((byte1 & 0xfU) << 8U) | byte2);
   const auto effectNumberAndValue = (((byte3 & 0xfU) << 8U) | byte4);

   return ModTrackCell(sampleNumber, samplePeriod, static_cast<int>(effectNumberAndValue));
}

}   // namespace arkostracker
