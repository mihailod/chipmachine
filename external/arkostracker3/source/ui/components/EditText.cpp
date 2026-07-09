#include "EditText.h"

#include "../lookAndFeel/CustomLookAndFeel.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

EditText::EditText(const juce::String& text, const bool enabled, const bool multiLine) noexcept :
        linkedComponent()
{
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
    applyFontToAllText(currentLookAndFeel.getBaseFont());

    if (multiLine) {
        setMultiLine(true, true);
    }
    setText(text, false);

    if (!enabled) {
        setReadOnly(true);
        setInterceptsMouseClicks(false, false);
        setMouseClickGrabsKeyboardFocus(false);
        setCaretVisible(false);
    }
}

void EditText::setLinkedComponent(Component* pLinkedComponent) noexcept
{
    linkedComponent = pLinkedComponent;
}

void EditText::enablementChanged()
{
    TextEditor::enablementChanged();

    const auto enabled = isEnabled();
    // The default JUCE component does not change the "color" when disabled.  The values come from the drawLabel in the L&F.
    const auto textColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(
        juce::Label::ColourIds::textColourId).withAlpha(enabled ? 1.0F : 0.5F);
    applyColourToAllText(textColor);

    // The linked component is enabled/disabled too.
    if (linkedComponent != nullptr) {
        linkedComponent->setEnabled(enabled);
    }
}

}   // namespace arkostracker
