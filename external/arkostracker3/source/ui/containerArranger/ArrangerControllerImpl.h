#pragma once

#include <set>

#include "ArrangerController.h"
#include "LineContainer.h"
#include "SearchableView.h"
#include "ViewArranger.h"

namespace arkostracker 
{

/** Implementation of the Arranger Controller. */
class ArrangerControllerImpl final : public ArrangerController,
                                     public juce::ComponentListener
{
public:
    /**
     * Constructor.
     * @param topOffset the offset from the top. Ignored if floating.
     * @param parentComponentNotManaged where the Views will be added, if the parent lifecycle must NOT be managed by this Controller.
     * @param mainController the Main Controller.
     */
    explicit ArrangerControllerImpl(int topOffset, juce::Component& parentComponentNotManaged, MainController& mainController) noexcept;

    /** Destructor. */
    ~ArrangerControllerImpl() override;

    // ComponentListener method implementations.
    // =========================================
    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;

    // Handle::HandleListener method implementations.
    // ===================================================
    void onHandleDragged(int handleId, int distancePixels) override;

    // ArrangerController method implementations.
    // ===================================================
    void addLineContainer(std::unique_ptr<SearchableView> searchableView, bool isExtremity) noexcept override;
    bool searchAndOpenPanel(PanelType panelType, bool focus) noexcept override;
    bool isPanelVisible(PanelType panelType) const noexcept override;
    Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) const noexcept override;
    void focusNextOrPreviousContainer(PanelType panelType, bool next) noexcept override;

    bool isEmpty() const noexcept override;
    std::set<PanelType> getPanelTypes() const noexcept override;
    void updateAllLayouts() noexcept override;
    std::vector<SearchableView*> getSearchableViews() const override;

private:
    /** Updates the vertical layout (only the LineContainers). */
    void updateLayoutVertical() noexcept;

    HandlesAndViews handlesAndViews;                                        // Holds the Handles and LineContainers.
    ViewArranger viewArranger;                                              // Calculates how the views are resized.
};

}   // namespace arkostracker
