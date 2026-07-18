#include "NoInstrumentEditorViewImpl.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../../utils/KeyboardFocusTraverserCustom.h"

namespace arkostracker 
{

NoInstrumentEditorViewImpl::NoInstrumentEditorViewImpl() noexcept :
        label(juce::String(), juce::translate("No instrument is selected."))
{
    addAndMakeVisible(label);
}

void NoInstrumentEditorViewImpl::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto labelHeight = LookAndFeelConstants::labelsHeight;

    label.setBounds(0, (height - labelHeight) / 2, width, labelHeight);
    label.setJustificationType(juce::Justification::centred);
}

void NoInstrumentEditorViewImpl::getKeyboardFocus() noexcept
{
    grabKeyboardFocus();
}

}   // namespace arkostracker
