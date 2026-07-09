#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** The header of each Location. */
class LinkHeaderItem final : public juce::TreeViewItem
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     * @param containSubItems true to show the "deploy" arrow.
     * @param headerId an ID for the client to know which one was clicked.
     * @param showLinkButton true to show the link button.
     * @param onLinkButtonClicked called when the button is clicked.
     */
    LinkHeaderItem(juce::String text, bool containSubItems, int headerId, bool showLinkButton, std::function<void(int pHeaderId)> onLinkButtonClicked) noexcept;

    std::unique_ptr<juce::Component> createItemComponent() override;
    bool mightContainSubItems() override;

private:
    juce::String text;              // The text to display.
    bool containSubItems;
    int headerId;
    bool showLinkButton;
    std::function<void(int headerId)> onLinkButtonClicked;
};


// =============================================

/** Represents what is actually displayed in each header. */
class LinkHeaderComponent final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     * @param headerId an ID for the client to know which one was clicked.
     * @param showLinkButton true to show the link button.
     * @param onLinkButtonClicked called when the button is clicked.
     */
    LinkHeaderComponent(const juce::String& text, int headerId, bool showLinkButton, std::function<void(int pHeaderId)> onLinkButtonClicked) noexcept;

    void resized() override;

private:
    juce::Label label;
    juce::TextButton linkButton;
    int headerId;
    std::function<void(int headerId)> onLinkButtonClicked;
};

}   // namespace arkostracker
