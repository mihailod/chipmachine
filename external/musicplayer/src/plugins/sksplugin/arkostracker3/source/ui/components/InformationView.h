#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

/** A rectangle with a background and a text. It also has a I/E image to switch between information and effect context. */
class InformationView final : public juce::Component
{
public:
    /** Constructor. */
    InformationView() noexcept;

    /**
     * Displays a message.
     * @param message a message.
     * @param isError true if it is an error message.
     */
    void displayMessage(const juce::String& message, bool isError) noexcept;

    /** Clears the message. */
    void clearMessage() noexcept;

    // Component method implementations.
    // =============================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    /** Updates the text color from the current error state. */
    void updateTextColor() noexcept;

    juce::Label messageLabel;

    juce::String displayedMessage;
    bool displayedInError;
};

}   // namespace arkostracker
