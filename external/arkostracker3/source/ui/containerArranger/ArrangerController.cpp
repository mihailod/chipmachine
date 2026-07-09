#include "ArrangerController.h"

#include "../../controllers/MainController.h"
#include "../keyboard/CommandIds.h"
#include "Container.h"

namespace arkostracker 
{

ArrangerController::ArrangerController(const int pTopOffset, juce::Component& pParentComponent, MainController& pMainController) noexcept:
        parentComponent(&pParentComponent),
        mainController(pMainController),
        panelFactory(pMainController, *this),
        lastOpenedPanelType(PanelType::dummy),
        topOffset(pTopOffset),
        lastFocusOrder(0)
{
}

juce::Component& ArrangerController::getParentComponent() const noexcept
{
    return *parentComponent;
}

std::unique_ptr<Panel> ArrangerController::createPanel(const PanelType panelType) const noexcept
{
    return panelFactory.createPanel(panelType);
}

int ArrangerController::getTopOffset() const noexcept
{
    return topOffset;
}

void ArrangerController::setLastFocusOrder(const int focusOrder) noexcept
{
    lastFocusOrder = focusOrder;
}

juce::String ArrangerController::getTooltipForPanelType(const PanelType panelType) const noexcept
{
    auto baseString = PanelTypeHelper::panelTypeToDisplayableString(panelType);

    OptionalInt commandId;
    switch (panelType) {
        case PanelType::linker:
            commandId = CommandIds::openLinker;
            break;
        case PanelType::patternViewer:
            commandId = CommandIds::openPatternViewer;
            break;
        case PanelType::instrumentEditor:
            commandId = CommandIds::openInstrumentEditor;
            break;
        case PanelType::instrumentList:
            commandId = CommandIds::openInstrumentList;
            break;
        case PanelType::arpeggioTable:
            commandId = CommandIds::openArpeggioTable;
            break;
        case PanelType::arpeggioList:
            commandId = CommandIds::openArpeggioList;
            break;
        case PanelType::pitchTable:
            commandId = CommandIds::openPitchTable;
            break;
        case PanelType::pitchList:
            commandId = CommandIds::openPitchList;
            break;
        case PanelType::testArea:
            commandId = CommandIds::openTestArea;
            break;
        case PanelType::meters:
            break;
        default:
            jassertfalse;       // Shouldn't happen!
        case PanelType::dummy:
            break;
    }

    if (commandId.isAbsent()) {
        return baseString;
    }

    // Gets the key String for the "open" command.
    const auto* commandInfo = mainController.getCommandManager().getCommandForID(commandId.getValue());
    if (commandInfo != nullptr) {
        const auto& keys = commandInfo->defaultKeypresses;
        if (!keys.isEmpty()) {
            const auto keyString = keys.getFirst().getTextDescriptionWithIcons();
            return baseString + " (" + keyString + ")";
        }
    }

    return baseString;
}

PanelType ArrangerController::getLatestOpenedPanelType() const noexcept
{
    return lastOpenedPanelType;
}

// Panel::Listener method implementations.
// ===================================================

void ArrangerController::onPanelFocused(const PanelType panelType) noexcept
{
    lastOpenedPanelType = panelType;
}

}   // namespace arkostracker
