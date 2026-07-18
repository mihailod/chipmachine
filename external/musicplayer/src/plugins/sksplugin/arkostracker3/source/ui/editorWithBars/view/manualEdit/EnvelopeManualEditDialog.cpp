#include "EnvelopeManualEditDialog.h"

#include <BinaryData.h>

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String EnvelopeManualEditDialog::textEditorRestriction("ABCDEFabcdef0123456789ghijGHIJ ,");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

EnvelopeManualEditDialog::EnvelopeManualEditDialog(const std::function<juce::String(juce::String)>& pValidateCallback,
    const std::function<void()>& pCloseCallback) noexcept :
        BaseManualEditDialog(juce::translate("Envelope quick edit"),
            juce::translate("Software: enter hex volumes from 0 to f, separated by comma or space."),
            pValidateCallback, pCloseCallback, 480,280),
        hardwareTextLabel(juce::String(), juce::translate("Hardware: enter the following values:")),
        hardware8Label(juce::String(), "g"),
        hardware8Image(BinaryData::HardwareEnvelope8_png, BinaryData::HardwareEnvelope8_pngSize, juce::Colours::white, true),
        hardwareALabel(juce::String(), "h"),
        hardwareAImage(BinaryData::HardwareEnvelopeA_png, BinaryData::HardwareEnvelopeA_pngSize, juce::Colours::white, true),
        hardwareCLabel(juce::String(), "i"),
        hardwareCImage(BinaryData::HardwareEnvelopeC_png, BinaryData::HardwareEnvelopeC_pngSize, juce::Colours::white, true),
        hardwareELabel(juce::String(), "j"),
        hardwareEImage(BinaryData::HardwareEnvelopeE_png, BinaryData::HardwareEnvelopeE_pngSize, juce::Colours::white, true)
{
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> EnvelopeManualEditDialog::getTextEditorRestriction() noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(characterMaximumCount, textEditorRestriction);
}

int EnvelopeManualEditDialog::addComponents(const int x, int y, const int width) noexcept
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    hardwareTextLabel.setBounds(x, y, width, labelsHeight);
    y = hardwareTextLabel.getBottom();

    auto envelopeX = x;
    const auto envelopeY = y;
    displayEnvelope(envelopeX, envelopeY, hardware8Label, hardware8Image);
    displayEnvelope(envelopeX, envelopeY, hardwareALabel, hardwareAImage);
    displayEnvelope(envelopeX, envelopeY, hardwareCLabel, hardwareCImage);
    displayEnvelope(envelopeX, envelopeY, hardwareELabel, hardwareEImage);

    addComponentToModalDialog(hardwareTextLabel);

    return 70;
}

void EnvelopeManualEditDialog::displayEnvelope(int& x, int y, juce::Label& hardwareLabel, juce::Component& hardwareImage) noexcept
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins / 2;
    constexpr auto hardwareLabelsWidth = 30;
    constexpr auto hardwareImagesWidth = hardwareLabelsWidth;
    constexpr auto hardwareImagesHeight = 10;

    y += margins;
    hardwareImage.setBounds(x, y, hardwareImagesWidth, hardwareImagesHeight);
    hardwareLabel.setBounds(x, hardwareImage.getBottom() - 2, hardwareLabelsWidth, labelsHeight);
    hardwareLabel.setJustificationType(juce::Justification::horizontallyCentred);

    addComponentToModalDialog(hardwareLabel);
    addComponentToModalDialog(hardwareImage);

    x += hardwareLabelsWidth;
}

}   // namespace arkostracker
