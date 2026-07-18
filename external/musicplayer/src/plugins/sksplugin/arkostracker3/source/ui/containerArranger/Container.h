#pragma once

#include <vector>

#include "Panel.h"
#include "SearchableView.h"
#include "tab/Tab.h"

namespace arkostracker 
{

class ArrangerController;

/**
 * Holds one or more panels (only one visible at a time). Will be belonging to a LineContainer.
 * It has at least one left tab, which the selected one is shown below.
 * It may have right tabs, which are not linked to this container but will instantiate panels elsewhere.
 * A Container has a width (within tolerance or not).
 */
class Container final : public SearchableView,
                        public Tab::Listener
{
public:
    /**
     * Constructor.
     * @param width the width this Container should apply.
     * @param arrangerController the object to create tabs.
     * @param hideHeader true to hide the header.
     * @param leftPanelTypes the left-panel types to create.
     * @param rightPanelTypes the right-panel types to create.
     * @param focusOrder a possible ordering for the focus (when Tab is pressed).
     */
    Container(Dimension width, ArrangerController& arrangerController, bool hideHeader, const std::vector<PanelType>& leftPanelTypes,
              const std::vector<PanelType>& rightPanelTypes = { }, OptionalInt focusOrder = { }) noexcept;

    /**
     * Selects a tab and open its panel, if not done already. This only concerns the left tabs.
     * @param panelType the panel type. If not found, nothing happens, but asserts.
     */
    void selectAndShowTab(PanelType panelType) noexcept;

    /**
     * Selects the side tab. This does not do anything else.
     * @param panelType the panel type. If not found, nothing happens, but asserts.
     */
    void selectSideTab(PanelType panelType) const noexcept;

    /**
     * @return true if the given panel type is visible (selected).
     * @param panelType the panel type.
     */
    bool isPanelTypeVisible(PanelType panelType) const noexcept;

    /** Gives the keyboard focus to the currently seen panel. */
    void getKeyboardFocus() const noexcept;

    /** @return the possible left tabs. */
    std::vector<const Tab*> getLeftTabs() const noexcept;
    /** @return the possible right/side tabs. */
    std::vector<const Tab*> getRightTabs() const noexcept;

    // ArrangerView method implementations.
    // =======================================
    bool isHiddenHeader() const override;
    Dimension getArrangerViewWidth() const noexcept override;
    Dimension getArrangerViewHeight() const noexcept override;

    // Component method implementations.
    // =================================
    void paint(juce::Graphics &g) override;
    void resized() override;

    // Tab::Listener method implementations.
    // =========================================
    void onTabClicked(int id) override;

    // SearchableView method implementations.
    // =========================================
    Container* findContainerWithPanelType(bool findSidePanels, PanelType panelType) noexcept override;
    Container* findContainerWithFocusOrder(int focusOrder) noexcept override;
    std::vector<PanelType> getPanelTypes() const override;
    OptionalInt getFocusOrder() const noexcept override;
    PanelType getSelectedPanelType() const override;
    std::vector<PanelType> getSidePanelTypes() const override;
    std::vector<SearchableView*> getSearchableViews() const override;

private:
    static const int tabWidth;
    static const int leftTabStartX;             // Without their border in account.
    static const int leftTabStartY;
    static const int rightTabStartY;
    static const int rightTabEndXMargin;        // Without their border in account.
    static const int rightTabHeightMinus;       // How small the right tabs are according to the left ones.

    /** @return the X of the first right tab, according to the current width, and without the border around them. */
    int calculateRightTabX() const noexcept;

    /**
     * Creates an instance of a Tab View according to the given PanelType and adds it the tabs.
     * @param isLeftTab true to add a left tab, false for a right tab.
     * @param panelType the type of panel the tab is about.
     * @return the ID of the Tab.
     */
    int createAndAddTab(bool isLeftTab, PanelType panelType) noexcept;

    /** If it exists, sets the bound to the current Panel. */
    void setBoundsToCurrentPanel() const noexcept;

    /** @return the panel type, from the selected tab. There MUST be one! */
    PanelType findSelectedPanelType() const noexcept;

    /**
     * Selects a tab from its id, this refreshes the panel.
     * @param tabId the TabId to look for.
     */
    void makeTabSelectedAndRefreshPanel(int tabId) noexcept;

    /** Recreates the current panel, from the ID of the selected one. */
    void refreshCurrentPanelFromSelectedTab() noexcept;

    /**
     * Sets the coordinates to all the tabs. Useful to call this when one is added.
     * Also sets them to hidden/visible according to the Header Hidden flag.
     * @param leftTabs true if the left tabs, false for the right tabs.
     */
    void rearrangeTabs(bool leftTabs) const noexcept;

    /** Finds the panelType (left or right) from the given TabId. */
    OptionalValue<PanelType> findPanelType(int tabId) const noexcept;

    Dimension containerWidth;
    const int leftTabHeight;                            // How high are the left tabs.
    const int rightTabHeight;
    ArrangerController& arrangerController;             // The object to manage drag'n'drop actions.
    std::vector<std::unique_ptr<Tab>> leftTabs;         // The left tabs to click on.
    int selectedLeftTabId;                              // The ID of the selected leftTab.
    std::vector<std::unique_ptr<Tab>> rightTabs;        // The right tabs to click on.
    std::vector<PanelType> rightPanelTypes;             // Synced with the tabs.

    OptionalInt focusOrder;

    std::unique_ptr<Panel> currentPanel;                // The current Panel, the only one actually instantiated.
    bool headerHidden;                                  // True if the header is hidden.
};

}   // namespace arkostracker
