#pragma once

#include <memory>
#include <set>
#include <vector>

#include "../panelFactory/PanelFactory.h"
#include "Handle.h"
#include "Panel.h"
#include "SearchableView.h"

namespace arkostracker 
{

class Container;
class MainController;

/**
 * Abstract class for an Arranger Controller, which manages a whole window of LineComponents.
 * The main window will have only one.
 */
class ArrangerController : public Handle::HandleListener,
                           public Panel::Listener
{
public:
    /**
     * Constructor.
     * @param topOffset the offset from the top.
     * @param parentComponent to be notified of the resizing.
     * @param mainController the Main Controller to create panels.
     */
    ArrangerController(int topOffset, juce::Component& parentComponent, MainController& mainController) noexcept;

    // Handle::HandleListener method implementations.
    // ===================================================
    void onHandleDragged(int handleId, int distancePixels) override = 0;

    // Panel::Listener method implementations.
    // ===================================================
    void onPanelFocused(PanelType panelType) noexcept override;

    // ========================================================

    /**
     * Adds the Line Container. This should only be used when building the arrangement from scratch.
     * @param searchableView the view.
     * @param isExtremity true if first or last.
     */
    virtual void addLineContainer(std::unique_ptr<SearchableView> searchableView, bool isExtremity) noexcept = 0;

    /**
     * Searches a panel in the current layout, and opens it if needed.
     * @param panelType the panel type.
     * @param focus true to focus on the panel.
     * @return true if found or already opened. False if not found.
     */
    virtual bool searchAndOpenPanel(PanelType panelType, bool focus) noexcept = 0;

    /**
     * Searches a panel in the current layout.
     * @param panelType the panel type.
     * @return true if found.
     */
    virtual bool isPanelVisible(PanelType panelType) const noexcept = 0;

    /**
     * Finds the Container, which contains a panel with the given PanelType.
     * @param findSidePanels true to find in the side panels, false for the "normal" ones.
     * @param panelType the panel type to look for.
     * @return the Container, or nullptr if not found.
     */
    virtual Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) const noexcept = 0;

    /** @return true if the Controller has no more Containers. */
    virtual bool isEmpty() const noexcept = 0;

    /** @return all the Panel Types that are used in the Containers of this Controller. */
    virtual std::set<PanelType> getPanelTypes() const noexcept = 0;

    /** Updates all the layouts (vertical and inside the LineContainers). */
    virtual void updateAllLayouts() noexcept = 0;

    /** @return the top offset to use. */
    int getTopOffset() const noexcept;

    /**
     * Creates a Panel instance, of the desired type.
     * @param panelType the type of the Panel.
     * @return a new Panel instance.
     */
    std::unique_ptr<Panel> createPanel(PanelType panelType) const noexcept;

    /** @return the inner Searchable Views this View possesses. May be empty for the bottom layer. */
    virtual std::vector<SearchableView*> getSearchableViews() const = 0;

    /**
     * @return the tooltip to display according to the panel type.
     * @param panelType the panel type.
     */
    juce::String getTooltipForPanelType(PanelType panelType) const noexcept;

    /** @return the panel type that was lastly opened. */
    PanelType getLatestOpenedPanelType() const noexcept;

    /** Sets the last focus order (0, 1, 2...). */
    void setLastFocusOrder(int focusOrder) noexcept;

private:
    juce::Component* parentComponent;                                       // Where the Views will be added (main window, floating window content, etc.).
    MainController& mainController;
    PanelFactory panelFactory;
    PanelType lastOpenedPanelType;                                          // To store in the current layout before going into another one.

    int topOffset;                                                          // Offset from the top.

protected:
    /** @return the parent component. It may be managed by this object or not. */
    juce::Component& getParentComponent() const noexcept;

    int lastFocusOrder;
};

}   // namespace arkostracker
