#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../app/preferences/model/StoredOutputMix.h"
#include "../../components/GroupWithViewport.h"

namespace arkostracker
{

class OutputMixGroup final
{
public:
    static constexpr auto mixGroupHeight = 180;

    class Data
    {
    public:
        Data(const StoredOutputMix::StereoMixType pStereoMixType, const int pCustomLeftVolumePercent, const int pCustomCenterVolumePercent, const int pCustomRightVolumePercent) noexcept :
                stereoMixType(pStereoMixType),
                customLeftVolumePercent(pCustomLeftVolumePercent),
                customCenterVolumePercent(pCustomCenterVolumePercent),
                customRightVolumePercent(pCustomRightVolumePercent)
        {
        }

        StoredOutputMix::StereoMixType stereoMixType;
        int customLeftVolumePercent;
        int customCenterVolumePercent;
        int customRightVolumePercent;
    };

    /**
     * Constructor.
     * @param parent the parent Component to add the views to.
     */
    explicit OutputMixGroup(juce::Component& parent) noexcept;

    /**
     * Locates the various UI items.
     * @param left the X.
     * @param top the Y.
     * @param width the available width.
     */
    void locateItems(int left, int top, int width) noexcept;

    /**
     * Fills the UI with new data.
     * @param customVolumeLeft the custom left volume, from 0 to 100.
     * @param customVolumeCenter the custom center volume, from 0 to 100.
     * @param customVolumeRight the custom right volume, from 0 to 100.
     * @param stereoMix the stereo mix.
     */
    void fillUiFromData(int customVolumeLeft, int customVolumeCenter, int customVolumeRight, StoredOutputMix::StereoMixType stereoMix) noexcept;

    /** Updates the custom mix sliders according to the enablement. */
    void updateCustomMixSlidersFromToggleEnablement() noexcept;

    /** @return the data from the UI. */
    Data retrieveData() const noexcept;

private:
    GroupWithViewport mixGroup;
    juce::ToggleButton flatMixToggle;
    juce::ToggleButton cpcMixToggle;
    juce::ToggleButton customMixToggle;
    juce::Label customLeftVolumeLabel;
    juce::Slider customLeftVolumeSlider;
    juce::Label customMiddleVolumeLabel;
    juce::Slider customMiddleVolumeSlider;
    juce::Label customRightVolumeLabel;
    juce::Slider customRightVolumeSlider;
};

}   // namespace arkostracker
