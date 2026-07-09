#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../song/psg/PsgType.h"
#include "../../utils/WithParent.h"

namespace arkostracker 
{

class MetersController;

/**
 * Shows the waveform, volume, hardware envelope and noise of a channel.
 * This view is stupid and does not manage the timer at which it is refreshed.
 */
class ChannelView final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param metersController the Controller.
     * @param channelIndex convenient index to return to the controllers when sending it an event (>=0).
     * @param psgType the type of the PSG (AY/YM).
     * @param muted true if muted.
     * @param signalBufferSize the size of the buffer of the signal. Warning, it MUST be related to what the generator produces!
     */
    ChannelView(MetersController& metersController, int channelIndex, PsgType psgType, bool muted, size_t signalBufferSize) noexcept;

    /**
     * Sets the PSG type.
     * @param psgType the type (AY/YM).
     */
    void setPsgStyle(PsgType psgType) noexcept;

    /**
     * Changes the muted state of the View. It only changes its internal state and
     * refreshes the UI. Nothing happens if the state is the same.
     */
    void changeMuteState(bool isMuted) noexcept;

    /** Called to refresh the ChannelView. This is typically called by the MetersView when its timer is ticking. */
    void refreshChannelView() noexcept;

    /** @return the channel index represented by this View. */
    int getChannelIndex() const noexcept;

    /** @return the block that contains the signal to display. Use this to fill it. */
    juce::HeapBlock<uint16_t>& getDisplayBuffer() noexcept;

    // Component methods override.
    // ===========================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    static constexpr auto volumeCount = 32;                  // How many volumes there are (32 because YM half-step).

    static constexpr auto labelChannelNumberBaseSize = 9;    // Base width of the label where the channel number is written (without any digit inside).
    static constexpr auto widthAddedPerDigit = 10;           // Width to the label each time a digit is added.
    static constexpr auto labelChannelNumberHeight = 20;     // Height of the label where the channel number is written.
    static constexpr auto labelChannelNumberBorderSize = 5;  // Width/height around the label where the channel number is written.

    /** Updates the internal colors from the current look'n'feel. */
    void updateInternalColorsAndNativeComponents() noexcept;

    MetersController& metersController;                 // The controller.

    const int channelIndex;                             // Index of the channel (>=0).

    juce::HeapBlock<juce::uint16> bufferToDisplay;      // The data to display, from 0 to 65535.
    size_t signalBufferSize;                            // How large the display buffer are.

    bool muted;                                         // True if the channel is muted.

    juce::Label labelChannelNumber;                     // Where the channel number is written.

    PsgType psgType;                                    // The type of the PSG (AY/YM).

    /** Mouse listener when the Label is clicked. */
    class LabelMouseListener final :  public MouseListener,
                                      public WithParent<ChannelView>
    {
    public:
        explicit LabelMouseListener(ChannelView& parent) : WithParent(parent) {}
        void mouseDown(const juce::MouseEvent& event) override;
    };
    LabelMouseListener labelMouseListener;

    // Cached colors to use. Updated on look'n'feel change.
    juce::Colour colorMetersBackground;
    juce::Colour colorMetersScaleLines;
    juce::Colour colorMetersLabelBackground;
    juce::Colour colorMetersLabel;
    juce::Colour colorMetersShape;
    juce::Colour colorMetersMuteFilter;
};

}   // namespace arkostracker
