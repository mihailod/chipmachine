#include "ContainerArrangement.h"

namespace arkostracker 
{

ContainerArrangement::ContainerArrangement(const Dimension pDesiredWidth, const bool pHiddenHeader, std::vector<PanelType> pPanelTypes,
    std::vector<PanelType> pSidePanelTypes, OptionalInt pFocusOrder) noexcept :
        desiredWidth(pDesiredWidth),
        hiddenHeader(pHiddenHeader),
        panelTypes(std::move(pPanelTypes)),
        sidePanelTypes(std::move(pSidePanelTypes)),
        focusOrder(pFocusOrder)
{
    jassert(!panelTypes.empty());
    jassert(focusOrder.isAbsent() || (focusOrder.getValue() >= 0));
}

Dimension ContainerArrangement::getDesiredWidth() const noexcept
{
    return desiredWidth;
}

bool ContainerArrangement::isHiddenHeader() const noexcept
{
    return hiddenHeader;
}

const std::vector<PanelType>& ContainerArrangement::getPanelTypes() const noexcept
{
    return panelTypes;
}

const std::vector<PanelType>& ContainerArrangement::getSidePanelTypes() const noexcept
{
    return sidePanelTypes;
}

OptionalInt ContainerArrangement::getFocusOrder() const noexcept
{
    return focusOrder;
}

bool operator==(const ContainerArrangement& left, const ContainerArrangement& right)
{
    return (left.desiredWidth == right.desiredWidth)
           && (left.hiddenHeader == right.hiddenHeader)
           && (left.panelTypes == right.panelTypes)
           && (left.focusOrder == right.focusOrder);
}

}   // namespace arkostracker
