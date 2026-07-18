#include "UiUtil.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

std::unique_ptr<juce::BubbleMessageComponent> UiUtil::createBubble(juce::Component& targetComponent, const juce::String& text,
    const bool locateAboveOnly, const int durationMs) noexcept
{
    return createBubble(targetComponent, *targetComponent.getTopLevelComponent(), text, locateAboveOnly, durationMs);
}

std::unique_ptr<juce::BubbleMessageComponent> UiUtil::createBubble(juce::Component& targetComponent, juce::Component& parentComponent, const juce::String& text,
    const bool locateAboveOnly, const int durationMs) noexcept
{
    const auto textColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);

    auto bubble = std::make_unique<juce::BubbleMessageComponent>(fadeOutLengthMs);

    juce::AttributedString attributedString;
    attributedString.setText(text);
    attributedString.setJustification(juce::Justification::centred);
    attributedString.setColour(textColor);
    parentComponent.addChildComponent(*bubble);
    if (locateAboveOnly) {
        bubble->setAllowedPlacement(juce::BubbleComponent::BubblePlacement::above);
    }
    bubble->showAt(&targetComponent, attributedString, durationMs);

    return bubble;
}

}   // namespace arkostracker
