#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

/** Shows bars according to frequencies. This view does not update itself, its update method must be called. */
class FrequencyBarsView final : public juce::Component
{
public:
    class Controller
    {
    public:
        virtual ~Controller() = default;
        /** @return how many channels there are. */
        virtual int getChannelCount() const noexcept = 0;
        /** @return the indexes of the muted channels. */
        virtual std::unordered_set<int> getMutedChannelIndexes() const noexcept = 0;
        /** The user wants to mute a channel, which index is given. */
        virtual void onUserWantsToMuteChannel(int channelIndex) noexcept = 0;
        /** The user wants to solo a channel, which index is given. */
        virtual void onUserWantsToSoloChannel(int channelIndex) noexcept = 0;
        /** The user wants to unmute all. */
        virtual void onUserWantsToUnmuteAll() noexcept = 0;
        /** Switches to the meters view. */
        virtual void onUserWantsToSwitchView() noexcept = 0;
    };

    /**
     * Constructor.
     * @param controller the controller.
     */
    explicit FrequencyBarsView(Controller& controller) noexcept;

    /** Updates the view. */
    void update() noexcept;

    /** Called before new values are going to be received. */
    void onBeforeNewValues() noexcept;

    /**
     * Adds data to show. This will be stored up to the update method is called.
     * @param channelIndex the channel index where it happened.
     * @param softwarePeriod the software period.
     * @param volume the volume, from 0 to 15.
     */
    void addSoftwarePeriod(int channelIndex, int softwarePeriod, int volume) noexcept;

    /**
     * Adds data to show. This will be stored up to the update method is called.
     * @param hardwarePeriod the software period.
     */
    void addHardwarePeriod(int hardwarePeriod) noexcept;

    /**
     * Adds data to show. This will be stored up to the update method is called.
     * @param channelIndex the channel index where it happened.
     * @param noise the noise value.
     * @param volume the volume, from 0 to 16.
     */
    void addNoise(int channelIndex, int noise, int volume) noexcept;

    // juce::Component method implementations.
    // ==============================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    static const int gapWidth;
    static const int barWidthWithoutGap;
    static const int barWidthWithGap;
    static const int barHeightDecreaseSpeed;
    static const float barAlpha;
    static const float noiseAlphaBars;

    /**
     * @return a log value to the given value.
     * @param inputValue the input value.
     * @param maximumValue the maximum value is it supposed to reach.
     */
    static int logValue(int inputValue, int maximumValue) noexcept;

    /** Updates the internal colors from the current look'n'feel. */
    void updateInternalColorsAndNativeComponents() noexcept;

    /**
     * Decreases all the bars.
     * @param barIndexToHeight the bars and their height, the latter are decreased.
     */
    static void decreaseBars(std::unordered_map<int, int>& barIndexToHeight) noexcept;

    Controller& controller;

    std::unordered_map<int, std::unordered_map<int, int>> channelIndexToSoftwareBarIndexToHeight; // Height, in pixels, per shown bar, per channelIndex.
    std::unordered_map<int, int> hardwareBarIndexToHeight;
    std::unordered_map<int, std::unordered_map<int, int>> channelIndexToNoiseBarIndexToHeight;
    int softwareFrequencyRangePerBar;
    int hardwareFrequencyRangePerBar;
    float noiseBarWidthWithGap;
    int currentBarHeight;
    int frequencyBarCount;

    // Cached colors to use. Updated on look'n'feel change.
    juce::Colour colorMetersBackground;
    juce::Colour colorMetersSoftware;
    juce::Colour colorMetersHardware;
    juce::Colour colorMetersNoise;
};
}   // namespace arkostracker
