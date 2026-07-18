#pragma once

#include <memory>

#include "../../components/ColoredImage.h"
#include "Tab.h"

namespace arkostracker 
{

/** A Tab as seen in the top of a Container. */
class TabWithIcon final : public Tab,
                          public juce::TooltipClient
{
public:

    /**
     * Constructor.
     * @param panelType the panel this Tab represents.
     * @param listener the listener to be aware of the events from the tab.
     * @param tooltip the tooltip to show.      // TO IMPROVE this should be dynamic in case the user changes it.
     */
    TabWithIcon(PanelType panelType, Listener& listener, juce::String tooltip) noexcept;

    PanelType getPanelType() const noexcept override;

    // Component method implementations.
    // =================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;
    void lookAndFeelChanged() override;

    // TooltipClient method implementations.
    // =====================================
    juce::String getTooltip() override;

private:
    /**
     * Creates the tab icon.
     * @param imageData the data of the image.
     * @param dataSize the size of the data.
     * @return the icon.
     */
    static std::unique_ptr<ColoredImage> createIcon(const void* imageData, size_t dataSize) noexcept;

    /** @return the color to use to draw the icon. */
    static juce::Colour getIconColor() noexcept;

    PanelType panelType;                        // The panel type linked to this tab.
    Listener& listener;                         // The listener to be aware of the events from the tab.
    std::unique_ptr<ColoredImage> icon;         // The icon to show.
    juce::String toolTip;                       // The tooltip to show.
};

}   // namespace arkostracker
