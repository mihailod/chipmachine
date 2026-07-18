#include "PitchTableEditorViewImpl.h"
#include "../../editorWithBars/view/component/AllBarsAreaConstants.h"

namespace arkostracker 
{

PitchTableEditorViewImpl::PitchTableEditorViewImpl(EditorWithBarsController& pController, const int pXZoomRate) noexcept :
        EditorWithBarsViewImpl(pController, pXZoomRate),
        allAreaTypes({ AreaType::primaryPitch })
{
    initializeViews();
}


// AllBarsArea::DataProvider method implementations.
// ====================================================

const std::vector<AreaType>& PitchTableEditorViewImpl::getAllAreaTypes() const
{
    return allAreaTypes;
}

std::vector<int> PitchTableEditorViewImpl::getBarHeights(AreaType /*areaType*/) const
{
    return std::vector {
            AllBarsAreaConstants::normalHeight,
            AllBarsAreaConstants::largeHeight,
            AllBarsAreaConstants::largerHeight,
            AllBarsAreaConstants::veryLargeHeight,
    };}

juce::String PitchTableEditorViewImpl::getDisplayedName(AreaType /*areaType*/) const
{
    return juce::translate("Pitch");
}

std::vector<std::pair<AreaType, juce::String>> PitchTableEditorViewImpl::getSections() const
{
    return { };
}

}   // namespace arkostracker
