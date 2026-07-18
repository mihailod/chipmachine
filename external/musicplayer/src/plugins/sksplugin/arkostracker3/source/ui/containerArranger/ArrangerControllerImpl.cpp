#include "ArrangerControllerImpl.h"

namespace arkostracker 
{

ArrangerControllerImpl::ArrangerControllerImpl(const int pTopOffset, juce::Component& pParentComponent, MainController& pMainController) noexcept:
        ArrangerController(pTopOffset, pParentComponent, pMainController),
        handlesAndViews(getParentComponent(), *this, true),
        viewArranger(false, getTopOffset())
{
    // Be notified of the parent resize.
    getParentComponent().addComponentListener(this);
}

ArrangerControllerImpl::~ArrangerControllerImpl()
{
    getParentComponent().removeComponentListener(this);
}


// ComponentListener method implementations.
// =========================================

void ArrangerControllerImpl::componentMovedOrResized(juce::Component& /*component*/, bool /*wasMoved*/, const bool wasResized)
{
    // The parent is resized, rescales our children.
    if (wasResized) {
        updateLayoutVertical();
    }
}


// Handle::HandleListener method implementations.
// ===================================================

void ArrangerControllerImpl::onHandleDragged(const int handleId, const int distancePixels)
{
    // Delegates the work to the Arranger.
    viewArranger.onHandleDragged(handleId, distancePixels, handlesAndViews.buildViewArray());
}


// ===================================================

void ArrangerControllerImpl::updateLayoutVertical() noexcept
{
    // Delegates the work to the Arranger.
    viewArranger.onResized(getParentComponent(), handlesAndViews.buildViewArray());
}

void ArrangerControllerImpl::updateAllLayouts() noexcept
{
    updateLayoutVertical();
    handlesAndViews.updateLayout();
}

std::vector<SearchableView*> ArrangerControllerImpl::getSearchableViews() const
{
    return handlesAndViews.getSearchableViews();
}

void ArrangerControllerImpl::addLineContainer(std::unique_ptr<SearchableView> searchableView, const bool isExtremity) noexcept
{
    handlesAndViews.addView(std::move(searchableView), isExtremity);
}

bool ArrangerControllerImpl::searchAndOpenPanel(const PanelType panelType, const bool focus) noexcept
{
    auto found = false;

    // Finds the Container with the tab, if any. Selects BOTH the normal/side, so that panel and tab can be shown/selected, in case
    // they are in separate Containers.
    auto* container = findContainerWithPanelType(false, panelType);
    if (container != nullptr) {
        container->selectAndShowTab(panelType);
        if (focus) {
            container->getKeyboardFocus();
        }
        found = true;
    }

    container = findContainerWithPanelType(true, panelType);
    if (container != nullptr) {
        container->selectSideTab(panelType);
    }

    // If specific panels are selected, opens the related list panel, if any.
    if (found) {
        auto syncedPanelType = PanelType::dummy;
        switch (panelType) {
            case PanelType::instrumentEditor:
                syncedPanelType = PanelType::instrumentList;
                break;
            case PanelType::pitchTable:
                syncedPanelType = PanelType::pitchList;
                break;
            case PanelType::arpeggioTable:
                syncedPanelType = PanelType::arpeggioList;
                break;
            case PanelType::meters: [[fallthrough]];
            case PanelType::linker: [[fallthrough]];
            case PanelType::patternViewer: [[fallthrough]];
            case PanelType::instrumentList: [[fallthrough]];
            case PanelType::arpeggioList: [[fallthrough]];
            case PanelType::pitchList: [[fallthrough]];
            case PanelType::testArea: [[fallthrough]];
            case PanelType::dummy:
                // Nothing to do.
                break;
        }
        if (syncedPanelType != PanelType::dummy) {
            // Opens (without focusing) the found panel.
            if (auto* foundContainer = findContainerWithPanelType(false, syncedPanelType); foundContainer != nullptr) {
                foundContainer->selectAndShowTab(syncedPanelType);
            }
        }
    }

    return found;
}

bool ArrangerControllerImpl::isPanelVisible(const PanelType panelType) const noexcept
{
    const auto* container = findContainerWithPanelType(false, panelType);
    if (container != nullptr) {
        return container->isPanelTypeVisible(panelType);
    }

    return false;
}

Container* ArrangerControllerImpl::findContainerWithPanelType(const bool findSidePanels, const PanelType panelType) const noexcept
{
    return handlesAndViews.findContainerWithPanelType(findSidePanels, panelType);
}

void ArrangerControllerImpl::focusNextOrPreviousContainer(const PanelType panelType, const bool next) noexcept
{
    // Gets the focus order for the current panel.
    const auto* currentContainer = findContainerWithPanelType(false, panelType);
    if (currentContainer == nullptr) {
        jassertfalse;       // Abnormal, the panel should exist since.
        return;
    }

    const auto focusOrderOptional = currentContainer->getFocusOrder();
    if (focusOrderOptional.isAbsent()) {
        jassertfalse;       // The panel has no focus order, cannot go to next.
        return;
    }

    // Finds the next/previous focus order.
    auto focusOrder = focusOrderOptional.getValue() + (next ? 1 : -1);
    if (focusOrder < 0) {
        focusOrder = lastFocusOrder;
    } else if (focusOrder > lastFocusOrder) {
        focusOrder = 0;
    }

    // What container has this focus order?
    const auto* foundContainer = handlesAndViews.findContainerWithFocusOrder(focusOrder);
    if (foundContainer == nullptr) {
        jassertfalse;       // Strange, focus not found.
        return;
    }

    foundContainer->getKeyboardFocus();
}


// ArrangerController method implementations.
// ===================================================

bool ArrangerControllerImpl::isEmpty() const noexcept
{
    return handlesAndViews.isEmpty();
}


// =========================================

std::set<PanelType> ArrangerControllerImpl::getPanelTypes() const noexcept
{
    return handlesAndViews.getPanelTypes();
}

}   // namespace arkostracker
