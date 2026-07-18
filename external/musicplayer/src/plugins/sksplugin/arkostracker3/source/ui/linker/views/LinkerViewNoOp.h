#pragma once

#include "LinkerView.h"

namespace arkostracker 
{

/** A LinkerView that does nothing. Used when the Panel is destroyed. */
class LinkerViewNoOp final : public LinkerView
{
public:
    explicit LinkerViewNoOp(LinkerController& linkerController);

    void refreshAllItems(const std::vector<PatternItemView::DisplayedData>& itemsData, const LoopBar::DisplayData& loopData,
                         const PlayLineView::DisplayedData& playLineData, const std::unordered_map<int, juce::String>& positionToMarker) noexcept override;
    void scrollToItem(int position) noexcept override;

    void editItems(const std::set<int>& positionIndexes) noexcept override;
};


}   // namespace arkostracker

