#include "PitchBarCreator.h"

#include "../../../song/ExpressionConstants.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

const juce::Image PitchBarCreator::emptyImage = juce::Image();          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

BarAndCaptionData PitchBarCreator::createPitchBarData(const BarAreaSize areaSize, const int pitch, const bool withinLoop, const bool outOfBounds,
    const bool hovered, const bool selected, const bool withCursor) noexcept
{
    // This is the full pitch.
    int minimumValue;           // NOLINT(*-init-variables)
    int maximumValue;           // NOLINT(*-init-variables)
    switch (areaSize) {
        case BarAreaSize::small:
            maximumValue = 0xa;
            minimumValue = -maximumValue;
            break;
        case BarAreaSize::normal:
            maximumValue = 0x7f;
            minimumValue = -maximumValue;
            break;
        case BarAreaSize::extended: [[fallthrough]];
        case BarAreaSize::allUnusual: [[fallthrough]];
        default:
            minimumValue = ExpressionConstants::minimumPitch;
            maximumValue = ExpressionConstants::maximumPitch;
            break;
    }

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barPitchBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barPitch;
    constexpr auto retrig = false;
    constexpr auto generated = false;
    constexpr auto ignored = false;

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 pitch, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (pitch == 0) ? juce::String() : NumberUtil::signedHexToStringWithPrefix(pitch, juce::String(), true);      // Signed hex.
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

}   // namespace arkostracker
