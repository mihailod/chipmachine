#pragma once

#include <memory>
#include <vector>

#include "Container.h"
#include "Handle.h"
#include "HandlesAndViews.h"
#include "PanelType.h"
#include "SearchableView.h"
#include "ViewArranger.h"

namespace arkostracker 
{

/**
 * Holds one or more Containers, and vertical Handles to allow inserting before or after.
 * It has a possible fixed height.
 */
class LineContainer final : public SearchableView,
                            public Handle::HandleListener
{
public:
    /**
     * Constructor with a default container.
     * @param desiredHeight the possible desired height.
     */
    explicit LineContainer(OptionalInt desiredHeight) noexcept;

    /**
     * Adds a Container at the end. A Handle is also added just after.
     * @param container the Container to add.
     * @param isExtremity true if first or last.
     */
    void addContainer(std::unique_ptr<Container> container, bool isExtremity) noexcept;

    // Component method implementations.
    // =================================
    void resized() override;

    // HandleListener method implementations.
    // ======================================
    void onHandleDragged(int handleId, int distancePixels) noexcept override;

    // SearchableView method implementations.
    // =========================================
    Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) noexcept override;
    Container* findContainerWithFocusOrder(int focusOrder) noexcept override;
    OptionalInt getFocusOrder() const override;
    std::vector<PanelType> getPanelTypes() const override;
    PanelType getSelectedPanelType() const override;
    std::vector<PanelType> getSidePanelTypes() const override;
    std::vector<SearchableView*> getSearchableViews() const override;
    bool isHiddenHeader() const override;

    // ArrangerView method implementations.
    // =========================================
    Dimension getArrangerViewWidth() const noexcept override;
    Dimension getArrangerViewHeight() const noexcept override;

private:
    /**
     * @return a vector with unique Panel Types, from the input ones, which may have duplicates.
     * @param inputPanelTypes the input panel types.
     */
    static std::vector<PanelType> buildUniquePanelTypes(const std::set<PanelType>& inputPanelTypes) noexcept;

    const Dimension desiredHeight;                                  // The height of this Line Container.
    ViewArranger viewArranger;                                      // Calculates how the views are resized.
    HandlesAndViews handlesAndViews;                                // Holds Handles and Containers.
};

}   // namespace arkostracker

