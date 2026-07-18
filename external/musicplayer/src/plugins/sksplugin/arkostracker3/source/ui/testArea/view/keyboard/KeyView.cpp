#include "KeyView.h"

namespace arkostracker 
{

const juce::Colour KeyView::outsideColorForWhiteKey(0xff404040);
const juce::Colour KeyView::insideColorForWhiteKey(0xffe0e0e0);
const juce::Colour KeyView::insideColorForBlackKey(0xff101010);
const juce::Colour KeyView::textColor(0xff808080);
const juce::Colour KeyView::pressedColor(0xff808080);

KeyView::KeyView(const int pNoteNumber, const bool pWhite, const juce::String& pLetter, std::function<void(int)> pOnVirtualKeyboardKeyPressed) noexcept :
    onVirtualKeyboardKeyPressed(std::move(pOnVirtualKeyboardKeyPressed)),
    pressed(false),
    noteNumber(pNoteNumber),
    white(pWhite),
    showLetter(pLetter.isNotEmpty()),
    letter(showLetter ? pLetter : juce::String()),         // Only the first letter matters.
    outsideColor(outsideColorForWhiteKey),
    insideColor(white ? insideColorForWhiteKey : insideColorForBlackKey)
{
}

bool KeyView::isWhite() const noexcept
{
    return white;
}

void KeyView::setPressedState(const bool newPressed) noexcept
{
    // Repaints the key if the state has changed.
    if (pressed != newPressed) {
        pressed = newPressed;
        repaint();
    }
}


// Component method implementations.
// =======================================================

void KeyView::paint(juce::Graphics& g)
{
    const auto width = getWidth();
    const auto height = getHeight();
    constexpr auto thickness = 1;

    // First draws the inside.
    g.setColour(pressed ? pressedColor : insideColor);
    g.fillRect(thickness, thickness, width, height);

    // Then the outside (only for white keys).
    if (white) {
        g.setColour(outsideColor);
        g.drawRect(0, 0, width, height, thickness);

        // Finally, adds the text (only for white keys).
        if (showLetter) {
            g.setColour(textColor);
            g.drawText(letter, 0, 0, width, height, juce::Justification::centredBottom, false);
        }
    }
}

void KeyView::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        onVirtualKeyboardKeyPressed(noteNumber);
    }
}

}   // namespace arkostracker
