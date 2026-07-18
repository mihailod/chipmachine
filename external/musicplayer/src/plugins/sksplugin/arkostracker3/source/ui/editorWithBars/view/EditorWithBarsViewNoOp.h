#pragma once

#include "EditorWithBarsView.h"
#include "../controller/EditorWithBarsController.h"
#include "component/BarCaptionDisplayedData.h"
#include "component/BarData.h"

namespace arkostracker 
{

/** An EditorWithBarsView that does nothing. */
class EditorWithBarsViewNoOp : public EditorWithBarsView
{
public:
    EditorWithBarsViewNoOp() noexcept;

    void setDisplayedBarCount(int barCount) noexcept override;
    void showBarData(AreaType areaType, int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) noexcept override;
    void setMetadataDisplayData(std::unordered_map<AreaType, BarAreaSize> areaTypeToSize, std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData,
                                std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData) noexcept override;
    void setDisplayTopHeaderData(const DisplayTopHeaderData& displayNonData) noexcept override;
    void setXZoomRate(int xZoomRate) noexcept override;
    int getValue(AreaType areaType, int barIndex) noexcept override;
    AreaType getShiftedAreaType(AreaType areaType, int offset) const noexcept override;
    void ensureVisible(AreaType areaType, int barIndex) noexcept override;
    void highlightBarIndexes(const std::vector<int>& indexes) noexcept override;

    // AllBarsAreaDataProvider method implementations.
    // ==================================================
    const std::vector<AreaType>& getAllAreaTypes() const override;
    std::vector<int> getBarHeights(AreaType areaType) const override;
    juce::String getDisplayedName(AreaType areaType) const override;
    std::vector<std::pair<AreaType, juce::String>> getSections() const override;

private:
    std::unique_ptr<EditorWithBarsController> controller;
};

}   // namespace arkostracker
