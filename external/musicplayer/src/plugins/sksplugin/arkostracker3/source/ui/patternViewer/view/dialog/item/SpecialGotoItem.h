#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

/** Shows a Text with a position index, and a Goto Button. */
class SpecialGotoItem final : public juce::TreeViewItem
{
public:
    /** To be aware of clicks. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when the goto is clicked. */
        virtual void onSpecialGotoClicked(int positionIndex) = 0;
    };

    /**
     * Constructor.
     * @param text the text to display.
     * @param listener to be aware of the clicks.
     * @param positionIndex the position index linked to this item.
     */
    SpecialGotoItem(juce::String text, Listener& listener, int positionIndex);

    // TreeViewItem method implementations.
    // ==========================================
    std::unique_ptr<juce::Component> createItemComponent()  override;
    bool mightContainSubItems() override;

private:
    juce::String text;
    Listener& listener;
    int positionIndex;
};


// =============================================

/** What is displayed in the item. */
class SpecialGotoComponent final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param listener called when events occur.
     * @param text the text to display.
     * @param positionIndex the position index linked to this item.
     */
    SpecialGotoComponent(SpecialGotoItem::Listener& listener, const juce::String& text, int positionIndex) noexcept;

    // Component method implementations.
    // ==========================================
    void resized() override;

private:
    juce::Label label;
    juce::TextButton gotoButton;
    SpecialGotoItem::Listener& listener;
    int positionIndex;
};

}   // namespace arkostracker
