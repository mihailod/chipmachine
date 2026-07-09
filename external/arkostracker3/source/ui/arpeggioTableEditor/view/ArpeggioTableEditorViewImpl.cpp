#include "ArpeggioTableEditorViewImpl.h"

#include "../../editorWithBars/view/component/AllBarsAreaConstants.h"

namespace arkostracker 
{

ArpeggioTableEditorViewImpl::ArpeggioTableEditorViewImpl(EditorWithBarsController& pController, const int pXZoomRate) noexcept :
        EditorWithBarsViewImpl(pController, pXZoomRate),
        allAreaTypes({ AreaType::primaryArpeggioOctave, AreaType::primaryArpeggioNoteInOctave })
{
    initializeViews();
}


// AllBarsArea::DataProvider method implementations.
// ====================================================

const std::vector<AreaType>& ArpeggioTableEditorViewImpl::getAllAreaTypes() const
{
    return allAreaTypes;
}

std::vector<int> ArpeggioTableEditorViewImpl::getBarHeights(AreaType /*areaType*/) const
{
    return std::vector {
        AllBarsAreaConstants::minimumHeight,
        AllBarsAreaConstants::normalHeight,
        AllBarsAreaConstants::largeHeight,
        AllBarsAreaConstants::largerHeight,
    };
}

juce::String ArpeggioTableEditorViewImpl::getDisplayedName(const AreaType areaType) const
{
    switch (areaType) {
        case AreaType::primaryArpeggioNoteInOctave:
            return juce::translate("Note");
        case AreaType::primaryArpeggioOctave:
            return juce::translate("Octave");
        case AreaType::soundType:
        case AreaType::envelope:
        case AreaType::noise:
        case AreaType::primaryPeriod:
        case AreaType::primaryPitch:
        case AreaType::secondaryPeriod:
        case AreaType::secondaryArpeggioNoteInOctave:
        case AreaType::secondaryPitch:
        case AreaType::secondaryArpeggioOctave:
        case AreaType::sidBalancePercent:
        case AreaType::sidLowShelfVolume:
        case AreaType::sidIsActivatedAndRatio:
        case AreaType::sidArpeggio:
        case AreaType::sidPitch:
            jassertfalse;       // Shouldn't happen!
            break;
    }

    return { };
}

std::vector<std::pair<AreaType, juce::String>> ArpeggioTableEditorViewImpl::getSections() const
{
    return { };
}

}   // namespace arkostracker
