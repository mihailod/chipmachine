#include "InformationView.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

InformationView::InformationView() noexcept:
        messageLabel(),
        displayedMessage(),
        displayedInError(false)
{
    setOpaque(true);

    messageLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(messageLabel);
}

void InformationView::displayMessage(const juce::String& message, const bool isError) noexcept
{
    // If no change, don't do anything.
    if ((message == displayedMessage) && (isError == displayedInError)) {
        return;
    }
    displayedMessage = message;
    displayedInError = isError;

    messageLabel.setText(message, juce::NotificationType::dontSendNotification);
    updateTextColor();
}

void InformationView::clearMessage() noexcept
{
    messageLabel.setText(juce::String(), juce::NotificationType::dontSendNotification);
}


// Component method implementations.
// =============================================

void InformationView::paint(juce::Graphics& g)
{
    const auto width = getWidth();
    const auto height = getHeight();

    // Fills the whole and adds a border.
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    const auto backgroundColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundUnfocused));
    const auto borderColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBorderUnfocused));

    g.fillAll(backgroundColor);
    g.setColour(borderColor);
    g.drawRect(0, 0, width, height, 1);
}

void InformationView::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    messageLabel.setBounds(0, 0, width, height);
}

void InformationView::lookAndFeelChanged()
{
    updateTextColor();
}

void InformationView::updateTextColor() noexcept
{
    const static auto errorColor = juce::Colours::red;

    // Uses the default text color if not an error.
    messageLabel.setColour(juce::Label::ColourIds::textColourId, displayedInError
                                                                 ? errorColor
                                                                 : juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId));
}

}   // namespace arkostracker
