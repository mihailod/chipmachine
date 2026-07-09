#pragma once

#include <vector>

#include "ArrangerView.h"
#include "PanelType.h"

namespace arkostracker 
{

class Container;

/** Abstract pure class for an object that can be or contains Containers, which can be searched. */
class SearchableView : public ArrangerView
{
public:
    /** Destructor. */
    ~SearchableView() override = default;

    /**
     * Finds the Container, which contains a panel with the given PanelType.
     * @param findSidePanels true to find in the side panels, false for the "normal" ones.
     * @param panelType the panel type to look for.
     * @return the Container, or nullptr if not found.
     */
    virtual Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) noexcept = 0;

    /**
     * Finds the Container which has the given focus order.
     * @param focusOrder the focus order to find (0, 1, 2...).
     * @return the Container, or nullptr if not found.
     */
    virtual Container* findContainerWithFocusOrder(int focusOrder) noexcept = 0;

    /** @return all the Panel Types that are used in the Containers. No duplicate. */
    virtual std::vector<PanelType> getPanelTypes() const = 0;
    /** @return the selected PanelType, or dummy, if not relevant. */
    virtual PanelType getSelectedPanelType() const = 0;
    /** @return all the side-panel Types that are used in the Containers. No duplicate. */
    virtual std::vector<PanelType> getSidePanelTypes() const = 0;

    /** @return the possible focus order (0, 1, 2...). */
    virtual OptionalInt getFocusOrder() const = 0;

    /** @return the inner Searchable Views this View possesses. May be empty for the bottom layer. */
    virtual std::vector<SearchableView*> getSearchableViews() const = 0;
};

}   // namespace arkostracker

