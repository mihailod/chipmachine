#include "MixChannelsPanel.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

MixChannelsPanel::MixChannelsPanel(int pChannelCount, int pSourceChannel, int pDestinationChannel, std::function<void()> pOkCallback,
                                   std::function<void()> pCancelCallback) noexcept :
    ModalDialog(juce::translate("Channels to mix"), 420, 120, std::move(pOkCallback), std::move(pCancelCallback),
                true, true, false),
    mixLabel(juce::String(), juce::translate("Mix channel ")),
    toLabel(juce::String(), juce::translate("to channel ")),
    sourceSlider(),
    destinationSlider()
{

    // Sets up the UI.
    constexpr auto slidersHeight = 35;
    constexpr auto slidersWidth = 90;
    constexpr auto labelsYOffset = 5;
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto y = margins;

    mixLabel.setBounds(margins, y + labelsYOffset, 100, labelsHeight);
    sourceSlider.setBounds(mixLabel.getRight() + margins, y, slidersWidth, slidersHeight);
    toLabel.setBounds(sourceSlider.getRight() + margins, y + labelsYOffset, 80, labelsHeight);
    destinationSlider.setBounds(toLabel.getRight() + margins, y, slidersWidth, slidersHeight);

    sourceSlider.setSliderStyle(juce::Slider::IncDecButtons);
    sourceSlider.setRange(1, pChannelCount, 1);
    destinationSlider.setSliderStyle(juce::Slider::IncDecButtons);
    destinationSlider.setRange(1, pChannelCount, 1);
    sourceSlider.addListener(this);
    destinationSlider.addListener(this);

    sourceSlider.setValue(pSourceChannel + 1, juce::NotificationType::dontSendNotification);       // + 1 because inputs are indexes.
    destinationSlider.setValue(pDestinationChannel + 1, juce::NotificationType::sendNotificationAsync);        // Security.

    addComponentToModalDialog(mixLabel);
    addComponentToModalDialog(toLabel);
    addComponentToModalDialog(sourceSlider);
    addComponentToModalDialog(destinationSlider);
}

int MixChannelsPanel::getSourceChannel() const noexcept
{
    return static_cast<int>(sourceSlider.getValue()) - 1;
}
int MixChannelsPanel::getDestinationChannel() const noexcept
{
    return static_cast<int>(destinationSlider.getValue()) - 1;
}


// Slider::Listener method implementations.
// =====================================================

void MixChannelsPanel::sliderValueChanged(juce::Slider* slider)
{
    // Only checks if the source is different from the destination.
    jassert((slider == &sourceSlider) || (slider == &destinationSlider));
    (void)slider;           // To avoid a warning in Release.

    setOkButtonEnable(!juce::exactlyEqual(sourceSlider.getValue(), destinationSlider.getValue()));
}

}   // namespace arkostracker
