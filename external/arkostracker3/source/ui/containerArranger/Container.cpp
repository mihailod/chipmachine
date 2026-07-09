#include "Container.h"

#include "ArrangerController.h"
#include "PanelSizes.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "tab/TabWithIcon.h"

namespace arkostracker 
{

const int Container::tabWidth = 30;
const int Container::leftTabStartX = 1;
const int Container::leftTabStartY = 1;
const int Container::rightTabStartY = 1;
const int Container::rightTabEndXMargin = 1;
const int Container::rightTabHeightMinus = 3;

Container::Container(const Dimension pWidth, ArrangerController& pArrangerController, const bool hideHeader, const std::vector<PanelType>& pLeftPanelTypes,
                     const std::vector<PanelType>& pRightPanelTypes, const OptionalInt pFocusOrder) noexcept :
        containerWidth(pWidth),
        leftTabHeight(PanelSizes::headerHeight),
        rightTabHeight(PanelSizes::headerHeight - rightTabHeightMinus),
        arrangerController(pArrangerController),
        leftTabs(),
        selectedLeftTabId(-1),
        rightTabs(),
        rightPanelTypes(pRightPanelTypes),
        focusOrder(pFocusOrder),
        currentPanel(),
        headerHidden(hideHeader)
{
    setOpaque(true);

    jassert(!pLeftPanelTypes.empty());           // Must never happen.

    // Adds all the given Left-Panel Types.
    for (const auto panelType : pLeftPanelTypes) {
        createAndAddTab(true, panelType);
    }

    // Adds the possible right-panel types.
    for (const auto panelType : pRightPanelTypes) {
        createAndAddTab(false, panelType);
    }

    // Locates the left tabs. The right tabs canNOT be located now because there is no width yet.
    rearrangeTabs(true);
}

int Container::createAndAddTab(const bool isLeftTab, PanelType panelType) noexcept
{
    // Instantiates the tab.
    const auto tooltip = arrangerController.getTooltipForPanelType(panelType);

    auto tab = std::make_unique<TabWithIcon>(panelType, *this, tooltip);
    if (!isLeftTab) {
        // We don't want the side-tabs to get the focus, because it will get it only temporarily before
        // the side panel is shown, making a not very beautiful temporary focus on the current panel.
        tab->setMouseClickGrabsKeyboardFocus(false);
    }

    const auto id = tab->getArrangerId();
    addAndMakeVisible(*tab);

    // Stores the tab.
    auto& tabs = isLeftTab ? leftTabs : rightTabs;
    tabs.emplace_back(std::move(tab));

    return id;
}

void Container::rearrangeTabs(const bool isLeftTabs) const noexcept
{
    const auto visible = !headerHidden;

    const auto& tabs = isLeftTabs ? leftTabs : rightTabs;

    // Puts the tabs one after the other, makes them visible or not.
    auto x = isLeftTabs ? leftTabStartX : calculateRightTabX();
    const auto y = isLeftTabs ? leftTabStartY : rightTabStartY;
    const auto height = isLeftTabs ? leftTabHeight : rightTabHeight;
    for (const auto& tab : tabs) {
        tab->setVisible(visible);
        tab->setBounds(x, y, tabWidth, height);
        x += tabWidth;
    }
}

int Container::calculateRightTabX() const noexcept
{
    return getWidth() - (tabWidth * static_cast<int>(rightTabs.size())) - rightTabEndXMargin;
}

void Container::paint(juce::Graphics& g)
{
    // The panel fills everything if there is no Tabs.
    if (headerHidden) {
        return;
    }

    const auto width = getWidth();

    // There is a header.
    // Only fills the tab part, the rest will be filled by the panel itself.
    g.setColour(findColour(static_cast<int>(LookAndFeelConstants::Colors::interspace)));
    g.fillRect(0, 0, width, leftTabHeight);

    // Draws the border over the tabs.
    constexpr auto borderSize = 1;
    const auto tabsWidth = static_cast<int>(leftTabs.size() * static_cast<size_t>(tabWidth));
    const auto right = tabsWidth + leftTabStartX;
    const auto bottom = leftTabHeight + borderSize;

    // TO IMPROVE? Wrong, it's the Parent that has the focus! Would have to ask for it (or add listener??)! But... It looks actually good when always unfocused.
    // const auto focused = hasKeyboardFocus(false);
    g.setColour(findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBorderUnfocused)));
    g.drawVerticalLine(0, 0.0F, static_cast<float>(bottom));
    g.drawVerticalLine(right, 0.0F, static_cast<float>(bottom));
    g.drawHorizontalLine(0, 0.0F, static_cast<float>(right));

    // Draws the right-tabs, if any.
    if (!rightTabs.empty()) {
        const auto rightTabsX = calculateRightTabX() - borderSize;
        const auto rightTabsWidth = static_cast<int>(rightTabs.size() * static_cast<size_t>(tabWidth)) + (borderSize * 2);
        constexpr auto rightTabsY = rightTabStartY - borderSize;
        const auto rightTabsHeight = rightTabHeight + (borderSize * 2);

        g.drawRect(rightTabsX, rightTabsY, rightTabsWidth, rightTabsHeight);
    }
}

void Container::resized()
{
    setBoundsToCurrentPanel();
}

void Container::setBoundsToCurrentPanel() const noexcept
{
    if (currentPanel == nullptr) {
        return;
    }

    // If hidden, takes all the room, else below the tabs and separator.
    const auto panelY = headerHidden ? 0 : PanelSizes::headerHeight + 1;
    currentPanel->setBounds(0, panelY, getWidth(), getHeight() - panelY);

    // Now that we have a width, we can actually relocate the right tabs.
    rearrangeTabs(false);
}


// ArrangerView method implementations.
// =======================================

bool Container::isHiddenHeader() const
{
    return headerHidden;
}

Dimension Container::getArrangerViewWidth() const noexcept
{
    return containerWidth;
}

Dimension Container::getArrangerViewHeight() const noexcept
{
    return Dimension::buildFree();
}


// =======================================

void Container::refreshCurrentPanelFromSelectedTab() noexcept
{
    // Deletes the possibly remaining Panel.
    currentPanel.reset();

    // What is the type of the panel?
    const auto panelType = findSelectedPanelType();
    // Instantiates it.
    currentPanel = arrangerController.createPanel(panelType);

    // Shows the Panel.
    addAndMakeVisible(*currentPanel, 0);    // In the background! Else the header corner will be hidden.
    setBoundsToCurrentPanel();
}

void Container::onTabClicked(const int tabId)
{
    // Let the controller do the selection.
    const auto panelType = findPanelType(tabId);
    if (panelType.isAbsent()) {
        jassertfalse;       // Panel type not found, abnormal!
        return;
    }
    arrangerController.searchAndOpenPanel(panelType.getValue(), true);
}

void Container::makeTabSelectedAndRefreshPanel(const int tabId) noexcept
{
    // Finds the tab to select among the current ones.
    auto newSelectedTabId = selectedLeftTabId;
    for (const auto& tab : leftTabs) {
        // Selects/unselects the tabs.
        const auto selected = (tabId == tab->getArrangerId());
        tab->setSelected(selected);

        if (selected) {
            newSelectedTabId = tabId;
        }
    }

    // Only recreates the panel if the selected one is different.
    if (newSelectedTabId != selectedLeftTabId) {
        selectedLeftTabId = newSelectedTabId;
        refreshCurrentPanelFromSelectedTab();
    }
}

PanelType Container::findSelectedPanelType() const noexcept
{
    for (const auto& tab : leftTabs) {
        if (tab->getArrangerId() == selectedLeftTabId) {
            return tab->getPanelType();
        }
    }

    jassertfalse;       // Should NEVER happen! We didn't find the Tab we were looking for!
    return PanelType::dummy;
}


void Container::selectAndShowTab(const PanelType panelType) noexcept
{
    // Finds the TabId from the panel type.
    for (const auto& tab : leftTabs) {
        if (tab->getPanelType() == panelType) {
            makeTabSelectedAndRefreshPanel(tab->getArrangerId());
            return;
        }
    }

    jassertfalse;       // Not found?
}

void Container::selectSideTab(const PanelType panelType) const noexcept
{
    // Selects the wanted one, unselects the others.
    for (const auto& tab : rightTabs) {
        const auto selected = (tab->getPanelType() == panelType);
        tab->setSelected(selected);
    }
}


// SearchableView method implementations.
// =========================================

Container* Container::findContainerWithPanelType(const bool isSidePanel, const PanelType panelType) noexcept
{
    const auto& tabs = isSidePanel ? rightTabs : leftTabs;
    for (const auto& tab : tabs) {
        if (tab->getPanelType() == panelType) {
            return this;
        }
    }
    return nullptr;
}

Container* Container::findContainerWithFocusOrder(const int focusOrderToFind) noexcept
{
    if (focusOrder == focusOrderToFind) {
        return this;
    }

    return nullptr;
}

std::vector<PanelType> Container::getPanelTypes() const
{
    std::vector<PanelType> panelTypes;
    panelTypes.reserve(leftTabs.size());

    for (const auto& tab : leftTabs) {
        panelTypes.emplace_back(tab->getPanelType());
    }

    return panelTypes;
}

std::vector<PanelType> Container::getSidePanelTypes() const
{
    std::vector<PanelType> panelTypes;
    panelTypes.reserve(rightTabs.size());

    for (const auto& tab : rightTabs) {
        panelTypes.emplace_back(tab->getPanelType());
    }

    return panelTypes;
}

std::vector<SearchableView*> Container::getSearchableViews() const
{
    return { };         // Does not contain any SearchableViews.
}


// =========================================

PanelType Container::getSelectedPanelType() const
{
    // What tab is selected?
    for (const auto& tab : leftTabs) {
        if (tab->getArrangerId() == selectedLeftTabId) {
            return tab->getPanelType();
        }
    }

    jassertfalse;               // Not found, abnormal.
    return PanelType::dummy;
}

OptionalValue<PanelType> Container::findPanelType(const int tabId) const noexcept
{
    for (const auto& tab : leftTabs) {
        if (tab->getArrangerId() == tabId) {
            return tab->getPanelType();
        }
    }

    for (const auto& tab : rightTabs) {
        if (tab->getArrangerId() == tabId) {
            return tab->getPanelType();
        }
    }

    return { };
}

void Container::getKeyboardFocus() const noexcept
{
    currentPanel->getKeyboardFocus();
}

bool Container::isPanelTypeVisible(const PanelType panelType) const noexcept
{
    return ((currentPanel != nullptr) && (currentPanel->getType() == panelType));
}

OptionalInt Container::getFocusOrder() const noexcept
{
    return focusOrder;
}

std::vector<const Tab*> Container::getLeftTabs() const noexcept
{
    std::vector<const Tab*> tabs;

    for (const auto& tab : leftTabs) {
        tabs.push_back(tab.get());
    }

    return tabs;
}

std::vector<const Tab*> Container::getRightTabs() const noexcept
{
    std::vector<const Tab*> tabs;

    for (const auto& tab : rightTabs) {
        tabs.push_back(tab.get());
    }

    return tabs;
}

}   // namespace arkostracker
