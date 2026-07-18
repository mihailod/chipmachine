#pragma once

#include "../../../editorWithBars/view/EditorWithBarsViewImpl.h"

namespace arkostracker 
{

/** The implementation of the PSG Instrument Editor View. */
class PsgInstrumentEditorViewImpl final : public EditorWithBarsViewImpl
{
public:
    /**
     * Constructor.
     * @param controller the controller of each Bar Area.
     * @param xZoomRate an arbitrary value for the X zoom (>=0).
     */
    PsgInstrumentEditorViewImpl(EditorWithBarsController& controller, int xZoomRate) noexcept;

    // AllBarsArea::DataProvider method implementations.
    // ====================================================
    const std::vector<AreaType>& getAllAreaTypes() const override;
    std::vector<int> getBarHeights(AreaType areaType) const override;
    juce::String getDisplayedName(AreaType areaType) const override;
    std::vector<std::pair<AreaType, juce::String>> getSections() const override;

    // ApplicationCommandTarget method implementations.
    // ==================================================
    bool perform(const InvocationInfo& info) override;
};

}   // namespace arkostracker
