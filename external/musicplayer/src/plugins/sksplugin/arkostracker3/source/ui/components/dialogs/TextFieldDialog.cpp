#include "TextFieldDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

TextFieldDialog::TextFieldDialog(const juce::String& pTitle, const juce::String& pText, const juce::String& pTextFieldText,
                                 std::function<void(const juce::String&)> pTextValidatedCallback, std::function<void()> pCancelCallback,
                                 const bool pAuthorizeEmptyText, const int pWidth, const int pHeight) noexcept :
        ModalDialog(pTitle, pWidth, pHeight,
                    [&] { onOkButtonClicked(); },
                    [&] { onDialogCancelled(); },
                    true, true),
        textValidatedCallback(std::move(pTextValidatedCallback)),
        cancelCallback(std::move(pCancelCallback)),
        label(juce::String(), pText),
        textEditor(),
        authorizeEmptyText(pAuthorizeEmptyText)
{
    addComponentToModalDialog(label);
    addComponentToModalDialog(textEditor);

    // This will give focus to the TextEditor.
    setWantsKeyboardFocus(false);

    const auto margins = LookAndFeelConstants::margins;
    const auto bounds = getUsableModalDialogBounds();
    const auto x = bounds.getX();
    const auto y = bounds.getY();
    label.setBounds(x, y, bounds.getWidth(), LookAndFeelConstants::labelsHeight);
    textEditor.setBounds(x, label.getBottom() + margins, bounds.getWidth(), LookAndFeelConstants::labelsHeight);

    textEditor.addListener(this);
    textEditor.setText(pTextFieldText);
    setOkButtonEnablementFromTextEditor();          // For whatever reason, the setText does not call the listener at first, so we do it manually.

    giveFocus(textEditor);
}

void TextFieldDialog::setOkButtonEnablementFromTextEditor() noexcept
{
    // No used character? Then disable OK, if wanted to.
    const auto empty = textEditor.getText().trim().isEmpty();

    const auto enabled = authorizeEmptyText || !empty;
    setOkButtonEnable(enabled);
}

void TextFieldDialog::setTextRestriction(juce::TextEditor::InputFilter& restriction) noexcept
{
    textEditor.setInputFilter(&restriction, false);
}

void TextFieldDialog::onOkButtonClicked() const noexcept
{
    if (isOkButtonEnabled()) {
        textValidatedCallback(textEditor.getText());
    }
}

void TextFieldDialog::onDialogCancelled() const noexcept
{
    cancelCallback();
}


// TextEditor::Listener method implementations.
// =============================================

void TextFieldDialog::textEditorTextChanged(juce::TextEditor& /*editor*/)
{
    setOkButtonEnablementFromTextEditor();
}

void TextFieldDialog::textEditorReturnKeyPressed(juce::TextEditor& /*editor*/)
{
    onOkButtonClicked();
}

void TextFieldDialog::textEditorEscapeKeyPressed(juce::TextEditor& /*editor*/)
{
    onDialogCancelled();
}

}   // namespace arkostracker
