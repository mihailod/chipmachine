#include "BaseManualEditDialog.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../../utils/TextEditorUtil.h"

namespace arkostracker
{

const int BaseManualEditDialog::characterMaximumCount = 3 * 32;

BaseManualEditDialog::BaseManualEditDialog(const juce::String& title, const juce::String& pMainText, std::function<juce::String(juce::String)> pValidateCallback,
    std::function<void()> pCloseCallback, const int pWidth, const int pHeight) noexcept :
    ModalDialog(title, pWidth, pHeight,
        [&] { onOkButtonClicked(); },
        [&] { onCancelButtonClicked(); },
        true, true),
    validateCallback(std::move(pValidateCallback)),
    closeCallback(std::move(pCloseCallback)),
    mainTextLabel(juce::String(), pMainText),
    textEditor(juce::String(), true, true),
    textEditorRestriction(TextEditorUtil::buildRestrictionForQuickEditPositive(characterMaximumCount))
{
}

void BaseManualEditDialog::setup() noexcept
{
    const auto borders = getUsableModalDialogBounds();
    const auto width = borders.getWidth();
    const auto top = borders.getY();
    const auto left = borders.getX();
    const auto bottom = getButtonsY() - LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    mainTextLabel.setBounds(left, top, width, labelsHeight);
    auto y = mainTextLabel.getBottom();

    const auto addedComponentsHeight = addComponents(left, y, width);
    y += addedComponentsHeight;

    errorLabel.setBounds(left, bottom - labelsHeight, width, labelsHeight);
    errorLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::red);

    const auto textEditorY = y;
    const auto textEditorHeight = errorLabel.getY() - textEditorY - margins;
    textEditor.setBounds(left, textEditorY, width, textEditorHeight);

    if (auto newTextEditorRestriction = getTextEditorRestriction(); newTextEditorRestriction != nullptr) {
        textEditorRestriction = std::move(newTextEditorRestriction);
    }
    textEditor.setInputFilter(textEditorRestriction.get(), false);
    textEditor.onReturnKey = [&] { onOkButtonClicked(); };
    textEditor.onEscapeKey = [&] { onCancelButtonClicked(); };

    addComponentToModalDialog(mainTextLabel);
    addComponentToModalDialog(textEditor);
    addComponentToModalDialog(errorLabel);

    giveFocus(textEditor, false);
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> BaseManualEditDialog::getTextEditorRestriction() noexcept
{
    return nullptr;
}

void BaseManualEditDialog::onOkButtonClicked() noexcept
{
    const auto errorMessage = validateCallback(textEditor.getText());
    errorLabel.setText(errorMessage, juce::NotificationType::dontSendNotification);

    if (errorMessage.isEmpty()) {
        closeCallback();
    }
}

void BaseManualEditDialog::onCancelButtonClicked() noexcept
{
    closeCallback();
}

int BaseManualEditDialog::addComponents(int /*x*/, int /*y*/, int /*width*/) noexcept
{
    return 0;
}

}   // namespace arkostracker
