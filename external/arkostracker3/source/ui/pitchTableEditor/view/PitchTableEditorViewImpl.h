#pragma once

#include "../../editorWithBars/controller/EditorWithBarsController.h"
#include "../../editorWithBars/view/EditorWithBarsViewImpl.h"

namespace arkostracker 
{

/** Implementation of the View of the Pitch Table Editor. */
class PitchTableEditorViewImpl : public EditorWithBarsViewImpl
{
public:

    /**
     * Constructor.
     * @param controller the controller to this View.
     * @param xZoomRate an arbitrary value for the X zoom (>=0).
     */
    PitchTableEditorViewImpl(EditorWithBarsController& controller, int xZoomRate) noexcept;

    // AllBarsArea::DataProvider method implementations.
    // ====================================================
    const std::vector<AreaType>& getAllAreaTypes() const override;
    std::vector<int> getBarHeights(AreaType areaType) const override;
    juce::String getDisplayedName(AreaType areaType) const override;
    std::vector<std::pair<AreaType, juce::String>> getSections() const override;

private:
    const std::vector<AreaType> allAreaTypes;               // All the areas to show.
};

}   // namespace arkostracker
