#include "FrequencyBarsView.h"

#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

const int FrequencyBarsView::gapWidth = 1;
const int FrequencyBarsView::barWidthWithoutGap = 4;
const int FrequencyBarsView::barWidthWithGap = barWidthWithoutGap + gapWidth;

const int FrequencyBarsView::barHeightDecreaseSpeed = 10;
const float FrequencyBarsView::barAlpha = 0.7F;
const float FrequencyBarsView::noiseAlphaBars = 0.5F;

FrequencyBarsView::FrequencyBarsView(Controller& pController) noexcept :
        controller(pController),
        channelIndexToSoftwareBarIndexToHeight(),
        hardwareBarIndexToHeight(),
        channelIndexToNoiseBarIndexToHeight(),
        softwareFrequencyRangePerBar(),
        hardwareFrequencyRangePerBar(),
        noiseBarWidthWithGap(),
        currentBarHeight(),
        frequencyBarCount(),
        // Colors are set below.
        colorMetersBackground(),
        colorMetersSoftware(),
        colorMetersHardware(),
        colorMetersNoise()
{
    setOpaque(true);

    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);

    updateInternalColorsAndNativeComponents();
}

void FrequencyBarsView::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    frequencyBarCount = width / barWidthWithGap;
    const auto newSoftwareFrequencyRangePerBar = PsgValues::maximumSoftwarePeriod / frequencyBarCount;
    const auto newHardwareFrequencyRangePerBar = PsgValues::maximumHardwarePeriod / frequencyBarCount;

    // There aren't many noise values, so they are spread in the width.
    const auto newNoiseBarWidthPerBar = static_cast<float>(width) / PsgValues::maximumNoise;

    auto changeInHeight = false;
    if ((currentBarHeight != height)) {
        currentBarHeight = height;
        changeInHeight = true;
    }

    if (changeInHeight || (softwareFrequencyRangePerBar != newSoftwareFrequencyRangePerBar)) {
        softwareFrequencyRangePerBar = newSoftwareFrequencyRangePerBar;
        channelIndexToSoftwareBarIndexToHeight.clear();
    }

    if (changeInHeight || (hardwareFrequencyRangePerBar != newHardwareFrequencyRangePerBar)) {
        hardwareFrequencyRangePerBar = newHardwareFrequencyRangePerBar;
        hardwareBarIndexToHeight.clear();
    }

    if (changeInHeight || !juce::exactlyEqual(noiseBarWidthWithGap, newNoiseBarWidthPerBar)) {
        noiseBarWidthWithGap = newNoiseBarWidthPerBar;
        channelIndexToNoiseBarIndexToHeight.clear();
    }
}

void FrequencyBarsView::onBeforeNewValues() noexcept
{
    // Have the bars being decreased every frame by a few pixels BEFORE the new values, so that they naturally go down or stay if a new volume is set.
    for (auto& [_, barIndexToHeight] : channelIndexToSoftwareBarIndexToHeight) {
        decreaseBars(barIndexToHeight);
    }

    decreaseBars(hardwareBarIndexToHeight);

    for (auto& [_, barIndexToHeight] : channelIndexToNoiseBarIndexToHeight) {
        decreaseBars(barIndexToHeight);
    }
}

void FrequencyBarsView::update() noexcept
{
    if ((softwareFrequencyRangePerBar <= 0) || (noiseBarWidthWithGap <= 0)) {
        // Nothing can be determined if the size is not right.
        return;
    }

    repaint();
}

void FrequencyBarsView::addSoftwarePeriod(const int channelIndex, const int softwarePeriod, const int volume) noexcept
{
    if ((softwareFrequencyRangePerBar <= 0) || (currentBarHeight <= 0) || (frequencyBarCount <= 0)) {
        // Nothing can be determined if the size is not right.
        return;
    }

    if (volume == 0) {      // Don't bother!
        return;
    }

    // Makes sure the channel entry exists.
    if (channelIndexToSoftwareBarIndexToHeight.find(channelIndex) == channelIndexToSoftwareBarIndexToHeight.cend()) {
        channelIndexToSoftwareBarIndexToHeight.insert({ channelIndex, { } });
    }

    const auto loggedPeriod = logValue(softwarePeriod, PsgValues::maximumSoftwarePeriod);
    const auto invertedPeriod = NumberUtil::correctNumber(PsgValues::maximumSoftwarePeriod - loggedPeriod, 0, PsgValues::maximumSoftwarePeriod);
    const auto targetBarIndex = NumberUtil::correctNumber(invertedPeriod / softwareFrequencyRangePerBar, 0, frequencyBarCount - 1);
    auto& softwareBarIndexToHeight = channelIndexToSoftwareBarIndexToHeight.at(channelIndex);

    // What height?
    constexpr auto maximumValue = PsgValues::maximumVolumeNoHard;
    const auto correctedVolume = NumberUtil::correctNumber(volume, 0, maximumValue);
    const auto barHeight = static_cast<int>(static_cast<double>(currentBarHeight) * correctedVolume / maximumValue);

    softwareBarIndexToHeight[targetBarIndex] = barHeight;
}

void FrequencyBarsView::addHardwarePeriod(const int hardwarePeriod) noexcept
{
    if ((hardwareFrequencyRangePerBar <= 0) || (currentBarHeight <= 0) || (frequencyBarCount <= 0)) {
        // Nothing can be determined if the size is not right.
        return;
    }

    const auto loggedPeriod = logValue(hardwarePeriod, PsgValues::maximumHardwarePeriod);
    const auto invertedPeriod = NumberUtil::correctNumber(PsgValues::maximumHardwarePeriod - loggedPeriod, 0, PsgValues::maximumHardwarePeriod);
    const auto targetBarIndex = NumberUtil::correctNumber(invertedPeriod / hardwareFrequencyRangePerBar, 0, frequencyBarCount - 1);

    // What height?
    const auto barHeight = static_cast<int>(static_cast<double>(currentBarHeight));

    hardwareBarIndexToHeight[targetBarIndex] = barHeight;
}

void FrequencyBarsView::addNoise(const int channelIndex, const int noise, const int volume) noexcept
{
    if ((noiseBarWidthWithGap <= 0) || (currentBarHeight <= 0)) {
        // Nothing can be determined if the size is not right.
        return;
    }

    // Makes sure the channel entry exists.
    if (channelIndexToNoiseBarIndexToHeight.find(channelIndex) == channelIndexToNoiseBarIndexToHeight.cend()) {
        channelIndexToNoiseBarIndexToHeight.insert({ channelIndex, { } });
    }

    const auto invertedNoise = NumberUtil::correctNumber(PsgValues::maximumNoise - noise, 0, PsgValues::maximumNoise);
    const auto targetBarIndex = invertedNoise;
    auto& noiseBarIndexToHeight = channelIndexToNoiseBarIndexToHeight.at(channelIndex);

    // What height?
    constexpr auto maximumValue = PsgValues::maximumVolumeNoHard;
    const auto correctedVolume = NumberUtil::correctNumber(volume, 0, maximumValue);
    const auto barHeight = static_cast<int>(static_cast<double>(currentBarHeight) * correctedVolume / maximumValue);

    noiseBarIndexToHeight[targetBarIndex] = barHeight;
}


// juce::Component method implementations.
// ==============================================

void FrequencyBarsView::paint(juce::Graphics& g)
{
    g.fillAll(colorMetersBackground);

    const auto height = currentBarHeight;
    if ((noiseBarWidthWithGap <= 0) || (height <= 0)) {
        return;
    }

    g.setColour(colorMetersSoftware.withAlpha(barAlpha));
    for (const auto& [_, barIndexToHeight] : channelIndexToSoftwareBarIndexToHeight) {
        for (const auto& [barIndex, barHeight] : barIndexToHeight) {
            if (barHeight <= 0) {
                continue;
            }
            const auto barX = barIndex * barWidthWithGap;
            const auto barY = height - barHeight;

            g.fillRect(barX, barY, barWidthWithoutGap, barHeight);
        }
    }

    g.setColour(colorMetersHardware.withAlpha(barAlpha));
    for (const auto& [barIndex, barHeight] : hardwareBarIndexToHeight) {
        if (barHeight <= 0) {
            continue;
        }
        const auto barX = barIndex * barWidthWithGap;
        const auto barY = height - barHeight;

        g.fillRect(barX, barY, barWidthWithoutGap, barHeight);
    }

    g.setColour(colorMetersNoise.withAlpha(noiseAlphaBars));
    const auto noiseBarWidthWithoutGap = noiseBarWidthWithGap - gapWidth;
    for (const auto& [_, barIndexToHeight] : channelIndexToNoiseBarIndexToHeight) {
        for (const auto& [barIndex, barHeight] : barIndexToHeight) {
            if (barHeight <= 0) {
                continue;
            }
            const auto barX = static_cast<float>(barIndex) * noiseBarWidthWithGap;
            const auto barY = static_cast<float>(height - barHeight);

            g.fillRect(barX, barY, noiseBarWidthWithoutGap, static_cast<float>(barHeight));
        }
    }
}

void FrequencyBarsView::lookAndFeelChanged()
{
    updateInternalColorsAndNativeComponents();
}

void FrequencyBarsView::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isRightButtonDown()) {
        return;
    }

    const auto channelCount = controller.getChannelCount();
    const auto mutedChannelIndexes = controller.getMutedChannelIndexes();

    // Builds the context menu.
    juce::PopupMenu popupMenu;
    // Mute/unmute.
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        const auto isChannelMuted = (mutedChannelIndexes.find(channelIndex) != mutedChannelIndexes.cend());
        popupMenu.addItem(juce::translate(isChannelMuted ? "Unmute channel " : "Mute channel ") + juce::String(channelIndex + 1), true, false,
                          [&, channelIndex] {
                              controller.onUserWantsToMuteChannel(channelIndex);
                          });
    }
    popupMenu.addSeparator();

    // Unmutes all.
    if (!mutedChannelIndexes.empty()) {
        popupMenu.addItem(juce::translate("Unmute all"), true, false,
                          [&] {
                              controller.onUserWantsToUnmuteAll();
                          });
        popupMenu.addSeparator();
    }

    // Solos/unsolos.
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        popupMenu.addItem(juce::translate("Toggle solo channel ") + juce::String(channelIndex + 1), true, false,
                          [&, channelIndex] {
                              controller.onUserWantsToSoloChannel(channelIndex);
                          });
    }

    popupMenu.show();
}


// ==============================================

int FrequencyBarsView::logValue(const int inputValue, const int maximumValue) noexcept
{
    if (inputValue <= 1) {      // Log 1 is 0...
        return 0;
    }
    // TO IMPROVE: Same log calculation as PeriodAndNoiseMeter::mapValueToBar, with the same inconvenience.
    // Maybe not perfect, because even the highest note doesn't reach the full right... Maybe cheat and change the maximum Value? Then re-corrects the value.
    return NumberUtil::toLog(inputValue, maximumValue, maximumValue);
}

void FrequencyBarsView::updateInternalColorsAndNativeComponents() noexcept
{
    const auto& defaultLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    colorMetersBackground = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersBackground));
    colorMetersSoftware = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersSoftwareBar));
    colorMetersHardware = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersHardwareBar));
    colorMetersNoise = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoiseBar));
}

void FrequencyBarsView::decreaseBars(std::unordered_map<int, int>& barIndexToHeight) noexcept
{
    for (auto& [barIndex, barHeight] : barIndexToHeight) {
        if (barHeight > 0) {
            barHeight -= barHeightDecreaseSpeed;
        }
    }
}

}   // namespace arkostracker
