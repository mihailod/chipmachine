#pragma once

#include "ColorSwatch.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

/** Shows the color name and a swatch, in the Theme editor. */
class ColorItem : public juce::TreeViewItem
{
public:
    /** To be aware of clicks. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;
        /**
         * Called when the color swatch is clicked.
         * @param colorId the color id represented by this line.
         */
        virtual void onColorSwatchClicked(int colorId) = 0;
    };

    /**
     * Constructor.
     * @param colorId the color related to this item.
     * @param colorName the name of the color to display.
     * @param listener to be aware of the clicks.
     */
    ColorItem(int colorId, juce::String colorName, Listener& listener);

    /** @return the colorId related to this item. */
    int getColorId() const noexcept;

    /** Finds the color from the color ID of this ColorItem. */
    juce::Colour findColourFromTheme() const noexcept;

    // TreeViewItem method implementations.
    // ==========================================
    void itemClicked(const juce::MouseEvent& event) override;
    std::unique_ptr<juce::Component> createItemComponent()  override;
    bool mightContainSubItems() override;

private:
    int colorId;
    juce::String colorName;           // The name of the color to display.
    Listener& listener;
};


// =============================================

/** What is displayed in the item. Note that it will update itself on theme change! */
class ColorComponent : public juce::Component
{
public:
    /**
     * Constructor.
     * @param colorName the name of the color to display.
     * @param colorItem the holder, to get the dynamic data.
     */
    ColorComponent(const juce::String& colorName, ColorItem& colorItem) noexcept;

    // Component method implementations.
    // ==========================================
    void resized() override;
    void lookAndFeelChanged() override;

private:
    juce::Label label;
    ColorItem& colorItem;
    ColorSwatch colorSwatch;
};

}   // namespace arkostracker
