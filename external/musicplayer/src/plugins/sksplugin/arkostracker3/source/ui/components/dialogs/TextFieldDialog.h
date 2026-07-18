#pragma once

#include "../EditText.h"
#include "ModalDialog.h"

namespace arkostracker 
{

/**
 * Dialog showing a text field and an OK/Cancel button.
 * By default, the text has no restriction. Use the setTextRestriction for that.
 */
class TextFieldDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param title the title to display.
     * @param text the text, above the text field.
     * @param textFieldText the text in the text field.
     * @param textValidatedCallback called when the text is validated.
     * @param cancelCallback called if the dialog is cancelled.
     * @param authorizeEmptyText true if empty texts are authorized.
     * @param width the window width.
     * @param height the window height.
     */
    TextFieldDialog(const juce::String& title, const juce::String& text, const juce::String& textFieldText,
                    std::function<void(const juce::String&)> textValidatedCallback, std::function<void()> cancelCallback,
                    bool authorizeEmptyText = false, int width = 500, int height = 140) noexcept;

    /** Sets the text restriction. It is retained by the caller! */
    void setTextRestriction(juce::TextEditor::InputFilter& restriction) noexcept;

protected:
    // TextEditor::Listener method implementations.
    // =============================================
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;

private:
    /** Enables or disables the OK Button according to its content. */
    void setOkButtonEnablementFromTextEditor() noexcept;

    /** Called when the OK button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the dialog is cancelled. */
    void onDialogCancelled() const noexcept;

    std::function<void(const juce::String&)> textValidatedCallback;
    std::function<void()> cancelCallback;

    juce::Label label;
    EditText textEditor;
    const bool authorizeEmptyText;
};

}   // namespace arkostracker
