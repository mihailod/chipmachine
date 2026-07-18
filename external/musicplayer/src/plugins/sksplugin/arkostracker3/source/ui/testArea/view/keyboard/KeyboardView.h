#pragma once

#include <functional>
#include <unordered_map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "KeyView.h"

namespace arkostracker 
{

/**
 * Shows the whole keyboard (piano roll). It manages the "press" and "release" states after a while via a timer.
 * This view manages its own horizontal Viewport.
 */
class KeyboardView final : public juce::Component,
                           juce::Timer
{
public:
    /**
     * Constructor.
     * @param keyCount how many keys there are.
     * @param onKeyboardKeyPressed called when a keyboard key is pressed.
     */
    KeyboardView(int keyCount, std::function<void(int)> onKeyboardKeyPressed) noexcept;

    /** Destructor. */
    ~KeyboardView() override;

    /**
     * Shows a key as pressed, for a certain amount of time.
     * @param noteNumber the note number.
     */
    void showKeyAsPressed(int noteNumber) noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static const float whiteKeyDefaultWidth;            // Width of a white key for a zoom ratio of 1.
    static const float blackKeyRatioHeight;             // Ratio of the height of a black key, compared to the white (that is, the total height).
    static const float blackKeyRatioWidth;              // Ratio of the width of a black key, compared to the white.

    static const float scrollWheelMultiplier;           // Multiplier of the mouse wheel value to scroll.
    static const float scrollWheelMultiplierWhenZoomed; // Multiplier of the mouse wheel value to scroll, so that it is faster when zoomed.

    static const int keyRefreshMs;                      // How often to check if a key must be released, in ms.
    static const int pressedDurationMs;                 // How long a key must be shown as "pressed", in ms. However, this is only checked according to the duration above.

    static const float minimumZoom;
    static const float maximumZoom;

    // juce::Timer method implementations.
    // ====================================
    void timerCallback() override;

    /**
     * Builds the keyboard, sets the keys location. It must be called on resize and zooming so that the keys are relocated and resized.
     * Nothing happens if the height and zoom are the same.
     * The listener, if any, is notified if the zoom changed.
     */
    void buildKeyboard() noexcept;

    /**
     * Called when a keyboard key is pressed.
     * @param keyNumber the key number.
     */
    void onKeyboardKeyPressed(int keyNumber) const noexcept;

    /** Called regularly, checks the keys that must be released. Updates the UI. */
    void manageKeyRelease() noexcept;

    /**
     * Shows a keyboard key as "pressed" or "released".
     * @param noteNumber the note number (>=0). If out of the keyboard boundary, nothing happens.
     * @param pressed true if pressed, false if released.
     */
    void setKeyboardKeyState(int noteNumber, bool pressed) const noexcept;

    const int keyCount;
    std::function<void(int)> keyboardKeyPressed;

    float zoom;                                         // The current zoom of the keyboard.
    juce::Viewport viewport;                            // Viewport for the keys.
    Component keysContainer;                            // Single Component to hold the keys, put in the Viewport.
    std::vector<std::unique_ptr<KeyView>> keys;         // All the keys.
    int previousHeight;                                 // The latest height used when displaying. Used to know if it is worth redrawing!
    float previousZoom;                                 // The latest zoom used when displaying. Used to know if it is worth redrawing!

    std::unordered_map<int, juce::int64> keyNumberToReleaseTimestamp;   // Links a key number to the timestamp the key must be released.
};

}   // namespace arkostracker
