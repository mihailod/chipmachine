#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A key of a keyboard. The client is responsible for setting the right size and setting the state of the keys (pressed or not). */
class KeyView final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param noteNumber the note number, from 0 to 119 for example.
     * @param white true if the key is white, else black.
     * @param letter the letter to show. May be empty. Must have only one character, else it won't be shown.
     * @param onVirtualKeyboardKeyPressed the callback with the note number that has been clicked. The UI is unchanged.
     */
    KeyView(int noteNumber, bool white, const juce::String& letter, std::function<void(int)> onVirtualKeyboardKeyPressed) noexcept;

    /** @return whether the key is white or not. */
    bool isWhite() const noexcept;

    /**
     * Sets the state of the key: pressed or not.
     * @param pressed true to see the key pressed, false for released.
     */
    void setPressedState(bool pressed) noexcept;

    // Component method implementations.
    // =======================================================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    static const juce::Colour outsideColorForWhiteKey;
    static const juce::Colour insideColorForWhiteKey;
    static const juce::Colour insideColorForBlackKey;
    static const juce::Colour textColor;
    static const juce::Colour pressedColor;

    std::function<void(int)> onVirtualKeyboardKeyPressed;

    bool pressed;                           // The current state: pressed or not.
    int noteNumber;                         // The note number, from 0 to 119 for example.
    bool white;                             // True if the key is white, else black.
    bool showLetter;                        // True if the letter is shown.
    juce::String letter;                    // The letter to show. May be empty.

    juce::Colour outsideColor;              // Color used for the outside.
    juce::Colour insideColor;               // Color used for the inside.
};

}   // namespace arkostracker
