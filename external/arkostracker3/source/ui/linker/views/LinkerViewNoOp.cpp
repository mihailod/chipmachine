#include "LinkerViewNoOp.h"

namespace arkostracker 
{

LinkerViewNoOp::LinkerViewNoOp(LinkerController& pLinkerController) :
        LinkerView(pLinkerController)
{
}

void LinkerViewNoOp::scrollToItem(int /*position*/) noexcept
{
}

void LinkerViewNoOp::editItems(const std::set<int>& /*positionIndexes*/) noexcept
{
}

void LinkerViewNoOp::refreshAllItems(const std::vector<PatternItemView::DisplayedData>& /*itemsData*/, const LoopBar::DisplayData& /*loopData*/,
                                     const PlayLineView::DisplayedData& /*playLineData*/, const std::unordered_map<int, juce::String>& /*positionToMarker*/) noexcept
{
}


}   // namespace arkostracker

