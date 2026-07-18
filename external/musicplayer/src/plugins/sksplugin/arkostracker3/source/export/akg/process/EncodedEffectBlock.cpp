#include "EncodedEffectBlock.h"

#include "../../../song/cells/CellEffects.h"
#include "../../../utils/NumberUtil.h"
#include "../../playerConfiguration/PlayerConfiguration.h"

namespace arkostracker 
{

EncodedEffectBlock::EncodedEffectBlock(PlayerConfiguration& playerConfiguration, const OptionalValue<Note> note, const CellEffects& cellEffects) noexcept:
        encodedBytes(),
        id()
{
    // Encodes the effects into bytes.
    encodedBytes = encodeEffects(playerConfiguration, note, cellEffects);

    // Calculates and stores the ID from the encoded bytes.
    id = NumberUtil::generateIdFromArray(encodedBytes);
}

std::vector<unsigned char> EncodedEffectBlock::getBytes() const noexcept
{
    return encodedBytes;
}

int EncodedEffectBlock::getByteCount() const noexcept
{
    return static_cast<int>(encodedBytes.size());
}

juce::String EncodedEffectBlock::getId() const noexcept
{
    return id;
}

bool EncodedEffectBlock::operator==(const EncodedEffectBlock& other) const noexcept
{
    return (id == other.id);
}

bool EncodedEffectBlock::operator!=(const EncodedEffectBlock& other) const noexcept
{
    return !operator==(other);
}

std::vector<unsigned char> EncodedEffectBlock::encodeEffects(PlayerConfiguration& playerConfiguration, const OptionalValue<Note> note, const CellEffects& cellEffects) noexcept
{
    // First, encodes the effects in a byte array.
    std::vector<unsigned char> encodedEffects;

    if (cellEffects.isEmpty()) {
        return encodedEffects;
    }

    // Encodes the present effects.
    const auto indexToCellEffect = cellEffects.getExistingEffects();
    auto effectRemainingCount = indexToCellEffect.size();
    for (const auto&[effectIndex, cellEffect] : indexToCellEffect) {
        const auto areMoreEffectsRemaining = (--effectRemainingCount) > 0;
        const auto effectValue = cellEffect.getEffectLogicalValue();

        switch (cellEffect.getEffect()) {
            case Effect::pitchUp:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchUp);
                encodeEffectPitch(EncodedEffect::pitchUp, effectValue, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::pitchDown:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchDown);
                encodeEffectPitch(EncodedEffect::pitchDown, effectValue, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::fastPitchUp:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchUp);
                encodeEffectPitch(EncodedEffect::pitchUp, effectValue << 4, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::fastPitchDown:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchDown);
                encodeEffectPitch(EncodedEffect::pitchDown, effectValue << 4, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::pitchGlide:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchGlide);
                encodeEffectGlide(note, effectValue, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::reset: {
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectReset);

                // Reset. In case the reset is full volume (R0), a special effect is coded with no parameter.
                const auto invertedVolume = effectValue;
                if (invertedVolume == 0) {
                    encodeEffectNumber(EncodedEffect::resetFullVolume, areMoreEffectsRemaining, encodedEffects);
                } else {
                    encodeEffectNumber(EncodedEffect::reset, areMoreEffectsRemaining, encodedEffects);
                    encodeEffectValue(invertedVolume, encodedEffects);
                }
                break;
            }
            case Effect::volume: {
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectSetVolume);
                const auto invertedVolume = 15 - effectValue;
                encodeEffectNumber(EncodedEffect::volume, areMoreEffectsRemaining, encodedEffects);
                encodeEffectValue(invertedVolume, encodedEffects);
                break;
            }

            case Effect::volumeIn:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectVolumeIn);
                encodeVolumeSlide(false, effectValue, areMoreEffectsRemaining, encodedEffects);
                break;
            case Effect::volumeOut:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectVolumeOut);
                encodeVolumeSlide(true, effectValue, areMoreEffectsRemaining, encodedEffects);
                break;

            case Effect::arpeggioTable: {
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectArpeggioTable);
                const auto arpeggioTable = effectValue;
                // Encodes an arpeggio Table, or a stop?
                if (arpeggioTable == 0) {
                    encodeEffectNumber(EncodedEffect::arpeggioTableStop, areMoreEffectsRemaining, encodedEffects);
                } else {
                    encodeEffectNumber(EncodedEffect::arpeggioTable, areMoreEffectsRemaining, encodedEffects);
                    encodeEffectValue(arpeggioTable - 1, encodedEffects);       // Index is >=0.
                }
                break;
            }
            case Effect::pitchTable: {
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectPitchTable);
                const auto pitchTable = effectValue;
                // Encodes a Pitch Table, or a stop?
                if (pitchTable == 0) {
                    encodeEffectNumber(EncodedEffect::pitchTableStop, areMoreEffectsRemaining, encodedEffects);
                } else {
                    encodeEffectNumber(EncodedEffect::pitchTable, areMoreEffectsRemaining, encodedEffects);
                    encodeEffectValue(pitchTable - 1, encodedEffects);       // Index is >=0.
                }
                break;
            }

            case Effect::legato:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectLegato);
                encodeEffectNumber(EncodedEffect::legato, areMoreEffectsRemaining, encodedEffects);
                encodeEffectValue(effectValue, encodedEffects);       // Direct encoding of the note.
                break;

            case Effect::forceInstrumentSpeed:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectForceInstrumentSpeed);
                encodeEffectNumber(EncodedEffect::forceInstrumentSpeed, areMoreEffectsRemaining, encodedEffects);
                encodeEffectValue(getIncreasedSpeed(effectValue), encodedEffects);
                break;
            case Effect::forceArpeggioSpeed:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectForceArpeggioSpeed);
                encodeEffectNumber(EncodedEffect::forceArpeggioSpeed, areMoreEffectsRemaining, encodedEffects);
                encodeEffectValue(getIncreasedSpeed(effectValue), encodedEffects);
                break;
            case Effect::forcePitchTableSpeed:
                playerConfiguration.addFlag(PlayerConfigurationFlag::effectForcePitchTableSpeed);
                encodeEffectNumber(EncodedEffect::forcePitchTableSpeed, areMoreEffectsRemaining, encodedEffects);
                encodeEffectValue(getIncreasedSpeed(effectValue), encodedEffects);
                break;
            case Effect::noEffect:
            case Effect::arpeggio3Notes:
            case Effect::arpeggio4Notes:
            default:
                jassertfalse;       // Effect not managed! Shouldn't happen.
                break;
        }
    }

    return encodedEffects;
}

void EncodedEffectBlock::encodeEffectNumber(EncodedEffect effectToEncode, const bool moreEffects, std::vector<unsigned char>& encodedEffects)
{
    // *2 because the bit 0 is "more effects?".
    auto encodedEffect = static_cast<int>(effectToEncode) * 2;
    if (moreEffects) {
        encodedEffect |= 1;     // Bit 0 set to 1 if "more effects".
    }

    jassert((encodedEffect >= 0) && (encodedEffect <= 255));

    encodedEffects.push_back(static_cast<unsigned char>(encodedEffect));
}

void EncodedEffectBlock::encodeEffectValue(const int value, std::vector<unsigned char>& encodedEffects)
{
    jassert((value >= 0) && (value <= 255));
    encodedEffects.push_back(static_cast<unsigned char>(value));
}

void EncodedEffectBlock::encodeEffectValue16Bits(const int value, std::vector<unsigned char>& encodedEffects)
{
    jassert((value >= -0xffff) && (value <= 0xffff));

    // Encodes the 16 bits value.
    encodeEffectValue(value & 255, encodedEffects);
    encodeEffectValue((value >> 8) & 255, encodedEffects);
}

void EncodedEffectBlock::encodeEffectPitch(const EncodedEffect pitchEffect, const int value, const bool moreEffects, std::vector<unsigned char>& encodedEffects)
{
    // Is the value 0? If yes, encodes a "pitch stop".
    if (value == 0) {
        encodeEffectNumber(EncodedEffect::pitchStop, moreEffects, encodedEffects);
    } else {
        jassert((pitchEffect == EncodedEffect::pitchDown) || (pitchEffect == EncodedEffect::pitchUp));

        encodeEffectNumber(pitchEffect, moreEffects, encodedEffects);
        encodeEffectValue16Bits(value, encodedEffects);
    }
}

int EncodedEffectBlock::getIncreasedSpeed(int speed) noexcept
{
    ++speed;
    return speed % 256;
}

void EncodedEffectBlock::encodeVolumeSlide(const bool outSlide, const int value, const bool moreEffects, std::vector<unsigned char>& encodedEffects)
{
    if (value == 0) {
        encodeEffectNumber(EncodedEffect::volumeSlideStop, moreEffects, encodedEffects);
    } else {
        encodeEffectNumber(EncodedEffect::volumeSlide, moreEffects, encodedEffects);
        encodeEffectValue16Bits(outSlide ? value : -value, encodedEffects);     // The expected volume slide is inverted! negative for a fade in, positive for a fade out.
    }
}

void EncodedEffectBlock::encodeEffectGlide(const OptionalValue<Note> note, const int cellValue, const bool moreEffects, std::vector<unsigned char>& encodedEffects)
{
    // Is there a note?
    if (note.isPresent()) {
        // A note is present, so this is a GlideWithNote.
        // The speed may be 0, but even though it has no interest, it must be encoded anyway, because a more relevant value (without note) may follow.
        encodeEffectNumber(EncodedEffect::glideWithNote, moreEffects, encodedEffects);
        encodeEffectValue(note.getValue().getNote(), encodedEffects);
        encodeEffectValue16Bits(cellValue, encodedEffects);
    } else {
        // No note. There is only a Speed.
        if (cellValue == 0) {
            // Glide Stop? The encoded effect does not matter, it will be encoded as a Pitch Stop.
            encodeEffectPitch(EncodedEffect::glideWithNote, 0, moreEffects, encodedEffects);
        } else {
            encodeEffectNumber(EncodedEffect::glideSpeed, moreEffects, encodedEffects);
            encodeEffectValue16Bits(cellValue, encodedEffects);
        }
    }
}

}   // namespace arkostracker

