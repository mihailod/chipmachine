#include "PanelType.h"
#include "PanelSizes.h"

namespace arkostracker 
{

juce::String PanelTypeHelper::panelTypeToDisplayableString(PanelType panelType) noexcept
{
    switch (panelType) {
        case PanelType::instrumentEditor:
            return juce::translate("Instrument editor");
        case PanelType::instrumentList:
            return juce::translate("Instruments");
        case PanelType::arpeggioList:
            return juce::translate("Arpeggios");
        case PanelType::pitchList:
            return juce::translate("Pitches");
        case PanelType::linker:
            return juce::translate("Linker");
        case PanelType::patternViewer:
            return juce::translate("Pattern viewer");
        case PanelType::testArea:
            return juce::translate("Test area");
        case PanelType::meters:
            return juce::translate("Meters");
        case PanelType::pitchTable:
            return juce::translate("Pitch editor");
        case PanelType::arpeggioTable:
            return juce::translate("Arpeggio editor");
        default:
            jassertfalse;       // Shouldn't happen. Forgot to declare a panel above?
        case PanelType::dummy:
            return juce::translate("Empty");
    }
}

Dimension PanelTypeHelper::getPanelWidth(PanelType panelType) noexcept
{
    switch (panelType) {
        case PanelType::instrumentList:
            [[fallthrough]];
        case PanelType::arpeggioList:
            [[fallthrough]];
        case PanelType::pitchList:
            return Dimension(PanelSizes::genericListDesiredWidth, PanelSizes::genericListMinimumWidth, PanelSizes::genericListMaximumWidth);
        default:
            jassertfalse;       // Shouldn't happen. Forgot to declare a panel above?
        case PanelType::dummy:
            [[fallthrough]];
        case PanelType::testArea:
            [[fallthrough]];
        case PanelType::meters:
            [[fallthrough]];
        case PanelType::patternViewer:
            [[fallthrough]];
        case PanelType::instrumentEditor:
            [[fallthrough]];
        case PanelType::pitchTable:
            [[fallthrough]];
        case PanelType::arpeggioTable:
            [[fallthrough]];
        case PanelType::linker:
            return Dimension::buildFree();
    }
}


}   // namespace arkostracker

