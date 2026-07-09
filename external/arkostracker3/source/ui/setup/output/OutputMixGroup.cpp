#include "OutputMixGroup.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

OutputMixGroup::OutputMixGroup(juce::Component& parent) noexcept :
        mixGroup(juce::translate("Channel mix"), true),
        flatMixToggle(juce::translate("Flat mix")),
        cpcMixToggle(juce::translate("Amstrad CPC stereo HP mix (100% - 91% - 100%)")),
        customMixToggle(juce::translate("Custom mix")),
        customLeftVolumeLabel(juce::String(), juce::translate("Left channel volume")),
        customLeftVolumeSlider(juce::String()),
        customMiddleVolumeLabel(juce::String(), juce::translate("Center channel volume")),
        customMiddleVolumeSlider(juce::String()),
        customRightVolumeLabel(juce::String(), juce::translate("Right channel volume")),
        customRightVolumeSlider(juce::String())
{
    for (auto* slider : { &customMiddleVolumeSlider, &customLeftVolumeSlider, &customRightVolumeSlider }) {
        slider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        slider->setRange(0.0, 100.0, 1.0);
    }

    parent.addAndMakeVisible(mixGroup);
}

void OutputMixGroup::locateItems(const int left, const int top, const int width) noexcept
{
    constexpr auto mixLabelsX = 20;
    constexpr auto mixVolumeLabelsWidth = 160;
    constexpr auto mixSlidersWidth = 200;
    constexpr auto mixRadiogroupId = 4561;  // Random.
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margin = LookAndFeelConstants::margins;

    mixGroup.setBounds(left, top, width, mixGroupHeight);
    auto mixGroupArea = mixGroup.getGroupInnerArea();
    flatMixToggle.setBounds(mixGroupArea.removeFromTop(labelsHeight));
    cpcMixToggle.setBounds(mixGroupArea.removeFromTop(labelsHeight));
    customMixToggle.setBounds(mixGroupArea.removeFromTop(labelsHeight));
    customLeftVolumeLabel.setBounds(mixGroupArea.removeFromTop(labelsHeight).withWidth(mixVolumeLabelsWidth).withX(mixLabelsX));
    customLeftVolumeSlider.setBounds(customLeftVolumeLabel.getRight() + margin, customLeftVolumeLabel.getY(), mixSlidersWidth, labelsHeight);
    customMiddleVolumeLabel.setBounds(mixGroupArea.removeFromTop(labelsHeight).withWidth(mixVolumeLabelsWidth).withX(mixLabelsX));
    customMiddleVolumeSlider.setBounds(customMiddleVolumeLabel.getRight() + margin, customMiddleVolumeLabel.getY(), mixSlidersWidth, labelsHeight);
    customRightVolumeLabel.setBounds(mixGroupArea.removeFromTop(labelsHeight).withWidth(mixVolumeLabelsWidth).withX(mixLabelsX));
    customRightVolumeSlider.setBounds(customRightVolumeLabel.getRight() + margin, customRightVolumeLabel.getY(), mixSlidersWidth, labelsHeight);
    flatMixToggle.setRadioGroupId(mixRadiogroupId);
    cpcMixToggle.setRadioGroupId(mixRadiogroupId);
    customMixToggle.setRadioGroupId(mixRadiogroupId);
    customMixToggle.onStateChange = [&] { updateCustomMixSlidersFromToggleEnablement(); };

    mixGroup.addComponentToGroup(flatMixToggle);
    mixGroup.addComponentToGroup(cpcMixToggle);
    mixGroup.addComponentToGroup(customMixToggle);
    mixGroup.addComponentToGroup(customLeftVolumeLabel);
    mixGroup.addComponentToGroup(customLeftVolumeSlider);
    mixGroup.addComponentToGroup(customMiddleVolumeLabel);
    mixGroup.addComponentToGroup(customMiddleVolumeSlider);
    mixGroup.addComponentToGroup(customRightVolumeLabel);
    mixGroup.addComponentToGroup(customRightVolumeSlider);
}

void OutputMixGroup::updateCustomMixSlidersFromToggleEnablement() noexcept
{
    const auto selected = customMixToggle.getToggleState();

    const auto components = std::vector<juce::Component*> {
        &customLeftVolumeLabel, &customLeftVolumeSlider,
        &customMiddleVolumeLabel, &customMiddleVolumeSlider,
        &customRightVolumeLabel, &customRightVolumeSlider,
    };

    for (auto* component : components) {
        component->setEnabled(selected);
    }
}

void OutputMixGroup::fillUiFromData(const int customVolumeLeft, const int customVolumeCenter, const int customVolumeRight, const StoredOutputMix::StereoMixType stereoMix) noexcept
{
    customLeftVolumeSlider.setValue(customVolumeLeft, juce::NotificationType::dontSendNotification);
    customMiddleVolumeSlider.setValue(customVolumeCenter, juce::NotificationType::dontSendNotification);
    customRightVolumeSlider.setValue(customVolumeRight, juce::NotificationType::dontSendNotification);

    juce::ToggleButton* toggleButton;       // NOLINT(*-init-variables)
    switch (stereoMix) {
        default:
            jassertfalse;       // No linked Toggle?
        case StoredOutputMix::StereoMixType::flat:
            toggleButton = &flatMixToggle;
        break;
        case StoredOutputMix::StereoMixType::cpc:
            toggleButton = &cpcMixToggle;
        break;
        case StoredOutputMix::StereoMixType::custom:
            toggleButton = &customMixToggle;
        break;
    }

    toggleButton->setToggleState(true, juce::NotificationType::sendNotification);

    updateCustomMixSlidersFromToggleEnablement();
}

OutputMixGroup::Data OutputMixGroup::retrieveData() const noexcept
{
    auto stereoMixType = StoredOutputMix::StereoMixType::custom;
    if (flatMixToggle.getToggleState()) {
        stereoMixType = StoredOutputMix::StereoMixType::flat;
    } else if (cpcMixToggle.getToggleState()) {
        stereoMixType = StoredOutputMix::StereoMixType::cpc;
    } else {
        jassert(customMixToggle.getToggleState());
    }

    return { stereoMixType, static_cast<int>(customLeftVolumeSlider.getValue()),
        static_cast<int>(customMiddleVolumeSlider.getValue()), static_cast<int>(customRightVolumeSlider.getValue()) };
}

}   // namespace arkostracker
