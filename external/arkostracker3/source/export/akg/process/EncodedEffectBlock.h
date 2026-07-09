#pragma once

#include "juce_core/juce_core.h"

#include "EncodedEffect.h"
#include "../../../song/cells/Note.h"

namespace arkostracker 
{

class CellEffects;
class PlayerConfiguration;

/**
 * Represents an encoded Effect Block. Effects are added, then a Label can be retrieved from it and used as a hashcode in a Map.
 * The given effects may comes directly from the Tracks and thus contain errors (twice the same effect, etc),
 * but the order will be secured by this class on encoding.
 */
class EncodedEffectBlock
{
public:
    /**
     * Constructor.
     * @param playerConfiguration the configuration to add flags to.
     * @param note the possible note. It is necessary by some effects, such as Glide. It will be ignored in the other case anyway.
     * @param cellEffects the effects, as-is. They may not be valid, they will be corrected before stored.
     */
    EncodedEffectBlock(PlayerConfiguration& playerConfiguration, OptionalValue<Note> note, const CellEffects& cellEffects) noexcept;

    /** @return the bytes of the effects. */
    std::vector<unsigned char> getBytes() const noexcept;
    /** @return how many bytes are encoded in this effect block. */
    int getByteCount() const noexcept;

    /** @return the ID of the effects. It is meant to be used as a label in assembler. */
    juce::String getId() const noexcept;

    /** Equality operator, used for maps/sets. */
    bool operator==(const EncodedEffectBlock& other) const noexcept;
    /** Inequality operator. */
    bool operator!=(const EncodedEffectBlock& other) const noexcept;

private:
    /**
     * Encodes the given effects into bytes.
     * @param playerConfiguration to add new flags.
     * @param note the possible note . It is necessary by some effects, such as Glide. It will be ignored in the other case anyway.
     * @param cellEffects the effects. They must be valid, but the order does not matter.
     */
    static std::vector<unsigned char> encodeEffects(PlayerConfiguration& playerConfiguration, OptionalValue<Note> note, const CellEffects& cellEffects) noexcept;

    /**
     * Adds an effect at the end of the given array.
     * @param effectToEncode the effect to encode.
     * @param moreEffects true if there are more effects that this one, false if it is the last.
     * @param encodedEffects the array of data.
     */
    static void encodeEffectNumber(EncodedEffect effectToEncode, bool moreEffects, std::vector<unsigned char>& encodedEffects);

    /**
     * Adds a value at the end of the given array.
     * @param value the value to encode. Should be between 0 and 255.
     * @param encodedEffects the array of data.
     */
    static void encodeEffectValue(int value, std::vector<unsigned char>& encodedEffects);

    /**
     * Encodes a 16 bits value. The effect is not encoded here.
     * @param value the value.
     * @param encodedEffects the array of data to fill.
     */
    static void encodeEffectValue16Bits(int value, std::vector<unsigned char>& encodedEffects);

    /**
     * Encodes a pitch effect (effect and value). If the value is 0, a "stop pitch" is encoded.
     * Can also be used for a Glide Stop.
     * @param pitchEffect what effect (slow/fast, up/down).
     * @param value the value. Must be positive. May be 0.
     * @param moreEffects true if there are more effects that this one, false if it is the last.
     * @param encodedEffects the array of data to fill.
     */
    static void encodeEffectPitch(EncodedEffect pitchEffect, int value, bool moreEffects, std::vector<unsigned char>& encodedEffects);

    /**
     * Encodes a volume in/out (effect and value). If the value is 0, a "stop volume slide" is encoded.
     * @param outSlide false if in, true if out.
     * @param value the value. May be negative, may be 0.
     * @param moreEffects true if there are more effects that this one, false if it is the last.
     * @param encodedEffects the array of data to fill.
     */
    static void encodeVolumeSlide(bool outSlide, int value, bool moreEffects, std::vector<unsigned char>& encodedEffects);

    /**
     * Encodes a glide effect (effect and value). The encoding depends on whether a note is here. A "stop pitch" is encoded.
     * @param note the possible note.
     * @param value the value (>=0).
     * @param moreEffects true if there are more effects that this one, false if it is the last.
     * @param encodedEffects the array of data to fill.
     */
    static void encodeEffectGlide(OptionalValue<Note> note, int value, bool moreEffects, std::vector<unsigned char>& encodedEffects);

    /** @return the speed, increased of 1. If 256, it becomes 0. */
    static int getIncreasedSpeed(int speed) noexcept;

    std::vector<unsigned char> encodedBytes;            // The bytes of the effects, when encoded.
    juce::String id;                                    // The id of the effects, from the bytes. Useful for labels.
};

}   // namespace arkostracker

