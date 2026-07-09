#include "LinkHeaderItem.h"

#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

LinkHeaderItem::LinkHeaderItem(juce::String pText, const bool pContainSubItems, const int pHeaderId, const bool pShowLinkButton,
    std::function<void(int headerId)> pOnLinkButtonClicked) noexcept :
        text(std::move(pText)),
        containSubItems(pContainSubItems),
        headerId(pHeaderId),
        showLinkButton(pShowLinkButton),
        onLinkButtonClicked(std::move(pOnLinkButtonClicked))
{
}

std::unique_ptr<juce::Component> LinkHeaderItem::createItemComponent()
{
    return std::make_unique<LinkHeaderComponent>(text, headerId, showLinkButton, onLinkButtonClicked);
}

bool LinkHeaderItem::mightContainSubItems()
{
    return containSubItems;
}


// ===========================================

LinkHeaderComponent::LinkHeaderComponent(const juce::String& text, const int pHeaderId, const bool showLinkButton,
    std::function<void(int headerId)> pOnLinkButtonClicked) noexcept :
        label(juce::String(), text),
        linkButton(juce::translate("Link")),
        headerId(pHeaderId),
        onLinkButtonClicked(std::move(pOnLinkButtonClicked))
{
    label.setJustificationType(juce::Justification::verticallyCentred);

    linkButton.onClick = [&]{
        onLinkButtonClicked(headerId);
    };
    addAndMakeVisible(label);

    if (showLinkButton) {
        addAndMakeVisible(linkButton);
    }
}

void LinkHeaderComponent::resized()
{
    constexpr auto buttonWidth = 60;
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::smallMargins;

    const auto height = getHeight();
    const auto linkButtonX = getWidth() - buttonWidth - margins;
    const auto buttonHeight = getHeight();

    linkButton.setBounds(linkButtonX, 0, buttonWidth, buttonHeight);
    label.setBounds(0, 0, linkButtonX - smallMargins, height);
}

}   // namespace arkostracker
