#include "ModalDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ModalDialog::ModalDialog(const juce::String& pTitle, const int pWidth, const int pHeight,
                         std::function<void()> pOkCallback, std::function<void()> pCancelCallback, const bool pShowOkButton,
                         const bool pShowCancelButton, const bool pResizable) noexcept :
        DialogWindow(pTitle,
                     juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground)),
                     true, true),
        okCallback(std::move(pOkCallback)),
        cancelCallback(std::move(pCancelCallback)),
        okButton(juce::translate("OK")),
        cancelButton(juce::translate("Cancel")),
        mainComponent(std::make_unique<Component>()),
        heightAboveButtonsWithoutMargin(0),
        tooltipWindow(std::make_unique<juce::TooltipWindow>(this))
{
    setResizable(pResizable, false);
    centreAroundComponent(nullptr, pWidth, pHeight);

    enterModalState(true, nullptr, false);

    okButton.onClick = [&] { okPressed(); };
    cancelButton.onClick = [&] { cancelPressed(); };

    const auto margins = LookAndFeelConstants::margins;

    // Creates the main Component where all the Views will be put. No margins can be added, because it must take all the room.
    mainComponent->setBounds(getLocalBounds());
    setContentNonOwned(mainComponent.get(), false);

    auto atLeastOneButton = true;
    // Ok and Cancel Button.
    if (pShowOkButton && pShowCancelButton) {
        setBoundsOkAndCancelButtons(okButton, cancelButton);
        addComponentToModalDialog(okButton);
        addComponentToModalDialog(cancelButton);
    } else if (pShowOkButton) {
        setBoundsOkOrCancelButton(okButton);
        addComponentToModalDialog(okButton);
    } else if (pShowCancelButton) {
        setBoundsOkOrCancelButton(cancelButton);
        addComponentToModalDialog(cancelButton);
    } else {
        // No Buttons.
        atLeastOneButton = false;
    }

    if (atLeastOneButton) {
        const auto yAboveOkButton = (pShowOkButton ? okButton.getY() : cancelButton.getY()) - margins;
        heightAboveButtonsWithoutMargin = yAboveOkButton - margins;
    } else {
        heightAboveButtonsWithoutMargin = pHeight - (2 * margins);
    }
}

void ModalDialog::addComponentToModalDialog(Component& component, const bool visible) const noexcept
{
    if (visible) {
        mainComponent->addAndMakeVisible(component);
    } else {
        mainComponent->addChildComponent(component);
    }
}

juce::Component& ModalDialog::getMainComponent() const noexcept
{
    return *mainComponent;
}

juce::Rectangle<int> ModalDialog::getUsableModalDialogBounds() const noexcept
{
    const auto margins = LookAndFeelConstants::margins;
    return { margins, margins, getWidth() - (2 * margins), heightAboveButtonsWithoutMargin };
}

bool ModalDialog::isOkButtonEnabled() const noexcept
{
    return okButton.isEnabled() && okButton.isVisible();
}

void ModalDialog::setOkButtonEnable(const bool enabled) noexcept
{
    okButton.setEnabled(enabled);
}

void ModalDialog::giveFocus(Component& component, const bool selectAll) noexcept
{
    if (selectAll) {
        // Hack, but I didn't want an extra method...
        if (auto* textEditor = dynamic_cast<juce::TextEditor*>(&component); textEditor != nullptr) {
            textEditor->setHighlightedRegion({ 0, 9999999 });
        }
    }
    juce::Timer::callAfterDelay(50, [&] { component.grabKeyboardFocus(); });
}

void ModalDialog::hide() noexcept
{
    setVisible(false);
}

void ModalDialog::okPressed() const noexcept
{
    if (!isOkButtonEnabled()) {
        return;
    }

    okCallback();
}

void ModalDialog::cancelPressed() const noexcept
{
    if (cancelCallback != nullptr) {
        cancelCallback();
    }
}

void ModalDialog::closeButtonPressed()
{
    cancelPressed();
}

bool ModalDialog::keyPressed(const juce::KeyPress& keyPress)
{
    // A "return" means "OK" (if the OK button is enabled).
    if ((keyPress == juce::KeyPress::returnKey) && (okCallback != nullptr) && (isOkButtonEnabled())) {
        okCallback();
        return true;
    }

    return false;
}

void ModalDialog::setBoundsOkOrCancelButton(Component& button) const noexcept
{
    const auto okCancelButtonsHeight = getButtonsHeight();

    const auto buttonX = (getWidth() - autoOkButtonWidth) / 2;
    const auto buttonY = getButtonsY();
    button.setBounds(buttonX, buttonY, autoOkButtonWidth, okCancelButtonsHeight);
}

void ModalDialog::setBoundsOkAndCancelButtons(Component& theOkButton, Component& theCancelButton) const noexcept
{
    const auto margins = LookAndFeelConstants::margins;
    const auto okCancelButtonsHeight = getButtonsHeight();

    const auto spaceBetweenButtons = margins * 2;
    const auto okAndCancelButtonsWidth = autoOkButtonWidth + spaceBetweenButtons + autoCancelButtonWidth;

    const auto okButtonX = (getWidth() - okAndCancelButtonsWidth) / 2;
    const auto cancelButtonX = okButtonX + autoOkButtonWidth + spaceBetweenButtons;
    theCancelButton.setBounds(cancelButtonX, getButtonsY(), autoCancelButtonWidth, okCancelButtonsHeight);
    theOkButton.setBounds(okButtonX, theCancelButton.getY(), autoOkButtonWidth, okCancelButtonsHeight);
}

void ModalDialog::setOkButtonText(const juce::String& text) noexcept
{
    okButton.setButtonText(text);
}

void ModalDialog::setCancelButtonText(const juce::String& text) noexcept
{
    cancelButton.setButtonText(text);
}

void ModalDialog::lookAndFeelChanged()
{
    // We have to do that for a real-time change of color.
    const auto currentColor = getBackgroundColour();
    const auto newColor = findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));
    if (currentColor != newColor) {
        setBackgroundColour(newColor);
    }
}

int ModalDialog::getButtonsY() const noexcept
{
    const auto margins = LookAndFeelConstants::margins;
    const auto okCancelButtonsHeight = getButtonsHeight();

    return getHeight() - margins - okCancelButtonsHeight - juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight();   // Seems to work...
}

int ModalDialog::getOkButtonsX() const noexcept
{
    return okButton.getX();
}

int ModalDialog::getButtonsHeight() noexcept
{
    return LookAndFeelConstants::buttonsHeight;
}

void ModalDialog::setOkButtonX(const int x) noexcept
{
    okButton.setTopLeftPosition(x, okButton.getY());
}

void ModalDialog::setOkButtonWidth(const int width) noexcept
{
    okButton.setSize(width, okButton.getHeight());
}

void ModalDialog::setCancelButtonWidth(const int width) noexcept
{
    cancelButton.setSize(width, cancelButton.getHeight());
}

void ModalDialog::setCancelButtonX(const int x) noexcept
{
    cancelButton.setTopLeftPosition(x, cancelButton.getY());
}

}   // namespace arkostracker
