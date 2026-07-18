#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

/** Shows a Text with a position and channel index, and a Goto Button. */
class GotoItem final : public juce::TreeViewItem
{
public:
    /** To be aware of clicks. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when the goto is clicked. */
        virtual void onGotoClicked(int positionIndex, int channelIndex) = 0;
    };

    /**
     * Constructor.
     * @param text the text to display.
     * @param listener to be aware of the clicks.
     * @param positionIndex the position index linked to this item.
     * @param channelIndex the channel index linked to this item.
     */
    GotoItem(juce::String text, Listener& listener, int positionIndex, int channelIndex);

    // TreeViewItem method implementations.
    // ==========================================
    std::unique_ptr<juce::Component> createItemComponent()  override;
    bool mightContainSubItems() override;

private:
    juce::String text;
    Listener& listener;
    int positionIndex;
    int channelIndex;
};


// =============================================

/** What is displayed in the item. */
class GotoComponent final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param listener called when events occur.
     * @param text the text to display.
     * @param positionIndex the position index linked to this item.
     * @param channelIndex the channel index linked to this item.
     */
    GotoComponent(GotoItem::Listener& listener, const juce::String& text, int positionIndex, int channelIndex) noexcept;

    // Component method implementations.
    // ==========================================
    void resized() override;

private:
    juce::Label label;
    juce::TextButton gotoButton;
    GotoItem::Listener& listener;
    int positionIndex;
    int channelIndex;
};

}   // namespace arkostracker
