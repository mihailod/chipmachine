#include "ClickableLabel.h"

namespace arkostracker
{
ClickableLabel::ClickableLabel(const juce::String& pLabelText, std::function<void()> pOnClick) noexcept :
        juce::Label(juce::String(), pLabelText),
        onClick(std::move(pOnClick))
{
}

void ClickableLabel::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        onClick();
    }
}

}   // namespace arkostracker
