#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** The header of each category in the Theme editor. */
class HeaderItem : public juce::TreeViewItem
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     * @param containSubItems true to show the "deploy" arrow.
     */
    explicit HeaderItem(juce::String text, bool containSubItems = true) noexcept;

    std::unique_ptr<juce::Component> createItemComponent() override;
    bool mightContainSubItems() override;

private:
    juce::String text;            // The text to display.
    bool containSubItems;
};


// =============================================

/** Represents what is actually displayed in each header. */
class HeaderComponent : public juce::Component
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     */
    explicit HeaderComponent(const juce::String& text) noexcept;

    void resized() override;

private:
    juce::Label label;
};

}   // namespace arkostracker

