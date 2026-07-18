#include "TabWithIcon.h"

#include <BinaryData.h>

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

TabWithIcon::TabWithIcon(const PanelType pPanelType, Listener& pListener, juce::String pTooltip) noexcept :
    Tab(),
    panelType(pPanelType),
    listener(pListener),
    icon(),
    toolTip(std::move(pTooltip))
{
    setOpaque(true);

    std::unique_ptr<ColoredImage> localIcon;

    // Shows the right icon from the PanelType.
    switch (panelType) {
        default:
            jassertfalse;                                   // Never supposed to happen! Forgot to declare an icon?
        case PanelType::meters: [[fallthrough]];            // These panels never show their icon.
        case PanelType::linker: [[fallthrough]];
        case PanelType::testArea: [[fallthrough]];
        case PanelType::dummy:
            localIcon = createIcon(BinaryData::PanelDummy_png, static_cast<size_t>(BinaryData::PanelDummy_pngSize));
            break;
        case PanelType::arpeggioTable:
            localIcon = createIcon(BinaryData::PanelArpeggioTable_png, static_cast<size_t>(BinaryData::PanelArpeggioTable_pngSize));
            break;
        case PanelType::arpeggioList:
            localIcon = createIcon(BinaryData::PanelArpeggioList_png, static_cast<size_t>(BinaryData::PanelArpeggioList_pngSize));
            break;
        case PanelType::pitchTable:
            localIcon = createIcon(BinaryData::PanelPitchTable_png, static_cast<size_t>(BinaryData::PanelPitchTable_pngSize));
            break;
        case PanelType::pitchList:
            localIcon = createIcon(BinaryData::PanelPitchList_png, static_cast<size_t>(BinaryData::PanelPitchList_pngSize));
            break;
        case PanelType::instrumentEditor:
            localIcon = createIcon(BinaryData::PanelInstrument_png, static_cast<size_t>(BinaryData::PanelInstrument_pngSize));
            break;
        case PanelType::instrumentList:
            localIcon = createIcon(BinaryData::PanelInstrumentList_png, static_cast<size_t>(BinaryData::PanelInstrumentList_pngSize));
            break;
        case PanelType::patternViewer:
            localIcon = createIcon(BinaryData::PanelPattern_png, static_cast<size_t>(BinaryData::PanelPattern_pngSize));
            break;
    }

    localIcon->setInterceptsMouseClicks(false, false);        // Else the icon gets the click.
    icon = std::move(localIcon);

    addAndMakeVisible(*icon);
}

std::unique_ptr<ColoredImage> TabWithIcon::createIcon(const void* imageData, size_t dataSize) noexcept
{
    const auto color = getIconColor();
    return std::make_unique<ColoredImage>(imageData, dataSize, color);
}


// Component method implementations.
// =================================

void TabWithIcon::resized()
{
    // Displays the image, centered.
    const auto totalWidth = getWidth();
    const auto totalHeight = getHeight();

    const auto imageWidth = icon->getImageWidth();
    const auto imageHeight = icon->getImageHeight();

    icon->setBounds((totalWidth - imageWidth) / 2, (totalHeight - imageHeight) / 2, imageWidth, imageHeight);
}

void TabWithIcon::paint(juce::Graphics& g)
{
    const auto color = findColour(isSelected() ?
                       static_cast<int>(LookAndFeelConstants::Colors::panelTabBackgroundSelected) :
                       static_cast<int>(LookAndFeelConstants::Colors::panelTabBackgroundUnselected));
    g.fillAll(color);
}

void TabWithIcon::mouseUp(const juce::MouseEvent& event)
{
    // Don't select if the tab was already selected.
    if (event.mods.isLeftButtonDown() && !isSelected()) {
        listener.onTabClicked(getArrangerId());
    }
}

void TabWithIcon::lookAndFeelChanged()
{
    const auto color = getIconColor();
    icon->setImageColor(color);
}


// =================================

PanelType TabWithIcon::getPanelType() const noexcept
{
    return panelType;
}

juce::Colour TabWithIcon::getIconColor() noexcept
{
    return juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::panelTabIcon));
}


// TooltipClient method implementations.
// =====================================

juce::String TabWithIcon::getTooltip()
{
    return toolTip;
}

}   // namespace arkostracker
