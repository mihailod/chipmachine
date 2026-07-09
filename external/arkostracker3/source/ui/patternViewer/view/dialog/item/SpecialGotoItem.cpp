#include "SpecialGotoItem.h"

#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

SpecialGotoItem::SpecialGotoItem(juce::String pText, Listener& pListener, const int pPositionIndex) :
        text(std::move(pText)),
        listener(pListener),
        positionIndex(pPositionIndex)
{
}

std::unique_ptr<juce::Component> SpecialGotoItem::createItemComponent()
{
    return std::make_unique<SpecialGotoComponent>(listener, text, positionIndex);
}

bool SpecialGotoItem::mightContainSubItems()
{
    return false;
}


// ===========================================

SpecialGotoComponent::SpecialGotoComponent(SpecialGotoItem::Listener& pListener, const juce::String& pText, const int pPositionIndex) noexcept :
        label(juce::String(), pText),
        gotoButton(juce::translate("Go")),
        listener(pListener),
        positionIndex(pPositionIndex)
{
    setInterceptsMouseClicks(false, true);     // Else the parent doesn't get the clicks.

    addAndMakeVisible(label);
    addAndMakeVisible(gotoButton);

    gotoButton.onClick = [&] {
        listener.onSpecialGotoClicked(positionIndex);
    };

    label.setJustificationType(juce::Justification::Flags::verticallyCentred);
}


// Component method implementations.
// ==========================================

void SpecialGotoComponent::resized()
{
    constexpr auto gotoButtonWidth = 40;
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::smallMargins;

    const auto height = getHeight();

    const auto gotoButtonX = getWidth() - gotoButtonWidth - margins;
    const auto buttonHeight = getHeight();

    gotoButton.setBounds(gotoButtonX, 0, gotoButtonWidth, buttonHeight);
    label.setBounds(0, 0, gotoButtonX - smallMargins, height);
}

}   // namespace arkostracker
