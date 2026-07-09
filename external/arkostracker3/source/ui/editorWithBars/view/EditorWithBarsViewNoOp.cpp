#include "EditorWithBarsViewNoOp.h"

#include "../controller/EditorWithBarsControllerNoOp.h"

namespace arkostracker 
{

EditorWithBarsViewNoOp::EditorWithBarsViewNoOp() noexcept :
        controller(std::make_unique<EditorWithBarsControllerNoOp>())
{
}

void EditorWithBarsViewNoOp::setDisplayedBarCount(int /*barCount*/) noexcept
{
}

void EditorWithBarsViewNoOp::showBarData(AreaType /*areaType*/, int /*barIndex*/, const BarData& /*barData*/, const BarCaptionDisplayedData& /*captionData*/) noexcept
{
}

void EditorWithBarsViewNoOp::setMetadataDisplayData(std::unordered_map<AreaType, BarAreaSize> /*areaTypeToSize*/,
                                                         std::unordered_map<AreaType, LeftHeaderDisplayData> /*areaTypeToLeftHeaderDisplayData*/,
                                                         std::unordered_map<AreaType, Loop> /*areaTypeToAutoSpreadData*/) noexcept
{
}

void EditorWithBarsViewNoOp::setDisplayTopHeaderData(const EditorWithBarsView::DisplayTopHeaderData& /*newDisplayNonData*/) noexcept
{
}

void EditorWithBarsViewNoOp::setXZoomRate(int /*xZoomRate*/) noexcept
{
}

int EditorWithBarsViewNoOp::getValue(AreaType /*areaType*/, int /*barIndex*/) noexcept
{
    return 0;
}

AreaType EditorWithBarsViewNoOp::getShiftedAreaType(AreaType areaType, int /*offset*/) const noexcept
{
    return areaType;
}

void EditorWithBarsViewNoOp::ensureVisible(AreaType /*areaType*/, int /*barIndex*/) noexcept
{
}

void EditorWithBarsViewNoOp::highlightBarIndexes(const std::vector<int>& /*indexes*/) noexcept
{
}


// AllBarsAreaDataProvider method implementations.
// ==================================================

const std::vector<AreaType>& EditorWithBarsViewNoOp::getAllAreaTypes() const
{
    static const std::vector<AreaType> areaTypes;
    return areaTypes;
}

std::vector<int> EditorWithBarsViewNoOp::getBarHeights(AreaType /*areaType*/) const
{
    return { };
}

juce::String EditorWithBarsViewNoOp::getDisplayedName(AreaType /*areaType*/) const
{
    return { };
}

std::vector<std::pair<AreaType, juce::String>> EditorWithBarsViewNoOp::getSections() const
{
    return { };
}

}   // namespace arkostracker
