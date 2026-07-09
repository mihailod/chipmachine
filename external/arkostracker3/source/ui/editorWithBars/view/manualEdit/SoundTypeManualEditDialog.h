#pragma once

#include "../../../components/ColoredImage.h"
#include "BaseManualEditDialog.h"

namespace arkostracker
{

class SoundTypeManualEditDialog final : public BaseManualEditDialog
{
public:
    /**
     * Constructor.
     * @param validateCallback called when OK is clicked, will validate or not the given String. The returned String, if not empty, is an error message.
     * The client may close this Dialog if it considers the entry valid.
     * @param closeCallback called when dialog can be closed.
     */
    SoundTypeManualEditDialog(const std::function<juce::String(juce::String)>& validateCallback, const std::function<void()>& closeCallback) noexcept;

protected:
    int addComponents(int x, int y, int width) noexcept override;
    std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> getTextEditorRestriction() noexcept override;

private:
    static const juce::String textEditorRestriction;

    /**
     * Displays an envelope with its name.
     * @param x the X.
     * @param y the Y. Modified to evolve.
     * @param label the label to set the position to.
     * @param image the image to set the position to.
     * @param valueLabel the value label to set the position to.
     */
    void displayEnvelope(int x, int& y, juce::Label& label, Component& image, juce::Label& valueLabel) noexcept;

    juce::Label linkNoSoftNoHardLabel;
    ColoredImage linkNoSoftNoHardImage;
    juce::Label linkNoSoftNoHardValueLabel;

    juce::Label softwareOnlyLabel;
    ColoredImage softwareOnlyImage;
    juce::Label softwareOnlyValueLabel;

    juce::Label hardwareOnlyLabel;
    ColoredImage hardwareOnlyImage;
    juce::Label hardwareOnlyValueLabel;

    juce::Label softwareToHardwareLabel;
    ColoredImage softwareToHardwareImage;
    juce::Label softwareToHardwareValueLabel;

    juce::Label hardwareToSoftwareLabel;
    ColoredImage hardwareToSoftwareImage;
    juce::Label hardwareToSoftwareValueLabel;

    juce::Label softwareAndHardwareLabel;
    ColoredImage softwareAndHardwareImage;
    juce::Label softwareAndHardwareValueLabel;
};

}   // namespace arkostracker
