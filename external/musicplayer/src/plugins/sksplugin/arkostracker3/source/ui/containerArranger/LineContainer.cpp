#include "LineContainer.h"

#include "../../utils/SetUtil.h"
#include "Container.h"

namespace arkostracker 
{

LineContainer::LineContainer(const OptionalInt pDesiredHeight) noexcept :
        desiredHeight(pDesiredHeight.isPresent() ? Dimension::buildLocked(pDesiredHeight.getValue()) : Dimension::buildFree()),
        viewArranger(true, 0),
        handlesAndViews(*this, *this, false)
{
}

void LineContainer::addContainer(std::unique_ptr<Container> container, const bool isExtremity) noexcept
{
    handlesAndViews.addView(std::move(container), isExtremity);
}

void LineContainer::onHandleDragged(const int handleId, const int distancePixels) noexcept
{
    // Delegates the work to the Arranger.
    viewArranger.onHandleDragged(handleId, distancePixels, handlesAndViews.buildViewArray());
}

void LineContainer::resized()
{
    // Delegates the work to the Arranger.
    viewArranger.onResized(*this, handlesAndViews.buildViewArray());
}


// SearchableView method implementations.
// =========================================

Container* LineContainer::findContainerWithPanelType(const bool findSidePanels, const PanelType panelType) noexcept
{
    return handlesAndViews.findContainerWithPanelType(findSidePanels, panelType);
}

Container* LineContainer::findContainerWithFocusOrder(const int focusOrder) noexcept
{
    return handlesAndViews.findContainerWithFocusOrder(focusOrder);
}

std::vector<PanelType> LineContainer::getPanelTypes() const
{
    auto panelTypesMaybeWithDoubles = handlesAndViews.getPanelTypes();
    return buildUniquePanelTypes(panelTypesMaybeWithDoubles);
}

std::vector<PanelType> LineContainer::getSidePanelTypes() const
{
    auto panelTypesMaybeWithDoubles = handlesAndViews.getSidePanelTypes();
    return buildUniquePanelTypes(panelTypesMaybeWithDoubles);
}

std::vector<PanelType> LineContainer::buildUniquePanelTypes(const std::set<PanelType>& inputPanelTypes) noexcept
{
    return SetUtil::setToVector(inputPanelTypes);
}

PanelType LineContainer::getSelectedPanelType() const
{
    jassertfalse;       // Shouldn't be called, not relevant.
    return PanelType::dummy;
}

OptionalInt LineContainer::getFocusOrder() const
{
    jassertfalse;       // Shouldn't be called, not relevant.
    return { };
}

/*int LineContainer::getContainerCount() const
{
    return handlesAndViews.getContainerCount();
}*/

std::vector<SearchableView*> LineContainer::getSearchableViews() const
{
    return handlesAndViews.getSearchableViews();
}

bool LineContainer::isHiddenHeader() const
{
    return handlesAndViews.isHiddenHeader();
}


// ArrangerView method implementations.
// =========================================

Dimension LineContainer::getArrangerViewWidth() const noexcept
{
    return Dimension::buildFree();
}

Dimension LineContainer::getArrangerViewHeight() const noexcept
{
    return desiredHeight;
}


}   // namespace arkostracker

