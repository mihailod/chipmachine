#include "SoundTypeManualEditDialog.h"

#include <BinaryData.h>

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String SoundTypeManualEditDialog::textEditorRestriction("shSH+- ,");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

SoundTypeManualEditDialog::SoundTypeManualEditDialog(const std::function<juce::String(juce::String)>& pValidateCallback,
    const std::function<void()>& pCloseCallback) noexcept :
        BaseManualEditDialog(juce::translate("Sound type quick edit"),
            juce::translate("Enter either of the following values, separated by comma or space."),
            pValidateCallback, pCloseCallback, 480,400),
        linkNoSoftNoHardLabel(juce::String(), "No software, no hardware:"),
        linkNoSoftNoHardImage(BinaryData::LinkNoSoftNoHard_png, BinaryData::LinkNoSoftNoHard_pngSize, juce::Colours::white, true),
        linkNoSoftNoHardValueLabel(juce::String(), "-"),
        softwareOnlyLabel(juce::String(), "Software only:"),
        softwareOnlyImage(BinaryData::LinkSoftOnly_png, BinaryData::LinkSoftOnly_pngSize, juce::Colours::white, true),
        softwareOnlyValueLabel(juce::String(), "s"),
        hardwareOnlyLabel(juce::String(), "Hardware only:"),
        hardwareOnlyImage(BinaryData::LinkHardOnly_png, BinaryData::LinkHardOnly_pngSize, juce::Colours::white, true),
        hardwareOnlyValueLabel(juce::String(), "h"),
        softwareToHardwareLabel(juce::String(), "Software to hardware:"),
        softwareToHardwareImage(BinaryData::LinkSoftToHard_png, BinaryData::LinkSoftToHard_pngSize, juce::Colours::white, true),
        softwareToHardwareValueLabel(juce::String(), "sh"),
        hardwareToSoftwareLabel(juce::String(), "Hardware to software:"),
        hardwareToSoftwareImage(BinaryData::LinkHardToSoft_png, BinaryData::LinkHardToSoft_pngSize, juce::Colours::white, true),
        hardwareToSoftwareValueLabel(juce::String(), "hs"),
        softwareAndHardwareLabel(juce::String(), "Software and hardware:"),
        softwareAndHardwareImage(BinaryData::LinkSoftAndHard_png, BinaryData::LinkSoftAndHard_pngSize, juce::Colours::white, true),
        softwareAndHardwareValueLabel(juce::String(), "+")
{
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> SoundTypeManualEditDialog::getTextEditorRestriction() noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(characterMaximumCount, textEditorRestriction);
}

int SoundTypeManualEditDialog::addComponents(const int x, const int y, const int /*width*/) noexcept
{
    const auto envelopeX = x;
    auto envelopeY = y;
    displayEnvelope(envelopeX, envelopeY, linkNoSoftNoHardLabel, linkNoSoftNoHardImage, linkNoSoftNoHardValueLabel);
    displayEnvelope(envelopeX, envelopeY, softwareOnlyLabel, softwareOnlyImage, softwareOnlyValueLabel);
    displayEnvelope(envelopeX, envelopeY, hardwareOnlyLabel, hardwareOnlyImage, hardwareOnlyValueLabel);
    displayEnvelope(envelopeX, envelopeY, softwareToHardwareLabel, softwareToHardwareImage, softwareToHardwareValueLabel);
    displayEnvelope(envelopeX, envelopeY, hardwareToSoftwareLabel, hardwareToSoftwareImage, hardwareToSoftwareValueLabel);
    displayEnvelope(envelopeX, envelopeY, softwareAndHardwareLabel, softwareAndHardwareImage, softwareAndHardwareValueLabel);

    return 160;
}

void SoundTypeManualEditDialog::displayEnvelope(const int x, int& y, juce::Label& label, juce::Component& image, juce::Label& valueLabel) noexcept
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    constexpr auto imagesWidth = 20;
    constexpr auto labelsWidth = 180;
    constexpr auto valueLabelsWidth = 50;
    const auto hardwareImagesHeight = labelsHeight;

    image.setBounds(x, y, imagesWidth, hardwareImagesHeight);
    label.setBounds(image.getRight() + margins * 2, image.getY(), labelsWidth, labelsHeight);
    valueLabel.setBounds(label.getRight() + margins, image.getY(), valueLabelsWidth, labelsHeight);

    addComponentToModalDialog(label);
    addComponentToModalDialog(image);
    addComponentToModalDialog(valueLabel);

    y += labelsHeight;
}

}   // namespace arkostracker
