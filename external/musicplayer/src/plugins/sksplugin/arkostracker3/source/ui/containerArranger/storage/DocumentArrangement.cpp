#include "DocumentArrangement.h"

namespace arkostracker 
{

DocumentArrangement::DocumentArrangement() noexcept:
        lineContainerArrangements(),
        selectedPanelTypes(),
        focusedPanelType(PanelType::dummy)
{
}

void DocumentArrangement::addLineContainerArrangement(const LineContainerArrangement& lineContainerArrangement) noexcept
{
    lineContainerArrangements.push_back(lineContainerArrangement);
}

const std::vector<LineContainerArrangement>& DocumentArrangement::getLineContainerArrangements() const noexcept
{
    return lineContainerArrangements;
}

void DocumentArrangement::setSelectedPanels(PanelType pFocusedPanelType, std::set<PanelType> pPanelTypes) noexcept
{
    selectedPanelTypes = std::move(pPanelTypes);
    focusedPanelType = pFocusedPanelType;
}

std::set<PanelType> DocumentArrangement::getSelectedPanels() const noexcept
{
    return selectedPanelTypes;
}

PanelType DocumentArrangement::getFocusedPanel() const noexcept
{
    return focusedPanelType;
}

bool operator==(const DocumentArrangement& left, const DocumentArrangement& right)
{
    return (left.lineContainerArrangements == right.lineContainerArrangements)
           && (left.selectedPanelTypes == right.selectedPanelTypes)
           && (left.focusedPanelType == right.focusedPanelType);
}

}   // namespace arkostracker
