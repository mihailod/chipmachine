#include "AkmSequenceEncoder.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/BitNumber.h"
#include "../../../utils/CollectionUtil.h"
#include "../../SongExportResult.h"

namespace arkostracker 
{

const std::set<Effect> AkmSequenceEncoder::knownEffects = {    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        Effect::volume,
        Effect::reset,
        Effect::pitchUp,
        Effect::pitchDown,
        Effect::fastPitchUp,
        Effect::fastPitchDown,
        Effect::arpeggioTable,
        Effect::pitchTable,
        Effect::forceArpeggioSpeed,
        Effect::forcePitchTableSpeed,
        Effect::forceInstrumentSpeed
};

std::vector<std::pair<unsigned int, juce::String>> AkmSequenceEncoder::encodeTrack(SongExportResult& songExportResult,
                               const Track& track, bool areEffectsPresentInSong, int primaryDefaultInstrumentIndex,
                               int secondaryDefaultInstrumentIndex, int primaryWait, int secondaryWait, int startNoteInTrack, int startInstrumentInTrack, int startWaitTrack,
                               const std::vector<int>& mostUsedNoteIndexes) noexcept
{
    jassert(valueNoteAndEffects < 16);
    jassert(valueNewEscapeNote < 16);
    jassert(valueSameEscapeNote < 16);
    jassert(valueNoNoteMaybeEffects < 16);

    // ValueNoteAndEffects must be the first! Because it might be skipped.
    jassert(valueNoteAndEffects < valueNoNoteMaybeEffects);
    jassert(valueNoteAndEffects < valueNewEscapeNote);
    jassert(valueNoteAndEffects < valueSameEscapeNote);

    std::vector<std::pair<unsigned int, juce::String>> outputData;

    auto currentInstrumentIndex = startInstrumentInTrack;
    auto currentWait = startWaitTrack;
    auto currentNote = startNoteInTrack;

    AccumulatedCell accumulatedCell;
    for (auto cellIndex = 0, cellCount = Track::getSize(); cellIndex < cellCount; ++cellIndex) {
        auto cell = track.getCell(cellIndex);
        // Important: removes the unknown effects first, so that the waits are well optimized.
        removeUnknownEffects(cell, songExportResult);

        // Nothing has been accumulated? Then stores the cell and that's all.
        // We need to do that because the next Wait is encoded in the encoded Cell,
        // so we need to know what is "after".
        if (!accumulatedCell.isSet()) {
            accumulatedCell = AccumulatedCell(cell);
        } else {
            if (cell.isEmpty()) {
                accumulatedCell.increaseAccumulatedWait();
            } else {
                // There is "something".
                encodeAccumulatedCell(outputData, songExportResult, accumulatedCell, areEffectsPresentInSong, currentNote, currentInstrumentIndex,
                                      currentWait, true,
                                      primaryDefaultInstrumentIndex, secondaryDefaultInstrumentIndex,
                                      primaryWait, secondaryWait, mostUsedNoteIndexes);
                // Now, the Cell becomes the read one.
                accumulatedCell = AccumulatedCell(cell);
            }
        }
    }

    // At the end, there might be something accumulated.
    // However, we don't encode its possible "wait", if it was the very last note of a track (127th cell).
    // This case can be tested easily: it has no wait accumulated.
    const auto encodeWait = (accumulatedCell.getAccumulatedWait() != 0);
    if (encodeWait) {
        // The wait encoded is forced to the maximum, it will take care of the end of the track, and the player code won't have to test anything.
        accumulatedCell.forceWaitToMax();
    }

    encodeAccumulatedCell(outputData, songExportResult, accumulatedCell, areEffectsPresentInSong, currentNote, currentInstrumentIndex,
                          currentWait, encodeWait,
                          primaryDefaultInstrumentIndex, secondaryDefaultInstrumentIndex,
                          primaryWait, secondaryWait, mostUsedNoteIndexes);

    return outputData;
}

void AkmSequenceEncoder::encodeAccumulatedCell(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult,
                                               const AccumulatedCell& accumulatedCell,
                                               const bool areEffectsPresentInSong, int& currentNote, int& currentInstrumentIndex, int& currentWait, const bool mustEncodeWaits,
                                               const int primaryDefaultInstrumentIndex, const int secondaryDefaultInstrumentIndex, const int primaryWait, const int secondaryWait,
                                               const std::vector<int>& mostUsedNoteIndexes) noexcept
{
    (void)areEffectsPresentInSong;      // To avoid warning in Release.

    // Not set? Then don't do anything.
    if (!accumulatedCell.isSet()) {
        return;
    }
    const auto accumulatedWait = accumulatedCell.getAccumulatedWait();

    auto waitFlags = waitFlagsSameEscape;                // The 2-bit wait flags.
    auto instrumentFlags = instrumentFlagsSameEscape;    // The 2-bit instrument flags.
    int data;                                            // The 6-bit data.     // NOLINT(*-init-variables)
    auto waitEscape = false;
    auto instrumentEscape = false;
    auto noteEscape = false;

    // Reads the Cell.
    auto cell = accumulatedCell.getCell();

    // Takes care of the legato: it should be present. If it is, removes the note.
    if (cell.isLegato()) {
        // This is not accepted in this format. Shows a warning.
        songExportResult.addWarning("Legato effect is not supported in AKM. The note is ignored.");

        cell.clearNote();
        cell.clearInstrument();
    }

    // NOTE: all the unknown effects have been removed already.

    // Pre-pass, for "note with effect". In this case, a special byte is encoded.
    // Important! Only the KNOWN effects must be tested! Else, encodes the flag, but not the effects, resulting in a possible crash on hardware.
    // --> Now that we have removed unknown effects just above, the security shouldn't be needed.
    const auto areEffectPresent = cell.areEffectsPresentAmong(knownEffects);
    const auto isNoteWithEffect = (areEffectPresent && cell.isNoteAndInstrument());
    if (isNoteWithEffect) {
        // Only a "note and effects" is encoded, no other bits.
        constexpr auto byteToEncode = static_cast<unsigned int>(valueNoteAndEffects);
        outputData.emplace_back(byteToEncode, "Note with effects flag.");
    }

    juce::String comment;
    if (cell.isEmpty()) {
        // Empty? Then nothing about the instrument.
        data = valueNoNoteMaybeEffects;
    } else {
        if (cell.isNoteAndInstrument()) {
            // Note that we encode the note normally, we don't even check for effects. This has been
            // done before, on the pre-pass.
            // A new instrument to encode? May be primary/secondary one.
            const auto readInstrumentIndex = cell.getInstrument().getValue();
            if (readInstrumentIndex == primaryDefaultInstrumentIndex) {
                instrumentFlags = instrumentFlagsPrimary;
                comment = "Primary instrument (" + juce::String(readInstrumentIndex) + "). ";
            } else if (readInstrumentIndex == secondaryDefaultInstrumentIndex) {
                instrumentFlags = instrumentFlagsSecondary;
                comment = "Secondary instrument (" + juce::String(readInstrumentIndex) + "). ";
            } else if (currentInstrumentIndex != readInstrumentIndex) {
                // New instrument (that is not a primary/secondary one).
                instrumentFlags = instrumentFlagsNewInstrument;
                comment = "New instrument (" + juce::String(readInstrumentIndex) + "). ";

                // This "escape" instrument resets the current Instrument, contrary to the primary ones.
                currentInstrumentIndex = readInstrumentIndex;
                instrumentEscape = true;
            } // Else, same instrument.

            // There is a note. Can it be referenced?
            const auto readNote = cell.getNote().getValue().getNote();
            // Finds the note in the list of most used note. If not there, it means it can not be referenced.
            auto noteReferenceIndex = CollectionUtil::findItemAndGetIndex(mostUsedNoteIndexes, readNote);
            noteEscape = (noteReferenceIndex < 0);

            // Encodes the reference index, if found.
            if (noteEscape) {
                // Escape? Only if the note is different!
                if (readNote != currentNote) {
                    data = valueNewEscapeNote;
                    // The current note becomes the read note.
                    currentNote = readNote;
                    comment += "New escaped note: " + juce::String(currentNote) + ". ";
                } else {
                    data = valueSameEscapeNote;
                    noteEscape = false; // Not an escape anymore!
                    comment += "Same escaped note: " + juce::String(currentNote) + ". ";
                }
            } else {
                data = noteReferenceIndex;
                comment += "Note reference (" + juce::String(noteReferenceIndex) + "). ";
                // If there are effects, there is one less note reference.
                jassert(areEffectsPresentInSong ? (noteReferenceIndex < valueNoNoteMaybeEffects) :
                        (noteReferenceIndex <= valueNoNoteMaybeEffects));
            }
        } else {
            // The cell is not empty and there is no note/instrument. So there are effects only.
            jassert(cell.hasEffects());
            data = valueNoNoteMaybeEffects;
            // The instrument flag is diverted: indicate there are effects.
            instrumentFlags = instrumentFlagsWhenNoNoteButEffect;

            comment += "Effect only. ";
        }
    }

    // What are the wait flags?
    if (mustEncodeWaits) {
        const auto waitsToEncode = accumulatedWait;
        if (waitsToEncode == primaryWait) {
            waitFlags = waitFlagsPrimary;
            comment += "Primary wait (" + juce::String(waitsToEncode) +").";
        } else if (waitsToEncode == secondaryWait) {
            waitFlags = waitFlagsSecondary;
            comment += "Secondary wait (" + juce::String(waitsToEncode) +").";
        } else if (waitsToEncode != currentWait) {
            waitFlags = waitFlagsNewWait;
            comment += "New wait (" + juce::String(waitsToEncode) +").";

            // This "escape" wait becomes the new one.
            currentWait = waitsToEncode;
            waitEscape = true;
        } // Else, same wait.
    }

    BitNumber bitNumber;
    bitNumber.injectNumber(data, 4U);
    bitNumber.injectNumber(instrumentFlags, 2U);
    bitNumber.injectNumber(mustEncodeWaits ? waitFlags : waitFlagsSameEscape, 2U);  // "same" is enough when not wanting to encode the instrument.
    jassert(data < 16);
    jassert(bitNumber.get() < 256);
    outputData.emplace_back(bitNumber.get(), comment);

    // Encodes a possible "note" escape.
    if (noteEscape) {
        outputData.emplace_back(cell.getNote().getValue().getNote(), "  Escape note value.");
    }

    // Encodes a possible "instrument" escape.
    if (instrumentEscape) {
        outputData.emplace_back(cell.getInstrument().getValue(), "  Escape instrument value.");
    }

    // Encodes the "escape" waits, if any, and if authorized.
    if (mustEncodeWaits && waitEscape) {
        jassert(accumulatedWait < 256);
        outputData.emplace_back(accumulatedWait, "  Escape wait value.");
    }

    // Encode the effects, if any.
    encodeEffects(outputData, songExportResult, cell);
}

void AkmSequenceEncoder::encodeEffects(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult, const Cell& cell) noexcept
{
    // No effects? Then stops.
    if (!cell.hasEffects()) {
        return;
    }

    // How many non-empty effects?
    const auto cellEffects = cell.getEffects().getPresentEffects();
    auto remainingNonEmptyEffectCount = cellEffects.size();

    // Browses each effect.
    for (const auto& cellEffect : cellEffects) {
        const auto moreEffects = (remainingNonEmptyEffectCount > 1U);

        const auto effect = cellEffect.getEffect();
        const auto effectLogicalValue = cellEffect.getEffectLogicalValue();
        switch (effect) {
            case Effect::volume : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectSetVolume);

                const auto invertedVolume = 15 - effectLogicalValue;
                const auto encodedEffectNumber = getEncodedEffectFirstByte(effectNumberVolume, invertedVolume, moreEffects);
                outputData.emplace_back(encodedEffectNumber, "   Volume effect, with inverted volume: " + juce::String(invertedVolume) + ".");
                break;
            }
            case Effect::reset : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectReset);

                // No need to invert the volume, it is already inverted in the track.
                const auto invertedVolume = effectLogicalValue;
                const auto encodedEffectNumber = getEncodedEffectFirstByte(effectNumberResetWithInvertedVolume, invertedVolume, moreEffects);
                outputData.emplace_back(encodedEffectNumber, "   Reset effect, with inverted volume: " + juce::String(invertedVolume) + ".");
                break;
            }
            case Effect::pitchUp :
                [[fallthrough]];
            case Effect::pitchDown :
                [[fallthrough]];
            case Effect::fastPitchUp :
                [[fallthrough]];
            case Effect::fastPitchDown : {
                encodeFastOrSlowPitch(outputData, songExportResult, effect, effectLogicalValue, moreEffects);
                break;
            }
            case Effect::arpeggioTable : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectArpeggioTable);

                encodeEffectWithMaybeEscapeValue(outputData, effectNumberArpeggioTableEffect, effectLogicalValue,
                                                 moreEffects, "   Arpeggio table effect " + juce::String(effectLogicalValue) + ".");
                break;
            }
            case Effect::pitchTable : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectPitchTable);

                encodeEffectWithMaybeEscapeValue(outputData, effectNumberPitchTableEffect, effectLogicalValue,
                                                 moreEffects, "   Pitch table effect " + juce::String(effectLogicalValue) + ".");
                break;
            }
            case Effect::forceArpeggioSpeed : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectForceArpeggioSpeed);

                encodeEffectWithMaybeEscapeValue(outputData, effectNumberForceArpeggioSpeedEffect, effectLogicalValue,
                                                 moreEffects, "   Force arpeggio speed effect " + juce::String(effectLogicalValue) + ".");
                break;
            }
            case Effect::forcePitchTableSpeed : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectForcePitchTableSpeed);

                encodeEffectWithMaybeEscapeValue(outputData, effectNumberForcePitchSpeedEffect, effectLogicalValue,
                                                 moreEffects, "   Force pitch speed effect " + juce::String(effectLogicalValue) + ".");
                break;
            }
            case Effect::forceInstrumentSpeed : {
                songExportResult.addFlag(PlayerConfigurationFlag::effectForceInstrumentSpeed);

                encodeEffectWithMaybeEscapeValue(outputData, effectNumberForceInstrumentSpeedEffect, effectLogicalValue,
                                                 moreEffects, "   Force instrument speed effect " + juce::String(effectLogicalValue) + ".");
                break;
            }

            case Effect::noEffect :
                // If empty effect, nothing to encode.
                jassertfalse;       // Such effects should have been discarded.
                break;
            case Effect::pitchGlide:
                [[fallthrough]];
            case Effect::volumeIn:
                [[fallthrough]];
            case Effect::volumeOut:
                [[fallthrough]];
            case Effect::arpeggio3Notes:
                [[fallthrough]];
            case Effect::arpeggio4Notes:
                [[fallthrough]];
            case Effect::legato:
                [[fallthrough]];
            default:
                jassertfalse;       // Should have been removed earlier, abnormal!!
                break;
        }

        // Skips the empty effect in the counting of remaining non-empty effects.
        if (effect != Effect::noEffect) {
            --remainingNonEmptyEffectCount;
        }
    }
}

void AkmSequenceEncoder::removeUnknownEffects(Cell& cell, SongExportResult& songExportResult) noexcept
{
    // No effects? Then stops.
    if (!cell.hasEffects()) {
        return;
    }
    const auto& cellEffects = cell.getEffects();

    // Browses each effect.
    for (auto effectIndex = 0; effectIndex < CellConstants::effectCount; ++effectIndex) {
        const auto cellEffect = cellEffects.getEffect(effectIndex);
        const auto effect = cellEffect.getEffect();
        if (effect == Effect::noEffect) {
            continue;
        }
        if (knownEffects.find(effect) == knownEffects.cend()) {
            // Unknown effects, erase it.
            cell.removeEffectFromIndex(effectIndex);

            songExportResult.addWarning("Effect \"" + EffectUtil::toDisplayedString(effect) + "\" is not supported in AKM, so will be ignored.");
        }
    }
}

int AkmSequenceEncoder::getEncodedEffectFirstByte(const unsigned int effectNumber, const int effectValueQuartet, const bool moreEffects) noexcept
{
    jassert((effectValueQuartet >= 0) && (effectValueQuartet <= 15));
    jassert(effectNumber < 8U);     // Only 3 bits!

    BitNumber bitNumber;
    bitNumber.injectBool(moreEffects);
    bitNumber.injectNumber(static_cast<int>(effectNumber), 3U);
    bitNumber.injectNumber(effectValueQuartet, 4U);

    jassert(bitNumber.get() < 256);
    return bitNumber.get();
}

void AkmSequenceEncoder::encodeFastOrSlowPitch(std::vector<std::pair<unsigned int, juce::String>>& outputData, SongExportResult& songExportResult,
                                               const Effect effect, const int effectValue, const bool moreEffects) noexcept
{
    const auto up = ((effect == Effect::pitchUp) || (effect == Effect::fastPitchUp));
    const auto fast = ((effect == Effect::fastPitchUp) || (effect == Effect::fastPitchDown));

    songExportResult.addFlag(up ? PlayerConfigurationFlag::effectPitchUp : PlayerConfigurationFlag::effectPitchDown);

    const auto pitchValue = (static_cast<unsigned int>(effectValue) & 0xfffU) << (fast ? 4U : 0U);

    const auto isPitchUsed = (pitchValue != 0U);

    const juce::String upDown = up ? "up" : "down";
    const auto comment = "   Pitch " + upDown + ": " + juce::String(pitchValue) + ".";

    outputData.emplace_back(getEncodedEffectFirstByte(effectNumberPitchUpDown, (isPitchUsed ? 0b1111 : 0), moreEffects), comment);

    // Encodes the pitch, if present.
    encodePitch(outputData, isPitchUsed, static_cast<int>(pitchValue), up);
}

void AkmSequenceEncoder::encodePitch(std::vector<std::pair<unsigned int, juce::String>>& outputData, const bool isPitchUsed, const int pitchValue, const bool up) noexcept
{
    // Encodes the pitch, if present. A POSITIVE Pitch is encoded, but the sign remains on bit 15.
    jassert(pitchValue >= 0);

    if (isPitchUsed) {
        auto encodedPitch = static_cast<unsigned int>(pitchValue);
        if (up) {                   // Pitch up, so the period must go down.
            encodedPitch |= 0x8000U;
        }

        outputData.emplace_back(encodedPitch & 0xffU, "   Pitch, LSB.");
        outputData.emplace_back(encodedPitch >> 8U, "   Pitch, MSB.");
    }
}

void AkmSequenceEncoder::encodeEffectWithMaybeEscapeValue(std::vector<std::pair<unsigned int, juce::String>>& outputData, const unsigned int effectNumber, int effectValue,
                                                          const bool moreEffects, const juce::String& comment) noexcept
{
    // Bigger than 14? Then encodes 15 as an escape value, then encodes the value itself.
    if (effectValue < 15) {
        outputData.emplace_back(getEncodedEffectFirstByte(effectNumber, effectValue, moreEffects), comment);
    } else {
        // Encodes the escape value (15), then the value itself.
        outputData.emplace_back(getEncodedEffectFirstByte(effectNumber, 15, moreEffects), "   Effect escape value, because >14.");
        outputData.emplace_back(effectValue, comment);
    }
}


}   // namespace arkostracker

