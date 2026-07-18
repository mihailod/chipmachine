#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * Image Button which with two states, which can change when desired. A callback is available to
 * know when it is clicked. Nothing happens when it is clicked. It is up to client to
 * set its new state.
 * NOTE: This button has no border and is only the given image.
 * If you want to have a "normal" Button around, use ButtonWithImage instead.
 */
class ButtonTwoStates final : public juce::DrawableButton
{
public:
    /** Callback to know if the button has been clicked. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the button is clicked. Nothing is changed about it.
         * @param mouseEvent the mouse event.
         * @param newState the new state (on/off).
         * @param buttonIndex the button index, if provided at construction.
         */
        virtual void onButtonTwoStatesClicked(const juce::MouseEvent& mouseEvent, bool newState, int buttonIndex) noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param imageOn the image when on.
     * @param imageOff the image when off.
     * @param initialState the initial state.
     * @param buttonIndex an index, sent in the callback.
     */
    ButtonTwoStates(ButtonTwoStates::Listener& listener, const juce::Image& imageOn,
                    const juce::Image& imageOff, bool initialState = true, int buttonIndex = 0) noexcept;

    // DrawableButton methods override.
    // ================================
    void mouseDown(const juce::MouseEvent& e) override;

    /**
     * Sets a new state and updates the image. Nothing happens if the state is the same.
     * This does NOT notify the listener.
     * @param newState the new state.
     * @param forceUpdate true to force the update.
     */
    void setNewStateAndUpdateImage(bool newState, bool forceUpdate = false) noexcept;

private:
    juce::DrawableImage stateOffImage;              // The "off" stage image.
    juce::DrawableImage stateOnImage;               // The "on" stage image.
    bool state;                                     // The state (off/on).
    const int buttonIndex;                          // The button index. May be handy for the UI.

    Listener* buttonTwoStatesListener;              // The listener.
};

}   // namespace arkostracker
