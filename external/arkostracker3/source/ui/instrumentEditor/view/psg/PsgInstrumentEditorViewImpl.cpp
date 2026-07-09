#include "PsgInstrumentEditorViewImpl.h"

#include "../../../editorWithBars/controller/EditorWithBarsController.h"
#include "../../../editorWithBars/view/component/AllBarsAreaConstants.h"
#include "../../../keyboard/CommandIds.h"

namespace arkostracker 
{

PsgInstrumentEditorViewImpl::PsgInstrumentEditorViewImpl(EditorWithBarsController& pController, const int pXZoomRate) noexcept :
        EditorWithBarsViewImpl(pController, pXZoomRate)
{
    initializeViews();
}


// AllBarsArea::DataProvider method implementations.
// ====================================================

const std::vector<AreaType>& PsgInstrumentEditorViewImpl::getAllAreaTypes() const
{
    static const std::vector allAreaTypes = {
            AreaType::soundType,
            AreaType::envelope,
            AreaType::noise,
            AreaType::primaryArpeggioOctave,
            AreaType::primaryArpeggioNoteInOctave,
            AreaType::primaryPitch,
            AreaType::primaryPeriod,
            AreaType::secondaryArpeggioOctave,
            AreaType::secondaryArpeggioNoteInOctave,
            AreaType::secondaryPitch,
            AreaType::secondaryPeriod,
            AreaType::sidIsActivatedAndRatio,
            AreaType::sidArpeggio,
            AreaType::sidPitch,
            AreaType::sidBalancePercent,
            AreaType::sidLowShelfVolume,
    };
    return allAreaTypes;
}

std::vector<int> PsgInstrumentEditorViewImpl::getBarHeights(const AreaType areaType) const
{
    constexpr static auto minimumHeight = AllBarsAreaConstants::minimumHeight;
    constexpr static auto normalHeight = AllBarsAreaConstants::normalHeight;
    constexpr static auto largeHeight = AllBarsAreaConstants::largeHeight;
    constexpr static auto largerHeight = AllBarsAreaConstants::largerHeight;
    constexpr static auto veryLargeHeight = AllBarsAreaConstants::veryLargeHeight;

    // Returned values must always have the full size.
    switch (areaType) {
        default:
            jassertfalse;          // Should never happen!
        case AreaType::soundType:
            return { minimumHeight, normalHeight, normalHeight, normalHeight };
        case AreaType::envelope:
        case AreaType::noise:
            return { minimumHeight, normalHeight, largeHeight, largeHeight };
        case AreaType::primaryPeriod:
        case AreaType::secondaryPeriod:
            return { minimumHeight, largeHeight, largerHeight, veryLargeHeight };
        case AreaType::primaryArpeggioNoteInOctave:
        case AreaType::secondaryArpeggioNoteInOctave:
        case AreaType::sidArpeggio:
        case AreaType::sidPitch:
            return { minimumHeight, normalHeight, normalHeight, normalHeight };
        case AreaType::primaryArpeggioOctave:
        case AreaType::secondaryArpeggioOctave:
            return { minimumHeight, normalHeight, largeHeight, veryLargeHeight };
        case AreaType::primaryPitch:
        case AreaType::secondaryPitch:
            return { minimumHeight, normalHeight, largeHeight, veryLargeHeight };

        case AreaType::sidBalancePercent:
            return { minimumHeight, normalHeight, largeHeight, largeHeight };
        case AreaType::sidLowShelfVolume:
            return { minimumHeight, normalHeight, largeHeight, largeHeight };
        case AreaType::sidIsActivatedAndRatio:
            return { minimumHeight, normalHeight, largeHeight, largeHeight };
    }
}

juce::String PsgInstrumentEditorViewImpl::getDisplayedName(const AreaType areaType) const
{
    switch (areaType) {
        default:
            jassertfalse;          // Should never happen!
        case AreaType::soundType:
            return juce::translate("Sound type");
        case AreaType::envelope:
            return juce::translate("Envelope");
        case AreaType::noise:
            return juce::translate("Noise");
        case AreaType::primaryPeriod:
        case AreaType::secondaryPeriod:
            return juce::translate("Period");
        case AreaType::primaryArpeggioNoteInOctave:
        case AreaType::secondaryArpeggioNoteInOctave:
            return juce::translate("Arp note");
        case AreaType::primaryArpeggioOctave:
        case AreaType::secondaryArpeggioOctave:
            return juce::translate("Arp octave");
        case AreaType::primaryPitch:
        case AreaType::secondaryPitch:
            return juce::translate("Pitch");
        case AreaType::sidBalancePercent:
            return juce::translate("Balance");
        case AreaType::sidLowShelfVolume:
            return juce::translate("2nd vol.");
        case AreaType::sidIsActivatedAndRatio:
            return juce::translate("Ratio");
        case AreaType::sidArpeggio:
            return juce::translate("Arp");
        case AreaType::sidPitch:
            return juce::translate("Pitch");
    }
}

std::vector<std::pair<AreaType, juce::String>> PsgInstrumentEditorViewImpl::getSections() const
{
    return {
            { AreaType::primaryArpeggioOctave, juce::translate("Primary section") },
            { AreaType::secondaryArpeggioOctave, juce::translate("Secondary section") },
            { AreaType::sidIsActivatedAndRatio, juce::translate("SID section") },
    };
}


// ApplicationCommandTarget method implementations.
// ==================================================

bool PsgInstrumentEditorViewImpl::perform(const InvocationInfo& info)
{
    auto success = true;
    switch (info.commandID) {
        case psgBarEditorToggleRetrig:
            getController().onUserWantsToToggleRetrig();
            break;
        case psgBarEditorGenerateIncreasingVolume:
            getController().onUserWantsToGenerateIncreasingVolume();
            break;
        case psgBarEditorGenerateDecreasingVolume:
            getController().onUserWantsToGenerateDecreasingVolume();
            break;
        default:
            success = false;
            break;
    }

    return success || EditorWithBarsViewImpl::perform(info);
}

}   // namespace arkostracker
