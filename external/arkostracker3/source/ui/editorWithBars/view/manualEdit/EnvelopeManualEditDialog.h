#pragma once

#include "../../../components/ColoredImage.h"
#include "BaseManualEditDialog.h"

namespace arkostracker
{

class EnvelopeManualEditDialog final : public BaseManualEditDialog
{
public:
    /**
     * Constructor.
     * @param validateCallback called when OK is clicked, will validate or not the given String. The returned String, if not empty, is an error message.
     * The client may close this Dialog if it considers the entry valid.
     * @param closeCallback called when dialog can be closed.
     */
    EnvelopeManualEditDialog(const std::function<juce::String(juce::String)>& validateCallback, const std::function<void()>& closeCallback) noexcept;

protected:
    int addComponents(int x, int y, int width) noexcept override;
    std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> getTextEditorRestriction() noexcept override;

private:
    static const juce::String textEditorRestriction;

    /**
     * Displays an envelope with its name.
     * \param x the X. Modified to evolve.
     * \param y the Y.
     * \param hardwareLabel the label to set the position to.
     * \param hardwareImage the image to set the position to.
     */
    void displayEnvelope(int& x, int y, juce::Label& hardwareLabel, juce::Component& hardwareImage) noexcept;

    juce::Label hardwareTextLabel;

    juce::Label hardware8Label;
    ColoredImage hardware8Image;
    juce::Label hardwareALabel;
    ColoredImage hardwareAImage;
    juce::Label hardwareCLabel;
    ColoredImage hardwareCImage;
    juce::Label hardwareELabel;
    ColoredImage hardwareEImage;
};

}   // namespace arkostracker
