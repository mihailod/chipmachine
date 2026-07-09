#include "PsgQuickEditController.h"

#include "../../../../controllers/SongController.h"
#include "../../../../song/cells/CellConstants.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "../../../../utils/StringUtil.h"

namespace arkostracker
{

PsgQuickEditController::PsgQuickEditController(SongController& pSongController) noexcept :
        songController(pSongController)
{
}

int PsgQuickEditController::getItemLength(const Id& itemId) const noexcept
{
    auto length = 0;
    songController.performOnConstInstrument(itemId, [&] (const Instrument& instrument) {
        jassert(instrument.getType() == InstrumentType::psgInstrument);
        length = instrument.getConstPsgPart().getLength();
    });

    return length;
}

juce::String PsgQuickEditController::validateNoise(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumNoise, PsgValues::maximumNoise);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("Noise quick edit"),
        [numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
            return inputCell.withNoise(numbersRef.at(static_cast<size_t>(iterationIndex)));
        });

    return { };
}

juce::String PsgQuickEditController::validateSidLowShelfVolume(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("SID low-shelf volume quick edit"),
        [numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
            return inputCell.withSidLowShelfVolume(numbersRef.at(static_cast<size_t>(iterationIndex)));
        });

    return { };
}

juce::String PsgQuickEditController::validateSidBalancePercent(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumPercent, PsgValues::maximumPercent, false);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("SID balance percent quick edit"),
        [numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
            return inputCell.withSidBalancePercent(numbersRef.at(static_cast<size_t>(iterationIndex)));
        });

    return { };
}

juce::String PsgQuickEditController::validateSidArpeggio(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::sidMinimumArpeggio, PsgValues::sidMaximumArpeggio);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("SID arpeggio quick edit"),
        [numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
            return inputCell.withSidArpeggio(numbersRef.at(static_cast<size_t>(iterationIndex)));
        });

    return { };
}

juce::String PsgQuickEditController::validateSidPitch(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::sidMinimumPitch, PsgValues::sidMaximumPitch);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("SID pitch quick edit"),
        [numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
            return inputCell.withSidPitch(numbersRef.at(static_cast<size_t>(iterationIndex)));
        });

    return { };
}

juce::String PsgQuickEditController::validateEnvelope(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    static const auto splitHardwareToEnvelopeResult = std::unordered_map<juce::String, EnvelopeResult> {
        { "g", { 16, 0x8 } },
        { "h", { 16, 0xa } },
        { "i", { 16, 0xc } },
        { "j", { 16, 0xe } },
    };

    // The values are 0 to j, space, comma.
    const auto splits = normalize(values);
    std::vector<EnvelopeResult> envelopeResults;
    for (const auto& rawSplit : splits) {
        const auto split = rawSplit.toLowerCase();
        auto error = false;

        // Is it a hardware envelope?
        if (auto iterator = splitHardwareToEnvelopeResult.find(split); iterator != splitHardwareToEnvelopeResult.cend()) {
            envelopeResults.emplace_back(iterator->second);
            continue;
        }

        const auto number = NumberUtil::signedHexStringToInt(split, error);
        if (error) {
            return juce::translate("Error in value \"" + split + "\".");
        }

        // Within range?
        if ((number < PsgValues::minimumVolume) || (number > PsgValues::maximumVolumeNoHard)) {
            return juce::translate("Value \"" + split + "\" must be within range.");
        }
        envelopeResults.emplace_back(number, 0);
    }

    // Valid.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(envelopeResults.size()),
        juce::translate("Envelope quick edit"),
        [envelopeResults] (const int iterationIndex, const PsgInstrumentCell& inputCell) {
            const auto& envelopeResult = envelopeResults.at(static_cast<size_t>(iterationIndex));
            if (envelopeResult.getVolume() != PsgValues::hardwareVolumeValue) {
                // Software. Forces to SoftwareOnly if there was a hardware sound.
                auto newCell = inputCell.withVolume(envelopeResult.getVolume());
                if (!newCell.isSoftware()) {
                    newCell = newCell.withLink(PsgInstrumentCellLink::softOnly);
                }
                return newCell;
            }

            // Hardware. Forces to SoftToHard if it was a software sound, with a default ratio.
            auto newCell = inputCell.withEnvelope(envelopeResult.getHardwareEnvelope());
            if (!newCell.isHardware()) {
                newCell = newCell.withLink(PsgInstrumentCellLink::softToHard).withRatio(PsgValues::defaultRatio);
            }
            return newCell;
        });

    return { };
}

juce::String PsgQuickEditController::validateSoundType(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    static const auto splitToSoundTypeResult = std::unordered_map<juce::String, PsgInstrumentCellLink> {
        { "-", PsgInstrumentCellLink::noSoftNoHard },
        { "s", PsgInstrumentCellLink::softOnly },
        { "h", PsgInstrumentCellLink::hardOnly },
        { "sh", PsgInstrumentCellLink::softToHard },
        { "hs", PsgInstrumentCellLink::hardToSoft },
        { "+", PsgInstrumentCellLink::softAndHard },
    };

    const auto splits = normalize(values);
    std::vector<PsgInstrumentCellLink> cellLinkResults;
    for (const auto& rawSplit : splits) {
        const auto split = rawSplit.toLowerCase();

        // Known value?
        auto iterator = splitToSoundTypeResult.find(split);
        if (iterator == splitToSoundTypeResult.cend()) {
            return juce::translate("Value \"" + split + "\" is illegal.");
        }

        cellLinkResults.emplace_back(iterator->second);
    }

    // Valid.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(cellLinkResults.size()),
        juce::translate("Sound type quick edit"),
        [cellLinkResults] (const int iterationIndex, const PsgInstrumentCell& inputCell) {
            const auto& link = cellLinkResults.at(static_cast<size_t>(iterationIndex));
            return inputCell.withLink(link);
        });

    return { };
}

juce::String PsgQuickEditController::validateSidIsActivatedAndRatio(const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto splits = normalize(values);
    std::vector<std::pair<bool, int>> isActivatedAndRatioResults;

    for (const auto& rawSplit : splits) {
        const auto split = rawSplit.toLowerCase();

        // Known value?
        if (split == "-") {     // Also in SidIsActivatedAndRatioManualEditDialog.cpp...
            isActivatedAndRatioResults.push_back({ false, 0 });
        } else {
            auto success = true;
            const auto ratio = StringUtil::stringToInt(split, success);
            success &= ((ratio >= PsgValues::sidMinimumRatio) && (ratio <= PsgValues::sidMaximumRatio));
            if (success) {
                isActivatedAndRatioResults.push_back({ true, ratio });
            } else {
                return juce::translate("Value \"" + split + "\" is illegal.");
            }
        }
    }

    // Valid.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(isActivatedAndRatioResults.size()),
        juce::translate("SID activation and ratio quick edit"),
        [isActivatedAndRatioResults] (const int iterationIndex, const PsgInstrumentCell& inputCell) {
            const auto [activated, ratio] = isActivatedAndRatioResults.at(static_cast<size_t>(iterationIndex));
            if (!activated) {
                // If not activated, only changes this flag, not the ratio.
                return inputCell.withSidActivated(false);
            }
            return inputCell.withSidActivated(true)
                            .withSidRatio(ratio);
        });

    return { };
}

juce::String PsgQuickEditController::validatePitch(const bool isPrimaryPitch, const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumPitch, PsgValues::maximumPitch);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("Pitch quick edit"),
        [isPrimaryPitch, numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
                                    return isPrimaryPitch
                                               ? inputCell.withPrimaryPitch(numbersRef.at(static_cast<size_t>(iterationIndex)))
                                               : inputCell.withSecondaryPitch(numbersRef.at(static_cast<size_t>(iterationIndex)));
                                });

    return { };
}

juce::String PsgQuickEditController::validateArpeggioNoteInOctave(bool isPrimaryArpeggioInNote, const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, 0, CellConstants::lastNoteInOctave);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("Note arpeggio quick edit"),
        [isPrimaryArpeggioInNote, numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
                                    return isPrimaryArpeggioInNote
                                               ? inputCell.withPrimaryArpeggioNoteInOctave(numbersRef.at(static_cast<size_t>(iterationIndex)))
                                               : inputCell.withSecondaryArpeggioNoteInOctave(numbersRef.at(static_cast<size_t>(iterationIndex)));
                                });

    return { };
}

juce::String PsgQuickEditController::validateArpeggioOctave(bool isPrimaryArpeggioOctave, const Id& instrumentId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("Arpeggio octave quick edit"),
        [isPrimaryArpeggioOctave, numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
                                    return isPrimaryArpeggioOctave
                                               ? inputCell.withPrimaryArpeggioOctave(numbersRef.at(static_cast<size_t>(iterationIndex)))
                                               : inputCell.withSecondaryArpeggioOctave(numbersRef.at(static_cast<size_t>(iterationIndex)));
                                });

    return { };
}

juce::String PsgQuickEditController::validatePeriod(bool isPrimaryPeriod, const Id& instrumentId, int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, PsgValues::minimumPeriod, PsgValues::maximumSoftwarePeriod);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnCells(instrumentId, barIndex, static_cast<int>(numbers.size()), juce::translate("Period quick edit"),
        [isPrimaryPeriod, numbersRef] (const int iterationIndex, const PsgInstrumentCell& inputCell) noexcept {
                                    return isPrimaryPeriod
                                               ? inputCell.withPrimaryPeriod(numbersRef.at(static_cast<size_t>(iterationIndex)))
                                               : inputCell.withSecondaryPeriod(numbersRef.at(static_cast<size_t>(iterationIndex)));
                                });

    return { };
}

}   // namespace arkostracker
