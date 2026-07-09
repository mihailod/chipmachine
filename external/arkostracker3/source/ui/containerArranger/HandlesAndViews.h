#pragma once

#include <memory>
#include <vector>

#include "ArrangerView.h"
#include "Handle.h"
#include "SearchableView.h"

namespace arkostracker 
{

class ArrangerController;

/** Holds ArrangerViews and Handles in between. One handle, a View, one handle, a view, one handle for example. */
class HandlesAndViews
{
public:
    /**
     * Constructor.
     * @param parentComponent the parent Component, to make the new Component visible.
     * @param handleListener the listener to be notified of the events of the handles.
     * @param horizontalHandles true if the Handles are horizontal, false if vertical.
     */
    HandlesAndViews(juce::Component& parentComponent, Handle::HandleListener& handleListener, bool horizontalHandles) noexcept;

    /** @return the handles and LineContainers, in order. */
    std::vector<ArrangerView*> buildViewArray() const noexcept;

    /**
     * Adds the View. It will be automatically followed by a Handle.
     * @param arrangerView the View to add.
     * @param isExtremity true if first or last.
     */
    void addView(std::unique_ptr<SearchableView> arrangerView, bool isExtremity) noexcept;

    /** Asks the children to update their layout. May be useful to call when a view is deleted for example. */
    void updateLayout() noexcept;

    /**
     * Finds the Container, which contains a panel with the given PanelType.
     * @param findSidePanels true to find in the side panels, false for the "normal" ones.
     * @param panelType the panel type to look for.
     * @return the Container, or nullptr if not found.
     */
    Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) const noexcept;

    /**
     * Finds the Container which has the given focus order.
     * @param focusOrder the focus order to find (0, 1, 2...).
     * @return the Container, or nullptr if not found.
     */
    Container* findContainerWithFocusOrder(int focusOrder) const noexcept;

    /** @return true if there are no Containers. */
    bool isEmpty() const noexcept;

    /** @return all the Panel Types that are used in the Containers. */
    std::set<PanelType> getPanelTypes() const noexcept;

    /** @return all the side-panel Types that are used in the Containers. */
    std::set<PanelType> getSidePanelTypes() const noexcept;

    /** @return the SearchableViews held by this object. */
    std::vector<SearchableView*> getSearchableViews() const noexcept;

    /** @return false if at least one header is present. */
    bool isHiddenHeader() const noexcept;

private:
    juce::Component& parentComponent;                               // The parent Component, to make the new Component visible.
    Handle::HandleListener& handleListener;                         // The listener to be notified of the events of the handles.
    bool horizontalHandles;                                         // True if the Handles are horizontal, false if vertical.
    std::vector<std::unique_ptr<Handle>> handles;                   // The Handles, in order. Always one more than the ArrangerViews.
    std::vector<std::unique_ptr<SearchableView>> searchableViews;   // Holds the Container/LineContainer, in order, without the Handles.
};

}   // namespace arkostracker
