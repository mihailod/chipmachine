#pragma once



#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

/** Dialog box to select a source/destination channel. */
class MixChannelsPanel : public ModalDialog,
                         juce::Slider::Listener
{
public:
    /**
     * Constructor.
     * @param channelCount how many channels are available in the source MODule.
     * @param sourceChannel the channel index of the source (>=0!).
     * @param destinationChannel the channel index of the destination (>=0!).
     * @param okCallback callback when OK is pressed.
     * @param cancelCallback callback when Cancel is pressed.
     */
    MixChannelsPanel(int channelCount, int sourceChannel, int destinationChannel, std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

    /** Returns the chosen source channel (>=0). It is valid. */
    int getSourceChannel() const noexcept;
    /** Returns the chosen destination channel (>=0). It is valid. */
    int getDestinationChannel() const noexcept;


private:
    // Slider::Listener method implementations.
    // =====================================================
    void sliderValueChanged(juce::Slider* slider) override;

    juce::Label mixLabel;                       // "mix" label.
    juce::Label toLabel;                        // "to" label.
    juce::Slider sourceSlider;                  // Where to choose the source channel.
    juce::Slider destinationSlider;             // Where to choose the destination channel.
};

}   // namespace arkostracker

