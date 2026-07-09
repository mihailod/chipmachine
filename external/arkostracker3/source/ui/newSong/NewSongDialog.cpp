#include "NewSongDialog.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

NewSongDialog::NewSongDialog(Listener& pListener, std::map<int, juce::String> pIdToTemplateNameParam, int pSelectedTemplateId) noexcept :
        ModalDialog(juce::translate("New song"), 400, 140,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        listener(pListener),
        templateLabel(juce::String(), juce::translate("Select a template")),
        templateComboBox(),
        idToTemplateName(std::move(pIdToTemplateNameParam))
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    templateLabel.setBounds(left, top, width, labelsHeight);
    templateComboBox.setBounds(left, templateLabel.getBottom(), width, labelsHeight);

    addComponentToModalDialog(templateLabel);
    addComponentToModalDialog(templateComboBox);

    // Fills the ComboBox.
    for (const auto&[id, name] : idToTemplateName) {
        templateComboBox.addItem(name, id);
    }
    templateComboBox.setSelectedId(pSelectedTemplateId, juce::NotificationType::dontSendNotification);
}

void NewSongDialog::onOkButtonClicked() const noexcept
{
    listener.onNewSongParametersValidated(templateComboBox.getSelectedId());
}

void NewSongDialog::onCancelButtonClicked() const noexcept
{
    listener.onNewSongParametersCanceled();
}

}   // namespace arkostracker
