#include "ButtonTwoStates.h"

namespace arkostracker 
{

ButtonTwoStates::ButtonTwoStates(ButtonTwoStates::Listener& pListener, const juce::Image& pImageOn, const juce::Image& pImageOff,
                                 const bool pInitialState, const int pButtonIndex) noexcept :
        DrawableButton(juce::String(), DrawableButton::ButtonStyle::ImageRaw),
        stateOffImage(),
        stateOnImage(),
        state(pInitialState),
        buttonIndex(pButtonIndex),
        buttonTwoStatesListener(&pListener)
{
    stateOffImage.setImage(pImageOff);
    stateOnImage.setImage(pImageOn);

    setNewStateAndUpdateImage(state, true);
}

void ButtonTwoStates::mouseDown(const juce::MouseEvent& mouseEvent)
{
    if (mouseEvent.mods.isLeftButtonDown() || mouseEvent.mods.isRightButtonDown()) {
        buttonTwoStatesListener->onButtonTwoStatesClicked(mouseEvent, !state, buttonIndex);
    }
}

void ButtonTwoStates::setNewStateAndUpdateImage(const bool newState, const bool forceUpdate) noexcept
{
    // Any change?
    if (!forceUpdate && (newState == state)) {
        return;
    }

    state = newState;
    setImages(newState ? &stateOnImage : &stateOffImage);
}

}   // namespace arkostracker
