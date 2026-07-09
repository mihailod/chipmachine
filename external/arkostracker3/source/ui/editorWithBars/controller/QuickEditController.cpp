#include "QuickEditController.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/Id.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "../../../utils/StringUtil.h"
#include "../view/manualEdit/EnvelopeManualEditDialog.h"
#include "../view/manualEdit/PositiveDecimalManualEditDialog.h"
#include "../view/manualEdit/PositiveManualEditDialog.h"
#include "../view/manualEdit/RelativeManualEditDialog.h"
#include "../view/manualEdit/SidIsActivatedAndRatioManualEditDialog.h"
#include "../view/manualEdit/SoundTypeManualEditDialog.h"

namespace arkostracker
{

void QuickEditController::edit(const Id& itemId, const AreaType areaType, const int barIndex) noexcept
{
    jassert(dialog == nullptr);

    // Quick edit is only allowed up to the length (included).
    if (barIndex > getItemLength(itemId)) {
        return;
    }

    switch (areaType) {
        case AreaType::soundType:
            dialog = std::make_unique<SoundTypeManualEditDialog>(
                [&, itemId, barIndex] (const juce::String& input) {
                    return validateSoundType(itemId, barIndex, input);
                },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::noise:
            dialog = std::make_unique<PositiveManualEditDialog>(juce::translate("Noise quick edit"), PsgValues::minimumNoise, PsgValues::maximumNoise,
                [&, itemId, barIndex] (const juce::String& input) { return validateNoise(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::envelope:
            dialog = std::make_unique<EnvelopeManualEditDialog>(
                [&, itemId, barIndex] (const juce::String& input) { return validateEnvelope(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::primaryPitch: [[fallthrough]];
        case AreaType::secondaryPitch:
            dialog = std::make_unique<RelativeManualEditDialog>(juce::translate("Pitch quick edit"), PsgValues::minimumPitch, PsgValues::maximumPitch,
                [&, areaType, itemId, barIndex] (const juce::String& input) {
                    return validatePitch((areaType == AreaType::primaryPitch), itemId, barIndex, input);
                },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::primaryArpeggioNoteInOctave: [[fallthrough]];
        case AreaType::secondaryArpeggioNoteInOctave:
            dialog = std::make_unique<PositiveManualEditDialog>(juce::translate("Arpeggio note quick edit"),
                0, CellConstants::lastNoteInOctave,
                [&, areaType, itemId, barIndex] (const juce::String& input) {
                    return validateArpeggioNoteInOctave((areaType == AreaType::primaryArpeggioNoteInOctave), itemId, barIndex, input);
                },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::primaryArpeggioOctave: [[fallthrough]];
        case AreaType::secondaryArpeggioOctave:
           dialog = std::make_unique<RelativeManualEditDialog>(juce::translate("Arpeggio octave quick edit"),
               PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave,
               [&, areaType, itemId, barIndex] (const juce::String& input) {
                   return validateArpeggioOctave((areaType == AreaType::primaryArpeggioOctave), itemId, barIndex, input);
               },
               [&] { onLeaveDialog(); });
            break;
        case AreaType::primaryPeriod: [[fallthrough]];
        case AreaType::secondaryPeriod: {
            constexpr auto minimumValue = PsgValues::minimumPeriod;
            constexpr auto maximumValue = PsgValues::maximumSoftwarePeriod;              // TO IMPROVE: this is a shortcut... Hardware period is FFFF. Enough for users??
            dialog = std::make_unique<PositiveManualEditDialog>(juce::translate("Period quick edit"),
               minimumValue, maximumValue,
               [&, areaType, itemId, barIndex] (const juce::String& input) {
                   return validatePeriod((areaType == AreaType::primaryPeriod), itemId, barIndex, input);
               },
               [&] { onLeaveDialog(); },
               juce::translate("Enter hex values from " + NumberUtil::signedHexToStringWithPrefix(minimumValue, juce::String()).toLowerCase() +
                                                              " to " + NumberUtil::signedHexToStringWithPrefix(maximumValue, juce::String()).toLowerCase() + ", separated by comma or space. 0 for \"auto\"."),
               500);
            break;
        }
        case AreaType::sidLowShelfVolume:
            dialog = std::make_unique<PositiveManualEditDialog>(juce::translate("SID low-shelf volume quick edit"),
                PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard,
                [&, itemId, barIndex] (const juce::String& input) { return validateSidLowShelfVolume(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::sidBalancePercent:
            dialog = std::make_unique<PositiveDecimalManualEditDialog>(juce::translate("SID balance quick edit"),
                PsgValues::minimumPercent, PsgValues::maximumPercent,
                [&, itemId, barIndex] (const juce::String& input) { return validateSidBalancePercent(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::sidIsActivatedAndRatio:
            dialog = std::make_unique<SidIsActivatedAndRatioManualEditDialog>(PsgValues::sidMinimumRatio, PsgValues::sidMaximumRatio,
                [&, itemId, barIndex] (const juce::String& input) { return validateSidIsActivatedAndRatio(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::sidArpeggio:
            dialog = std::make_unique<PositiveManualEditDialog>(juce::translate("SID arpeggio quick edit"),
                PsgValues::sidMinimumArpeggio, PsgValues::sidMaximumArpeggio,
                [&, itemId, barIndex] (const juce::String& input) { return validateSidArpeggio(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;
        case AreaType::sidPitch:
            dialog = std::make_unique<RelativeManualEditDialog>(juce::translate("SID pitch quick edit"),
                PsgValues::sidMinimumPitch, PsgValues::sidMaximumPitch,
                [&, itemId, barIndex] (const juce::String& input) { return validateSidPitch(itemId, barIndex, input); },
                [&] { onLeaveDialog(); });
            break;

        default:
            jassertfalse;                   // Not managed.
            dialog = nullptr;
            break;
    }

    if (dialog != nullptr) {
        dialog->setup();
    }
}

void QuickEditController::onLeaveDialog() noexcept
{
    dialog.reset();
}

std::vector<juce::String> QuickEditController::normalize(const juce::String& values) noexcept
{
    const auto correctedValues = values.replaceCharacter(',', ' ');
    return StringUtil::split(correctedValues, " ");
}

std::pair<juce::String, std::vector<int>> QuickEditController::validateValues(const juce::String& values, const int minimumValue, const int maximumValue,
    const bool isHex) noexcept
{
    // The values are 0 to 1f, space, comma.
    const auto splits = normalize(values);
    std::vector<int> numbers;
    for (const auto& split : splits) {
        auto error = false;
        auto success = true;
        const auto number = isHex ? NumberUtil::signedHexStringToInt(split, error) : StringUtil::stringToInt(split, success);
        if (error && !success) {
            return { juce::translate("Error in value \"" + split + "\"."), { } };
        }

        // Within range?
        if ((number < minimumValue) || (number > maximumValue)) {
            return { juce::translate("Value \"" + split + "\" must be within range."), { } };
        }
        numbers.push_back(number);
    }

    return { juce::String(), numbers };
}

}   // namespace arkostracker
