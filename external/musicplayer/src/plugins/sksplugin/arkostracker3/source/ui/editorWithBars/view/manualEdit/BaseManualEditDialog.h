#pragma once

#include "../../../components/EditText.h"
#include "../../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

/**
 * A base dialog for manuel/quick editing. WARNING! The setup method MUST be called!
 * By default, signed hex keys are allowed.
 */
class BaseManualEditDialog : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param title the title of the window.
     * @param mainText the main text to display.
     * @param validateCallback called when OK is clicked, will validate or not the given String. The returned String, if not empty, is an error message.
     * The client may close this Dialog if it considers the entry valid.
     * @param closeCallback called when dialog can be closed.
     * @param width the width.
     * @param height the height.
     */
    BaseManualEditDialog(const juce::String& title, const juce::String& mainText, std::function<juce::String(juce::String)> validateCallback,
        std::function<void()> closeCallback,
        int width = 450, int height = 210) noexcept;

    /** Sets up the views. */
    void setup() noexcept;

protected:
    static const int characterMaximumCount;

    /**
     * Called for subclasses to add components below the main text. The default implementation does nothing.
     * @param x the X where your can add your components.
     * @param y the Y where your can add your components (without margins from what is above).
     * @param width the width your components can use.
     * @return the height that they take. 0 if none.
     */
    virtual int addComponents(int x, int y, int width) noexcept;

    /** @return a possible text restriction. The default one is signed hex. */
    virtual std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> getTextEditorRestriction() noexcept;

private:
    /** Called when OK is clicked. */
    void onOkButtonClicked() noexcept;
    /** Called when Cancel is clicked. */
    void onCancelButtonClicked() noexcept;

    std::function<juce::String(juce::String)> validateCallback;
    std::function<void()> closeCallback;

    juce::Label mainTextLabel;
    EditText textEditor;
    std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> textEditorRestriction;
    juce::Label errorLabel;
};

}   // namespace arkostracker
