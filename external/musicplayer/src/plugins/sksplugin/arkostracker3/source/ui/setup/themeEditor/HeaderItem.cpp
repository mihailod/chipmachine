#include "HeaderItem.h"

namespace arkostracker 
{

HeaderItem::HeaderItem(juce::String pText, const bool pContainSubItems) noexcept :
        text(std::move(pText)),
        containSubItems(pContainSubItems)
{
}

std::unique_ptr<juce::Component> HeaderItem::createItemComponent()
{
    return std::make_unique<HeaderComponent>(text);
}

bool HeaderItem::mightContainSubItems()
{
    return containSubItems;
}


// ===========================================

HeaderComponent::HeaderComponent(const juce::String& text) noexcept :
        label(juce::String(), text)
{
    label.setJustificationType(juce::Justification::verticallyCentred);

    addAndMakeVisible(label);
}

void HeaderComponent::resized()
{
    label.setBounds(0, 0, getWidth(), getHeight());
}


}   // namespace arkostracker

