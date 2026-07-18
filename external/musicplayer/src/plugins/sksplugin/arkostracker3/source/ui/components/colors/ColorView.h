#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * Shows a square with a color, and a small border, which goes bigger when selected.
 * When not selected, an invisible border is displayed.
 */
class ColorView final : public juce::Component
{
public:
    /** Listener to be aware of a click. */
    class Listener {
    public:
        /** Destructor.*/
        virtual ~Listener() = default;
        /**
         * Called when the color has been clicked.
         * @param color the color of this Component.
         * @param viewId the ID of the view (a rank, etc.).
         */
        virtual void onColorClicked(const juce::Colour& color, int viewId) noexcept = 0;

        /** Called when the color is double-clicked. The base implementation does nothing. */
        virtual void onColorDoubleClicked() noexcept
        {
            // Nothing done.
        }
    };

    static constexpr auto preferredSize = 30;                   // The width/height this component should have.
    static constexpr auto preferredSizeSmall = 25;              // The width/height this component should have.

    /**
     * Constructor.
     * @param listener the listener to be aware of a click.
     * @param color the shown color.
     * @param viewId the ID of the view (a rank, etc.).
     * @param selected true if selected.
     */
    ColorView(Listener& listener, juce::Colour color, int viewId = 0, bool selected = false) noexcept;

    /**
     * Sets the color and repaints, if necessary.
     * @param color the color.
     */
    void setColorAndRepaint(juce::Colour color) noexcept;

    /** @return the color. */
    const juce::Colour& getColor() const noexcept;

    /**
     * Shows the Component as selected or not, and repaints it.
     * @param selected true if selected.
     */
    void setSelectedAndRepaint(bool selected) noexcept;

    // Component method implementations.
    // ===================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void paint(juce::Graphics& g) override;

private:
    static constexpr int borderSelectSize = 2;                  // The border size, when selected.
    static constexpr int borderNotSelectedSize = 1;             // The border size, when not selected.

    /** Sets the internal border colors from the current color. */
    void setBorderColorFromCurrentColor() noexcept;

    juce::Colour borderNotSelectedColor;                    // The color of the border, when not selected.
    juce::Colour borderSelectedColor;                       // The color of the border, when selected.

    ColorView::Listener& listener;                          // A listener to be aware of a click.
    juce::Colour color;                                     // The shown color.
    bool selected;                                          // True if selected.

    int viewId;                                             // The ID of the view (a rank, etc.).
};

}   // namespace arkostracker
