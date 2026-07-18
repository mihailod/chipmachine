#pragma once

#include "BarEditorCommands.h"
#include "EditorWithBarsView.h"
#include "component/AllBarsArea.h"
#include "component/Header.h"

namespace arkostracker 
{

class EditorWithBarsController;

/**
 * Partial implementation of an EditorWithBarsView. It holds all the Bar Areas and the header.
 * IMPORTANT NOTE: the children MUST call initializeViews() when they are constructed.
 */
class EditorWithBarsViewImpl : public EditorWithBarsView,
                               public juce::ApplicationCommandTarget
{
public:

    /**
     * Constructor.
     * @param controller the controller to this View.
     * @param xZoomRate an arbitrary value for the X zoom (>=0).
     */
    EditorWithBarsViewImpl(EditorWithBarsController& controller, int xZoomRate) noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    // EditorWithBarsView method implementations.
    // =============================================
    void setDisplayedBarCount(int barCount) noexcept override;
    void showBarData(AreaType areaType, int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) noexcept override;
    void setMetadataDisplayData(std::unordered_map<AreaType, BarAreaSize> areaTypeToSize, std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData,
                                std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData) noexcept override;
    void setDisplayTopHeaderData(const DisplayTopHeaderData& displayTopHeaderData) noexcept override;
    void setXZoomRate(int xZoomRate) noexcept override;
    int getValue(AreaType areaType, int barIndex) noexcept override;
    AreaType getShiftedAreaType(AreaType areaType, int offset) const noexcept override;
    void ensureVisible(AreaType areaType, int barIndex) noexcept override;
    void highlightBarIndexes(const std::vector<int>& indexes) noexcept override;

    // ApplicationCommandTarget method implementations.
    // ==================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

protected:
    /** MUST be called by the child after it and the parent are constructed. */
    void initializeViews() noexcept;

    /** @return the controller. */
    EditorWithBarsController& getController() const noexcept;

private:
    BarEditorCommands barEditorCommands;
    EditorWithBarsController& controller;

    Header header;
    AllBarsArea allBarsArea;                                            // Where are all the bars, their header at the left, plus the loops.

    EditorWithBarsView::DisplayTopHeaderData displayTopHeaderData;      // The non-bars data.
};

}   // namespace arkostracker
