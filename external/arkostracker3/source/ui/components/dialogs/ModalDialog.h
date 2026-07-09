#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * A Modal Dialog. Clients can use it and simply call the addComponentToModalDialog to add visible Components.
 * The Callback can be either the client directly, or its parent (which is why the callback is made accessible by subclasses).
 *
 * Warning, to get the right width/height, better use the getWidth/HeightInModalDialog methods!
 *
 * The dialog is closed by deleting it, there is no specific method for that. The "close" icon at the top-right will call the callback for the client
 * to delete the dialog. It means that if no callback is given, the callback will not be called! It is possible to override closeButtonPressed() to
 * implement a specific behavior.
 *
 * Note that the "close" button calls the "cancel button clicked" method.
 *
 * TextEditors can "addListener(this)" and it will automatically allow Enter/Esc to simulate OK/Cancel.
 */
class ModalDialog : public juce::DialogWindow,
                    protected juce::TextEditor::Listener        // To allow TextEditor to register to Enter and Esc to simulate OK/Cancel.
{
public:
    /**
     * Constructor.
     * @param title the title of the Dialog.
     * @param width the width.
     * @param height the height.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
     * @param showOkButton true to show the Ok Button.
     * @param showCancelButton true to show the Cancel Button.
     * @param resizable true if the Dialog can be resized.
     */
    ModalDialog(const juce::String& title, int width, int height, std::function<void()> okCallback, std::function<void()> cancelCallback,
        bool showOkButton = true, bool showCancelButton = false, bool resizable = false) noexcept;

    /** Adds a Component to the Modal Dialog. */
    void addComponentToModalDialog(Component& component, bool visible = true) const noexcept;

    /** Returns the bounds that can be used (without the margins and the possible bottom buttons). */
    juce::Rectangle<int> getUsableModalDialogBounds() const noexcept;

    /** Indicates whether the OK Button is enabled (and visible) or not. */
    bool isOkButtonEnabled() const noexcept;

    /** @return the Y of the Buttons at the bottom. */
    int getButtonsY() const noexcept;
    /** @return the heights of the Buttons at the bottom. */
    static int getButtonsHeight() noexcept;
    /** @return the X of the OK Button at the bottom. */
    int getOkButtonsX() const noexcept;

    /**
     * Sets the OK button enabled or not.
     * @param enabled true if enabled.
     */
    void setOkButtonEnable(bool enabled) noexcept;

    /**
     * Gives focus to the given Component. It waits a tiny bit before giving it, which works fine when creating the Dialog.
     * @param component the Component to give focus to.
     * @param selectAll if true, selects the whole text, if the component is a TextEditor.
     */
    static void giveFocus(Component& component, bool selectAll = true) noexcept;

    /** Hides the dialog. */
    void hide() noexcept;

    // DialogWindow methods override.
    // ==============================================================================
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress& keyPress) override;
    void lookAndFeelChanged() override;

protected:
    static constexpr auto autoOkButtonWidth = 60;            // Size used when the OK button is centered automatically.
    static constexpr auto autoCancelButtonWidth = 80;        // Size used when the Cancel button is centered automatically.

    /** Helper method to set the position and size of the given OK or Cancel Button, centered at the bottom. They are not added to the UI here. */
    void setBoundsOkOrCancelButton(Component& button) const noexcept;

    /**
     * Helper method to set the position and size of the given OK and Cancel Button, centered at the bottom. They are not added to the UI here.
     * @param okButton the OK Button.
     * @param cancelButton the Cancel Button.
     */
    void setBoundsOkAndCancelButtons(Component& okButton, Component& cancelButton) const noexcept;

    /**
     * Sets the text to the OK Button.
     * @param text the text.
     */
    void setOkButtonText(const juce::String& text) noexcept;

    /**
     * Sets the text to the Cancel Button.
     * @param text the text.
     */
    void setCancelButtonText(const juce::String& text) noexcept;

    /**
     * Called when the OK button is pressed. It will call the callback.
     * This can be called when simulating a click, for example when Enter is pressed.
     * Warning, nothing happens if the OK button is disabled.
     */
    void okPressed() const noexcept;

    /** Sets the X of the OK button. */
    void setOkButtonX(int x) noexcept;
    /** Sets the width of the OK button. */
    void setOkButtonWidth(int width) noexcept;
    /** Sets the width of the Cancel button. */
    void setCancelButtonWidth(int width) noexcept;
    /** Sets the X of the Cancel button. */
    void setCancelButtonX(int x) noexcept;

    Component& getMainComponent() const noexcept;

private:

    /**
     * Called when the Cancel button is pressed. It will call onCancelButtonClicked and the callback.
     * This can be called when simulating a click, for example when Enter is pressed.
     */
    void cancelPressed() const noexcept;

    std::function<void()> okCallback;
    std::function<void()> cancelCallback;

    juce::TextButton okButton;                          // The OK Button.
    juce::TextButton cancelButton;                      // The Cancel Button. May be invisible.

    std::unique_ptr<Component> mainComponent;           // Holds the views.

    int heightAboveButtonsWithoutMargin;                // The height that can be used, above the Buttons, without the margins on both sides.

    std::unique_ptr<juce::TooltipWindow> tooltipWindow; // To have Tooltips.
};

}   // namespace arkostracker
