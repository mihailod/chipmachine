#include "GenerateArpeggioDialog.h"

#include "../../../../utils/NoteUtil.h"
#include "../../../../utils/SetUtil.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

GenerateArpeggioDialog::GenerateArpeggioDialog(std::function<void(juce::String arpeggioName, int baseNote)> pValidateCallback, std::function<void()> pCloseCallback,
    std::vector<int> pNotes) noexcept :
        ModalDialog(juce::translate("Generate arpeggio"), 400, 200, [&] {
            onOkButtonClicked();
        },
        [&] {
            onCancelButtonClicked();
        }, true, true),
        validateCallback(std::move(pValidateCallback)),
        closeCallback(std::move(pCloseCallback)),
        notes(std::move(pNotes)),
        nameLabel(juce::String(), juce::translate("Enter the name of the new arpeggio:")),
        nameTextEditor(juce::translate("Arp")),
        baseNoteLabel(juce::String(), juce::translate("Select the base note:")),
        baseNoteComboBox(),
        noteCountLabel(juce::String(), juce::translate("Note count: ") + juce::String(notes.size()))
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto left = bounds.getX();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    nameLabel.setBounds(left, top, width, labelsHeight);
    nameTextEditor.setBounds(left, nameLabel.getBottom(), width, labelsHeight);

    baseNoteLabel.setBounds(left, nameTextEditor.getBottom() + margins, width, labelsHeight);
    baseNoteComboBox.setBounds(left, baseNoteLabel.getBottom(), 100, labelsHeight);
    constexpr auto noteCountLabelWidth = 130;
    noteCountLabel.setBounds(left + width - noteCountLabelWidth, baseNoteComboBox.getY(), noteCountLabelWidth, labelsHeight);
    noteCountLabel.setJustificationType(juce::Justification::right);

    fillBaseNoteComboBox();

    addComponentToModalDialog(nameLabel);
    addComponentToModalDialog(nameTextEditor);
    addComponentToModalDialog(baseNoteLabel);
    addComponentToModalDialog(baseNoteComboBox);
    addComponentToModalDialog(noteCountLabel);

    giveFocus(nameTextEditor);
}

void GenerateArpeggioDialog::onOkButtonClicked() const noexcept
{
    validateCallback(nameTextEditor.getText().trim(), baseNoteComboBox.getSelectedId() - baseNodeIdOffset);
}

void GenerateArpeggioDialog::onCancelButtonClicked() const noexcept
{
    closeCallback();
}

void GenerateArpeggioDialog::fillBaseNoteComboBox() noexcept
{
    const auto uniqueNotes = SetUtil::toSet(notes);
    for (const auto note : uniqueNotes) {
        baseNoteComboBox.addItem(NoteUtil::getStringFromNote(note), note + baseNodeIdOffset);
    }
    baseNoteComboBox.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
}

}   // namespace arkostracker
