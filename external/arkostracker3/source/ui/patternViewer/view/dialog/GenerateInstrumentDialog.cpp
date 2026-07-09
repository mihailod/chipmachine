#include "GenerateInstrumentDialog.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

GenerateInstrumentDialog::GenerateInstrumentDialog(std::function<void(juce::String arpeggioName, bool usePeriods)> pValidateCallback, std::function<void()> pCloseCallback) noexcept :
        ModalDialog(juce::translate("Generate instrument"), 400, 190, [&] {
            onOkButtonClicked();
        },
        [&] {
            onCancelButtonClicked();
        }, true, true),
        validateCallback(std::move(pValidateCallback)),
        closeCallback(std::move(pCloseCallback)),
        nameLabel(juce::String(), juce::translate("Enter the name of the new instrument:")),
        nameTextEditor(juce::translate("New")),
        encodeAsPeriodsToggle(juce::translate("Encode as fixed periods")),
        encodeAsNotesToggle(juce::translate("Encode as notes"))
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto left = bounds.getX();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    constexpr auto encodeAsPeriodsToggleWidth = 200;
    constexpr auto encodeAsNotesToggleWidth = 180;
    constexpr auto exportRadioGroup = 212;      // Why not?

    nameLabel.setBounds(left, top, width, labelsHeight);
    nameTextEditor.setBounds(left, nameLabel.getBottom(), width, labelsHeight);

    encodeAsPeriodsToggle.setBounds(left, nameTextEditor.getBottom() + margins * 2, encodeAsPeriodsToggleWidth, labelsHeight);
    encodeAsNotesToggle.setBounds(encodeAsPeriodsToggle.getRight(), encodeAsPeriodsToggle.getY(), encodeAsNotesToggleWidth, labelsHeight);

    encodeAsPeriodsToggle.setRadioGroupId(exportRadioGroup, juce::NotificationType::dontSendNotification);
    encodeAsNotesToggle.setRadioGroupId(exportRadioGroup, juce::NotificationType::dontSendNotification);
    encodeAsPeriodsToggle.setToggleState(true, juce::NotificationType::dontSendNotification);

    addComponentToModalDialog(nameLabel);
    addComponentToModalDialog(nameTextEditor);
    addComponentToModalDialog(encodeAsPeriodsToggle);
    addComponentToModalDialog(encodeAsNotesToggle);

    giveFocus(nameTextEditor);
}

void GenerateInstrumentDialog::onOkButtonClicked() const noexcept
{
    validateCallback(nameTextEditor.getText().trim(), encodeAsPeriodsToggle.getToggleState());
}

void GenerateInstrumentDialog::onCancelButtonClicked() const noexcept
{
    closeCallback();
}

}   // namespace arkostracker
