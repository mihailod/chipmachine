#include "AllBarsArea.h"

#include "../../../../utils/CollectionUtil.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../utils/ViewportUtil.h"
#include "../../controller/BarAreaController.h"
#include "AllBarsAreaDataProvider.h"

namespace arkostracker 
{

const int AllBarsArea::mainLoopBarId = -1;                  // >=0 are for the AreaType bars (auto-spread).
const int AllBarsArea::heightCountPerArea = 4;
const int AllBarsArea::leftViewportWidth = 84;

const int AllBarsArea::postSectionMargin = 10;

const int AllBarsArea::leftHeaderHeightHidden = 30;

AllBarsArea::AllBarsArea(AllBarsAreaDataProvider& pDataProvider, BarAreaController& pBarAreaController, const int pXZoomRate) noexcept:
        dataProvider(pDataProvider),
        barAreaController(pBarAreaController),
        xZoomRate(pXZoomRate),
        barCount(0),
        globalStartX(0),
        globalStartY(0),
        globalTotalWidth(0),
        globalTotalHeight(0),

        cachedMainLoop(),

        leftViewport(*this),
        leftViewportComponent(),
        barViewport(*this),
        barViewportComponent(),

        mainLoopBar(*this, mainLoopBarId, getItemWidth(), 0, true, LookAndFeelConstants::Colors::loopStartEnd),
        barAreas(),
        areaTypeToAutoSpreadBar(),

        allAreaTypes(),

        areaTypeToCurrentSize(),
        areaTypeToLeftHeader(),
        areaTypeToHeights(),
        areaTypeToLeftHeaderDisplayData(),
        areaTypeToAutoSpreadData(),

        areaTypeToSection()
{
    jassert(mainLoopBarId < 0);         // Must not intersect with the AreaType value.
}


// LoopBar::Listener method implementations.
// ============================================

void AllBarsArea::onUserWantsToSetLoopBarStart(const int barId, const int newStart) noexcept
{
    const auto areaType = getAreaTypeFromBarId(barId);
    barAreaController.onUserWantsToSetLoopBarStart(areaType, newStart);
}

void AllBarsArea::onUserWantsToSetLoopBarEnd(const int barId, const int newEnd) noexcept
{
    const auto areaType = getAreaTypeFromBarId(barId);
    barAreaController.onUserWantsToSetLoopBarEnd(areaType, newEnd);
}

void AllBarsArea::onUserWantsToToggleLoop(const int barId) noexcept
{
    const auto areaType = getAreaTypeFromBarId(barId);
    barAreaController.onUserWantsToToggleLoop(areaType);
}


// LeftHeader::Listener method implementations.
// ===============================================

void AllBarsArea::onLeftHeaderPlusClicked(const AreaType areaType) noexcept
{
    barAreaController.onUserWantsToEnlargeArea(areaType);
}

void AllBarsArea::onLeftHeaderMinusClicked(const AreaType areaType) noexcept
{
    barAreaController.onUserWantsToShrinkArea(areaType);
}

void AllBarsArea::onLeftHeaderAutoSpreadClicked(const AreaType areaType) noexcept
{
    barAreaController.onUserWantsToToggleLoop(areaType);
}

void AllBarsArea::onLeftHeaderHideClicked(const AreaType areaType) noexcept
{
    barAreaController.onLeftHeaderHideClicked(areaType);
}

// ============================================

int AllBarsArea::getItemWidth() const noexcept
{
    return 16 + (xZoomRate * 6);
}

void AllBarsArea::onScrollingOnBarViewport(const juce::Rectangle<int>& newVisibleArea) noexcept
{
    // This can be called from any of the two Viewports!
    // Synchronizes both Viewport with the Bar Viewport (Y only).
    const auto newY = newVisibleArea.getY();
    const auto x = newVisibleArea.getX();
    scrollOnViewports(x, newY);
}

// ReSharper disable once CppDFAUnreachableFunctionCall
void AllBarsArea::scrollOnViewports(const int x, const int y) noexcept  // Stupid warning. This is called just above.
{
    leftViewport.setViewPosition(0, y);
    barViewport.setViewPosition(x, y);
}

void AllBarsArea::initViews(juce::Component& parent) noexcept
{
    jassert(allAreaTypes.empty());          // Don't call this more than once!
    jassert(areaTypeToCurrentSize.empty());
    jassert(areaTypeToSection.empty());

    parent.addAndMakeVisible(leftViewport);
    parent.addAndMakeVisible(barViewport);

    barViewportComponent.addAndMakeVisible(mainLoopBar);
    barViewportComponent.setWantsKeyboardFocus(true);

    // Any sections to display?
    const auto sections = dataProvider.getSections();
    for (const auto &[areaType, sectionText] : sections) {
        // Creates the section view.
        auto sectionView = std::make_unique<Section>(sectionText);
        barViewportComponent.addAndMakeVisible(*sectionView);       // The location is set later.

        areaTypeToSection.insert(std::make_pair(areaType, std::move(sectionView)));
    }

    // Gets all the area types to display.
    allAreaTypes = dataProvider.getAllAreaTypes();

    // Fills the height tables.
    for (const auto areaType : allAreaTypes) {
        // Sets the various heights for each Area, for each AreaSize. For the minimum size, we use caption height because we want to see it.
        const auto heightsPerSize = dataProvider.getBarHeights(areaType);
        jassert(heightsPerSize.size() == static_cast<size_t>(heightCountPerArea));            // Must be the right size!!

        areaTypeToHeights.insert(std::make_pair(areaType, heightsPerSize));
    }

    // Sets up the Viewports.
    leftViewport.setViewedComponent(&leftViewportComponent, false);
    leftViewport.setScrollBarsShown(false, false, true, false);

    barViewport.setViewedComponent(&barViewportComponent, false);
    barViewport.setScrollBarsShown(true, true, true, true);

    for (const auto areaType : allAreaTypes) {
        // Creates the Bar Areas.
        auto barArea = std::make_unique<BarArea>(areaType, barAreaController, getItemWidth());
        barViewportComponent.addAndMakeVisible(*barArea);

        barAreas.push_back(std::move(barArea));

        // Creates the left headers.
        const auto displayedName = dataProvider.getDisplayedName(areaType);

        const auto canHideRow = barAreaController.canHideRow(areaType);
        auto leftHeader = std::make_unique<LeftHeader>(*this, areaType, canHideRow, displayedName);
        leftViewportComponent.addAndMakeVisible(*leftHeader);           // Makes them visible.

        areaTypeToLeftHeader.insert(std::make_pair(areaType, std::move(leftHeader)));

        // Creates auto-spread toolbars with dummy data for now.
        auto barId = static_cast<int>(areaType);        // Important! The BarId is a cast of AreaType.
        auto barView = std::make_unique<LoopBar>(*this, barId, getItemWidth(), 0, false, LookAndFeelConstants::Colors::autoSpreadLoop);
        barViewportComponent.addAndMakeVisible(*barView);

        areaTypeToAutoSpreadBar.insert(std::make_pair(areaType, std::move(barView)));
    }
}

juce::Component* AllBarsArea::getFocusableComponent() noexcept
{
    return &barViewport;
}

void AllBarsArea::locateViews(const int startX, const int startY, const int totalWidth, const int totalHeight) noexcept
{
    globalStartX = startX;
    globalStartY = startY;
    globalTotalWidth = totalWidth;
    globalTotalHeight = totalHeight;

    locateViews();
}

void AllBarsArea::locateViews() noexcept
{
    if ((globalTotalWidth == 0) || (globalTotalHeight == 0)) {
        return;     // If the size has not been set yet, nothing need to bother.
    }
    if (areaTypeToCurrentSize.empty()) {
        return;     // Sizes not set yet. Nothing can be done. This can happen when the view is recreated. The dimensions are set this time, but not the maps.
    }

    const auto barViewportWidth = globalTotalWidth - leftViewportWidth;

    const auto margins = LookAndFeelConstants::margins;
    const auto availableWidth = barViewportWidth - (2 * margins);

    const auto autoSpreadBarHeight = static_cast<int>(LoopBar::markerSize);
    constexpr auto mainLoopBarHeight = 20;

    constexpr auto barViewportX = leftViewportWidth;
    const auto viewPortsY = globalStartY;
    const auto viewPortsHeight = globalTotalHeight;

    leftViewport.setBounds(0, viewPortsY, leftViewportWidth, viewPortsHeight);
    barViewport.setBounds(barViewportX, viewPortsY, barViewportWidth, viewPortsHeight);

    const auto areasX = globalStartX;
    auto y = 0;

    const auto calculatedAreasWidth = calculateAreasWidth();
    const auto areasWidth = std::max(availableWidth, calculatedAreasWidth + areasX);        // Have a minimum from the available width, looks better.

    // Adds the main loop bar.
    mainLoopBar.setBounds(areasX, y, calculatedAreasWidth, mainLoopBarHeight);
    y += mainLoopBar.getHeight() + margins;

    auto index = 0U;
    for (const auto areaType : allAreaTypes) {
        constexpr auto areaHeightWhenHidden = leftHeaderHeightHidden;
        addSection(areasX, y, areasWidth, areaType);        // Any section to locate before this Area?

        const auto baseY = y;

        // Shows the AutoSpread bar, if wanted.
        const auto showAutoSpreadBar = areaTypeToAutoSpreadData[areaType].isLooping() && !barAreaController.isAreaHidden(areaType);
        const auto& autoSpreadBar = areaTypeToAutoSpreadBar[areaType];
        autoSpreadBar->setVisible(showAutoSpreadBar);
        if (showAutoSpreadBar) {
            autoSpreadBar->setBounds(areasX, y, calculatedAreasWidth, autoSpreadBarHeight);
            y += autoSpreadBarHeight;
        }

        // Sets the bounds of the Area. Can be hidden too!
        const auto isHidden = areaTypeToLeftHeaderDisplayData[areaType].isHidden();
        const auto areaHeight = isHidden ? areaHeightWhenHidden : getAreaHeight(areaType);
        const auto& barArea = barAreas.at(index);
        barArea->setBounds(areasX, y, areasWidth, areaHeight);

        // Sets the bounds of the related Left Header. It must be as high as possible to allow the hover on it (hide button shown when hovered).
        const auto leftHeadersHeight = isHidden ? leftHeaderHeightHidden : areaHeight;
        const auto minimumAreaHeight = leftHeadersHeight;       // This is used when collapsed, so that areas won't overlap vertically.

        areaTypeToLeftHeader.find(areaType)->second->setBounds(0, baseY, leftViewportWidth, leftHeadersHeight);

        y += std::max(areaHeight + margins, minimumAreaHeight);     // Makes sure there is always a minimum height, else looks bad when collapsed.
        ++index;
    }

    // Sets the size of the Viewports.
    barViewportComponent.setSize(areasWidth, y);
    leftViewportComponent.setSize(leftViewportWidth, barViewportComponent.getHeight());

    displayVerticalLoopBars();
}

int AllBarsArea::calculateAreasWidth() const noexcept
{
    const auto barWidth = getItemWidth();
    return barCount * barWidth;
}

int AllBarsArea::getAreaHeight(const AreaType areaType) const noexcept
{
    jassert(areaTypeToCurrentSize.find(areaType) != areaTypeToCurrentSize.cend());      // The size must be set first!
    jassert(areaTypeToHeights.find(areaType) != areaTypeToHeights.cend());

    // Finds the size and heights. No check, we are not supposed to not find them!
    const auto currentSize = areaTypeToCurrentSize.find(areaType)->second;
    const auto& heights = areaTypeToHeights.find(areaType)->second;
    return heights.at(static_cast<size_t>(currentSize));      // Converts the Size directly to an index. No need for more elaborate stuff.
}

void AllBarsArea::setLoopBarDisplayData(const LoopBar::DisplayData& loopBarData) noexcept
{
    mainLoopBar.setDisplayData(loopBarData);
    // Caches it so that it can be displayed after.
    cachedMainLoop = Loop(loopBarData.getStartIndex(), loopBarData.getEndIndex(), loopBarData.isLoopEnabled());
}

void AllBarsArea::setDisplayedBarCount(const int newBarCount) noexcept
{
    barCount = newBarCount;

    for (const auto& barArea : barAreas) {
        barArea->setDisplayedBarCount(newBarCount);
    }

    locateViews();         // To set the width of the viewport.
}

void AllBarsArea::displayVerticalLoopBars() noexcept
{
    // Security, should never happen.
    if (barAreas.empty()) {
        jassertfalse;
        return;
    }

    const auto loopStartIndex = cachedMainLoop.getStartIndex();
    const auto endIndex = cachedMainLoop.getEndIndex();
    if ((loopStartIndex >= barCount) || (endIndex >= barCount)) {   // Security on view creation.
        return;
    }

    const auto& barArea = barAreas.at(0);
    const auto barWidth = barArea->getBarWidth();
    const auto startBarX = barArea->getBarX(loopStartIndex) + globalStartX;
    const auto endBarX = barArea->getBarX(endIndex) + barWidth + globalStartX - 1;
    const auto y = mainLoopBar.getBottom();

    barViewportComponent.setLoop(startBarX, endBarX, cachedMainLoop.isLooping(), y);
}

void AllBarsArea::showBarData(const AreaType areaType, const int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) const noexcept
{
    // Finds the related BarArea, from the Area Type...
    const auto& barArea = getBarArea(areaType);
    // ... and updates it with new data!
    barArea.setBarData(barIndex, barData, captionData);
}

BarArea& AllBarsArea::getBarArea(const AreaType areaType) const noexcept
{
    // Finds the BarArea. It must exist!
    for (const auto& barArea : barAreas) {
        if (barArea->getAreaType() == areaType) {
            return *barArea;
        }
    }

    jassertfalse;       // Should NEVER happen!!
    return **barAreas.cbegin();
}

AreaType AllBarsArea::getShiftedAreaType(const AreaType areaType, const int offset) const noexcept
{
    auto index = CollectionUtil::findIndexOfItem(allAreaTypes, areaType);
    if (index < 0) {
        jassertfalse;           // Not found? Abnormal.
        return areaType;
    }
    index = NumberUtil::correctNumber(index + offset, 0, static_cast<int>(allAreaTypes.size() - 1U));

    return allAreaTypes.at(static_cast<size_t>(index));
}

bool AllBarsArea::setAreaSizesData(std::unordered_map<AreaType, BarAreaSize> newAreaTypeToSize) noexcept
{
    // Any change?
    if (areaTypeToCurrentSize == newAreaTypeToSize) {
        return false;
    }

    areaTypeToCurrentSize = std::move(newAreaTypeToSize);
    return true;
}

bool AllBarsArea::setAreaAutoSpreadData(std::unordered_map<AreaType, Loop> newAreaTypeToAutoSpreadData) noexcept
{
    // Any change?
    if (areaTypeToAutoSpreadData == newAreaTypeToAutoSpreadData) {
        return false;
    }

    areaTypeToAutoSpreadData = std::move(newAreaTypeToAutoSpreadData);
    return true;
}

bool AllBarsArea::setLeftHeaderDisplayData(std::unordered_map<AreaType, LeftHeaderDisplayData> newAreaTypeToLeftHeaderDisplayData) noexcept
{
    // Any change?
    if (areaTypeToLeftHeaderDisplayData == newAreaTypeToLeftHeaderDisplayData) {
        return false;
    }

    areaTypeToLeftHeaderDisplayData = std::move(newAreaTypeToLeftHeaderDisplayData);

    return true;
}

int AllBarsArea::getValue(const AreaType areaType, const int barIndex) const noexcept
{
    const auto& barArea = getBarArea(areaType);
    return barArea.getValue(barIndex);
}

void AllBarsArea::refreshAutoSpreadViewsFromInternalData() noexcept
{
    for (const auto& [areaType, loop] : areaTypeToAutoSpreadData) {
        jassert(areaTypeToAutoSpreadBar.find(areaType) != areaTypeToAutoSpreadBar.cend());      // Must NEVER happen! The bar view hasn't been created!

        const LoopBar::DisplayData newDisplayData(loop.isLooping(), loop.getStartIndex(), loop.getEndIndex());
        areaTypeToAutoSpreadBar[areaType]->setDisplayData(newDisplayData);
    }
}

void AllBarsArea::refreshLeftHeaders(const bool forceRefresh) noexcept
{
    for (const auto& [areaType, data] : areaTypeToLeftHeaderDisplayData) {
        // Applies the data to the matching left header.
        const auto& header = areaTypeToLeftHeader.find(areaType)->second;
        jassert(header != nullptr); // Should never happen!!
        header->setDisplayedData(data, forceRefresh);
    }
}

void AllBarsArea::setXZoomRate(const int newXZoomRate) noexcept
{
    // Any change?
    if (newXZoomRate == xZoomRate) {
        return;
    }
    xZoomRate = newXZoomRate;

    const auto barWidth = getItemWidth();

    // Changes all the Bar Areas.
    for (const auto& barArea : barAreas) {
        barArea->setBarWidth(barWidth);
    }

    // Changes the width of the loop bars.
    for (auto& entry : areaTypeToAutoSpreadBar) {
        const auto& loopBar = entry.second;
        loopBar->setItemWidth(barWidth);
    }
    // Plus the "normal" loop bar.
    mainLoopBar.setItemWidth(barWidth);

    locateViews();     // To resize the viewport.
}

OptionalValue<AreaType> AllBarsArea::getAreaTypeFromBarId(const int barId) noexcept
{
    if (barId == mainLoopBarId) {
        return { };
    }

    return static_cast<AreaType>(barId);
}

void AllBarsArea::addSection(const int x, int& y, const int width, const AreaType areaType) noexcept
{
    const auto sectionIterator = areaTypeToSection.find(areaType);
    if (sectionIterator == areaTypeToSection.cend()) {
        return;         // Nothing to display here.
    }

    auto& sectionView = *sectionIterator->second;

    const auto sectionHeight = Section::getSectionHeight();
    sectionView.setBounds(x, y, width, sectionHeight);

    y += sectionHeight + postSectionMargin;       // Increases the Y for the views below.
}

void AllBarsArea::ensureVisible(const AreaType areaType, const int barIndex) noexcept
{
    const auto& barArea = getBarArea(areaType);

    const auto itemY = barArea.getY();
    const auto itemHeight = barArea.getHeight();
    const auto itemX = barArea.getBarX(barIndex);
    const auto itemWidth = barArea.getBarWidth();

    const auto newViewPortXPosition = ViewportUtil::calculateIfScrollNeeded(itemX, itemWidth,
        barViewport.getViewPositionX(), barViewport.getWidth());
    const auto newViewPortYPosition = ViewportUtil::calculateIfScrollNeeded(itemY, itemHeight,
        barViewport.getViewPositionY(), barViewport.getHeight());

    barViewport.setViewPosition(newViewPortXPosition, newViewPortYPosition);
}

void AllBarsArea::highlightBarIndexes(const std::vector<int>& indexes) noexcept
{
    mainLoopBar.setHighlightedIndexes(indexes);
}

}   // namespace arkostracker
