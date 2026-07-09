#include "SplashDialog.h"

#include "../../ProjectInfo.h"          // Generated file in the build folder.
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

void SplashDialog::setUpVersionLabel(juce::Label& label) noexcept
{
    label.setJustificationType(juce::Justification::centredRight);
    label.setText(juce::translate("v") + projectinfo::applicationVersion, juce::NotificationType::dontSendNotification);
    label.getProperties().set(LookAndFeelConstants::labelPropertyFontSize, static_cast<float>(LookAndFeelConstants::labelFontSize) - 3.0F);
    label.setColour(juce::Label::ColourIds::textColourId,
        juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground))
        .brighter(0.7F));
}

}   // namespace arkostracker
