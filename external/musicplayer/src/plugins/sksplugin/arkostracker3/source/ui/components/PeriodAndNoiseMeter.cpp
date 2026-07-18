#include "PeriodAndNoiseMeter.h"

#include "../../ui/lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "PeriodAndNoiseMeterInput.h"

namespace arkostracker 
{

const int PeriodAndNoiseMeter::barMaximumValue = 31;
const float PeriodAndNoiseMeter::barSeparationWidth = 2.0F;
const float PeriodAndNoiseMeter::alphaBars = 0.7F;

const int PeriodAndNoiseMeter::softwarePeriodDecreaseSpeed = 3;
const int PeriodAndNoiseMeter::noiseDecreaseSpeed = softwarePeriodDecreaseSpeed;
const int PeriodAndNoiseMeter::hardwarePeriodDecreaseSpeed = barMaximumValue;       // Must stay only one frame.

PeriodAndNoiseMeter::PeriodAndNoiseMeter() noexcept :
        maximumShownVolume(PsgValues::maximumVolumeNoHard),
        softwarePeriodVolumes(),
        hardwarePeriodVolumes(),
        noises(),
        softwareBarColor(),         // Set below.
        hardwareBarColor(),
        noiseBarColor()
{
    setColoursFromLookAndFeel();
}

void PeriodAndNoiseMeter::updateAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept // NOLINT(*-function-cognitive-complexity)
{
    decreaseAllBars();

    const auto& softwarePeriodAndVolumeOptional = periodAndNoiseMeterInput.getSoftwarePeriodAndVolumeOptional();
    const auto& noiseAndVolumeOptional = periodAndNoiseMeterInput.getNoiseAndVolumeOptional();
    const auto& hardwarePeriodOptional = periodAndNoiseMeterInput.getHardwarePeriodOptional();

    // Any value?
    if (softwarePeriodAndVolumeOptional.isAbsent() && hardwarePeriodOptional.isAbsent() && noiseAndVolumeOptional.isAbsent()) {
        repaint();          // To apply the decreasing.
        return;
    }

    // Manages the software period.
    if (softwarePeriodAndVolumeOptional.isPresent()) {
        const auto softwarePeriodAndVolume = softwarePeriodAndVolumeOptional.getValue();
        const auto softwarePeriod = softwarePeriodAndVolume.first;
        const auto volume = std::min(softwarePeriodAndVolume.second, maximumShownVolume);   // Even if volume is 16, stick to 15.
        jassert((softwarePeriod >= PsgValues::minimumPeriod) && (softwarePeriod <= PsgValues::maximumSoftwarePeriod));

        const auto mappedBarIndex = static_cast<unsigned int>(mapValueToBar(softwarePeriod, PsgValues::maximumSoftwarePeriod, true));

        const auto value = volume * barMaximumValue / maximumShownVolume;
        softwarePeriodVolumes.at(mappedBarIndex) = value;
    }

    // Manages the noise.
    if (noiseAndVolumeOptional.isPresent()) {
        const auto noiseAndVolume = noiseAndVolumeOptional.getValue();
        const auto noise = noiseAndVolume.first;
        const auto volume = std::min(noiseAndVolume.second, maximumShownVolume);            // Even if volume is 16, stick to 15.

        jassert((noise >= PsgValues::minimumNoise) && (noise <= PsgValues::maximumNoise));

        const auto mappedBarIndex = static_cast<unsigned int>(mapValueToBar(noise, PsgValues::maximumNoise, false));

        const auto value = volume * barMaximumValue / maximumShownVolume;
        noises.at(mappedBarIndex) = value;
    }

    // Manages the hardware period.
    if (hardwarePeriodOptional.isPresent()) {
        const auto hardwarePeriod = hardwarePeriodOptional.getValue();
        jassert((hardwarePeriod >= PsgValues::minimumPeriod) && (hardwarePeriod <= PsgValues::maximumHardwarePeriod));

        const auto mappedBarIndex = static_cast<unsigned int>(mapValueToBar(hardwarePeriod, PsgValues::maximumHardwarePeriod, true));

        hardwarePeriodVolumes.at(mappedBarIndex) = barMaximumValue;
    }

    repaint();
}

int PeriodAndNoiseMeter::mapValueToBar(int inputValue, const int maximumValue, const bool logarithmic) noexcept
{
    inputValue = NumberUtil::correctNumber(inputValue, 0, maximumValue);

    if (logarithmic) {
        // Maybe not perfect, because even the highest note doesn't reach the full right... Maybe cheat and change the maximum Value? Then re-corrects the value.
        inputValue = NumberUtil::toLog(inputValue, maximumValue, maximumValue);
    }

    // Inverts the value because all the numbers are periods. More user-friendly to show "low" frequencies to the left.
    inputValue = maximumValue - inputValue;

    return static_cast<int>(std::round(static_cast<double>(inputValue * (barCount - 1)) / static_cast<double>(maximumValue)));
}

void PeriodAndNoiseMeter::decreaseAllBars() noexcept
{
    decreaseBars(softwarePeriodVolumes, softwarePeriodDecreaseSpeed);
    decreaseBars(hardwarePeriodVolumes, hardwarePeriodDecreaseSpeed);
    decreaseBars(noises, noiseDecreaseSpeed);
}

void PeriodAndNoiseMeter::decreaseBars(std::array<int, static_cast<unsigned int>(barCount)>& array, const int decreaseSpeed) noexcept
{
    jassert(decreaseSpeed > 0);

    for (size_t i = 0U; i < static_cast<size_t>(barCount); ++i) {
        const auto value = std::max(array.at(i) - decreaseSpeed, 0);
        array.at(i) = value;
    }
}


// Component method implementations.
// ======================================

void PeriodAndNoiseMeter::paint(juce::Graphics& g)
{
    const auto width = static_cast<float>(getWidth());

    const auto barSizeWithSeparation = width / static_cast<float>(barCount);
    const auto barSize = barSizeWithSeparation - barSeparationWidth;
    if (barSize > 0.0F) {           // When minimized horizontally, not enough room. It's better not to display anything.
        drawBars(g, softwarePeriodVolumes, barSizeWithSeparation, barSize, softwareBarColor);
        drawBars(g, noises, barSizeWithSeparation, barSize, noiseBarColor);
        drawBars(g, hardwarePeriodVolumes, barSizeWithSeparation, barSize, hardwareBarColor);
    }
}

void PeriodAndNoiseMeter::lookAndFeelChanged()
{
    setColoursFromLookAndFeel();
    // No need to update, there is a timer for that.
}


// ======================================

void PeriodAndNoiseMeter::setColoursFromLookAndFeel()
{
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    softwareBarColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersSoftwareBar));
    hardwareBarColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersHardwareBar));
    noiseBarColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoiseBar));
}

void PeriodAndNoiseMeter::drawBars(juce::Graphics& g, const std::array<int, static_cast<unsigned int>(barCount)>& bars,
                                   const float barSizeWithSeparation, const float barSize, juce::Colour colour) const noexcept
{
    const auto height = static_cast<float>(getHeight());
    const auto bottom = height;

    const auto scaleY = height / static_cast<float>(barMaximumValue);

    colour = colour.withAlpha(alphaBars);           // Forces the alpha.
    g.setColour(colour);

    auto x = 0.0F;
    for (size_t i = 0U; i < static_cast<size_t>(barCount); ++i) {
        const auto value = bars.at(i);
        jassert(value <= barMaximumValue);

        if (value > 0) {
            const auto barHeight = static_cast<float>(value) * scaleY;
            const auto y = bottom - barHeight;

            g.fillRect(x, y, barSize, barHeight);
        }

        x += barSizeWithSeparation;
    }
}

}   // namespace arkostracker
