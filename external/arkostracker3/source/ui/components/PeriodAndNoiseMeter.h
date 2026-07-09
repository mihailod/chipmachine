#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

class PeriodAndNoiseMeterInput;

/**
 * Shows vertical bars according to the software/hardware periods and noise (one over the other).
 * Any bar that is not updated is decreased.
 * This view does NOT handle its refresh by itself. A Controller could handle a timer and ask for a refresh.
 */
class PeriodAndNoiseMeter final : public juce::Component
{
public:
    /** Constructor. */
    PeriodAndNoiseMeter() noexcept;

    /**
     * Updates the bars.
     * @param periodAndNoiseMeterInput the periods and volume to show.
     */
    void updateAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept;

    // Component method implementations.
    // ======================================
    void paint(juce::Graphics& g) override;

    void lookAndFeelChanged() override;

private:
    static constexpr auto barCount = 32;
    static const int barMaximumValue;                       // The value that is put in the bar.
    static const float barSeparationWidth;
    static const float alphaBars;

    static const int softwarePeriodDecreaseSpeed;
    static const int hardwarePeriodDecreaseSpeed;
    static const int noiseDecreaseSpeed;

    /**
     * Maps a value to a bar, which index is returned.
     * @param inputValue the input value.
     * @param maximumValue the maximum value is it supposed to reach.
     * @param logarithmic true if a logarithmic scale is used (more values at the "beginning").
     * @return the bar index.
     */
    static int mapValueToBar(int inputValue, int maximumValue, bool logarithmic) noexcept;

    /** Decreases the values of the bars (internal values only). */
    void decreaseAllBars() noexcept;
    /**
     * Decreases the values of the bars (internal values only).
     * @param array the array.
     * @param decreaseSpeed the speed decrease.
     */
    static void decreaseBars(std::array<int, static_cast<unsigned int>(barCount)>& array, int decreaseSpeed) noexcept;

    /**
     * Draws the bars.
     * @param g the Graphics object.
     * @param bars the bars to display.
     * @param barSizeWithSeparation the width of each bar, with separation.
     * @param barSize the width of each bar, without separation.
     * @param colour the Colour to fill the bar with.
     */
    void drawBars(juce::Graphics& g, const std::array<int, static_cast<unsigned int>(barCount)>& bars, float barSizeWithSeparation, float barSize, juce::Colour colour) const noexcept;

    /**
     * We accept a volume of 16, but stick to 15. This is logical: when a noise+hard or sound+hard comes, we show 15,
     * because the hardware bar is also shown to carry the information. We don't really want to reserve one level of data for the hard env.
     */
    const int maximumShownVolume;

    std::array<int, static_cast<unsigned int>(barCount)> softwarePeriodVolumes; // The volumes, from 0 to barMaximumValue.
    std::array<int, static_cast<unsigned int>(barCount)> hardwarePeriodVolumes; // The hardware periods, with a "fake" volume to manage it like the others.
    std::array<int, static_cast<unsigned int>(barCount)> noises;                // The noises, from 0 to barMaximumValue.

    juce::Colour softwareBarColor;
    juce::Colour hardwareBarColor;
    juce::Colour noiseBarColor;

    void setColoursFromLookAndFeel();
};

}   // namespace arkostracker

