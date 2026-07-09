#include "GotoItem.h"

#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

GotoItem::GotoItem(juce::String pText, Listener& pListener, const int pPositionIndex, const int pChannelIndex) :
        text(std::move(pText)),
        listener(pListener),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex)
{
}

std::unique_ptr<juce::Component> GotoItem::createItemComponent()
{
    return std::make_unique<GotoComponent>(listener, text, positionIndex, channelIndex);
}

bool GotoItem::mightContainSubItems()
{
    return false;
}


// ===========================================

GotoComponent::GotoComponent(GotoItem::Listener& pListener, const juce::String& pText, const int pPositionIndex, const int pChannelIndex) noexcept :
        label(juce::String(), pText),
        gotoButton(juce::translate("Go")),
        listener(pListener),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex)
{
    setInterceptsMouseClicks(false, true);     // Else the parent doesn't get the clicks.

    addAndMakeVisible(label);
    addAndMakeVisible(gotoButton);

    gotoButton.onClick = [&] {
        listener.onGotoClicked(positionIndex, channelIndex);
    };

    label.setJustificationType(juce::Justification::Flags::verticallyCentred);
}


// Component method implementations.
// ==========================================

void GotoComponent::resized()
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
