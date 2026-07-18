#include "ExpressionTableEditorViewNone.h"

namespace arkostracker 
{

ExpressionTableEditorViewNone::ExpressionTableEditorViewNone(bool pIsArpeggio) noexcept :
        isArpeggio(pIsArpeggio),
        label(juce::String(),
              isArpeggio ? juce::translate("No arpeggio is selected.") :
              juce::translate("No pitch is selected."))
{
    addAndMakeVisible(label);
}

void ExpressionTableEditorViewNone::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto labelHeight = LookAndFeelConstants::labelsHeight;

    label.setBounds(0, (height - labelHeight) / 2, width, labelHeight);
    label.setJustificationType(juce::Justification::centred);
}

}   // namespace arkostracker
