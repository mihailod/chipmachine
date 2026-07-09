#include "HandlesAndViews.h"

namespace arkostracker 
{

HandlesAndViews::HandlesAndViews(juce::Component& pParentComponent, Handle::HandleListener& pHandleListener, const bool pHorizontalHandles) noexcept :
        parentComponent(pParentComponent),
        handleListener(pHandleListener),
        horizontalHandles(pHorizontalHandles),
        handles(),
        searchableViews()
{
    // Creates a first Handle.
    auto handle = std::make_unique<Handle>(handleListener, horizontalHandles, true);
    parentComponent.addAndMakeVisible(*handle);

    handles.emplace_back(std::move(handle));
}

std::vector<ArrangerView*> HandlesAndViews::buildViewArray() const noexcept
{
    const auto handleCount = handles.size();
    const auto lineContainerCount = searchableViews.size();
    // There must always be one more handle than lineContainer!
    jassert(handleCount == (lineContainerCount + 1U));

    std::vector<ArrangerView*> result;
    result.reserve(handleCount + lineContainerCount);

    // Adds one handle, one lineContainer, etc.
    for (size_t index = 0U; index < lineContainerCount; ++index) {
        result.push_back(handles.at(index).get());
        result.push_back(searchableViews.at(index).get());
    }
    // Adds the last handle.
    result.push_back(handles.at(handleCount - 1U).get());

    return result;
}

void HandlesAndViews::addView(std::unique_ptr<SearchableView> arrangerView, bool isExtremity) noexcept
{
    parentComponent.addAndMakeVisible(*arrangerView);
    searchableViews.push_back(std::move(arrangerView));

    auto handle = std::make_unique<Handle>(handleListener, horizontalHandles, isExtremity);
    parentComponent.addAndMakeVisible(*handle);
    handles.push_back(std::move(handle));
}

void HandlesAndViews::updateLayout() noexcept
{
    for (const auto& view : searchableViews) {
        view->resized();
    }
}

Container* HandlesAndViews::findContainerWithPanelType(const bool findSidePanels, const PanelType panelType) const noexcept
{
    for (const auto& searchableView : searchableViews) {
        if (auto* container = searchableView->findContainerWithPanelType(findSidePanels, panelType); container != nullptr) {
            return container;
        }
    }

    // Not found.
    return nullptr;
}

Container* HandlesAndViews::findContainerWithFocusOrder(const int focusOrder) const noexcept
{
    for (const auto& searchableView : searchableViews) {
        auto* container = searchableView->findContainerWithFocusOrder(focusOrder);
        if (container != nullptr) {
            return container;
        }
    }

    // Not found.
    return nullptr;
}

bool HandlesAndViews::isEmpty() const noexcept
{
    return searchableViews.empty();
}

std::set<PanelType> HandlesAndViews::getPanelTypes() const noexcept
{
    std::set<PanelType> resultPanelTypes;

    // Accumulates the PanelTypes of each View.
    for (const auto& containerView : searchableViews) {
        auto panelTypes = containerView->getPanelTypes();
        resultPanelTypes.insert(panelTypes.cbegin(), panelTypes.cend());
    }

    return resultPanelTypes;
}

std::set<PanelType> HandlesAndViews::getSidePanelTypes() const noexcept
{
    std::set<PanelType> resultPanelTypes;

    // Accumulates the PanelTypes of each View.
    for (const auto& containerView : searchableViews) {
        auto panelTypes = containerView->getSidePanelTypes();
        resultPanelTypes.insert(panelTypes.cbegin(), panelTypes.cend());
    }

    return resultPanelTypes;
}

std::vector<SearchableView*> HandlesAndViews::getSearchableViews() const noexcept
{
    std::vector<SearchableView*> result;
    result.reserve(searchableViews.size());

    for (const auto& searchableView : searchableViews) {
        result.push_back(searchableView.get());
    }

    return result;
}

bool HandlesAndViews::isHiddenHeader() const noexcept
{
    for (const auto& searchableView : searchableViews) {
        if (!searchableView->isHiddenHeader()) {
            // At least one header is present.
            return false;
        }
    }
    // All the headers are hidden.
    return true;
}


}   // namespace arkostracker

