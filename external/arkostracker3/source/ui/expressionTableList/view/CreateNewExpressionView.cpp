#include "CreateNewExpressionView.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

CreateNewExpressionView::CreateNewExpressionView(CreateNewExpressionView::Listener& pListener, const int pIndex, const bool pIsArpeggio) noexcept :
        listener(pListener),
        isArpeggio(pIsArpeggio),
        index(pIndex),
        modalDialog(std::make_unique<ModalDialog>(isArpeggio ? juce::translate("Create new arpeggio") : juce::translate("Create new pitch"),
                                              400, 200,
                                              [&] { onOkButtonClicked(); },
                                              [&] { onDialogCancelled(); },
                                              true, true)),
        nameLabel(juce::String(), juce::translate("Name")),
        nameTextEditor(),

        templateLabel(juce::String(), juce::translate("Template")),
        templateComboBox()
{
    // Locates the UI items.
    const auto bounds = modalDialog->getUsableModalDialogBounds();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    const auto width = bounds.getWidth();
    const auto x = bounds.getX();
    const auto top = bounds.getY();

    nameLabel.setBounds(x, top, width, labelsHeight);
    nameTextEditor.setBounds(x, nameLabel.getBottom(), width, labelsHeight);
    nameTextEditor.setText(juce::translate("New"), false);
    nameTextEditor.onEscapeKey = [&] { onDialogCancelled(); };
    nameTextEditor.onReturnKey = [&] { onOkButtonClicked(); };

    templateLabel.setBounds(x, nameTextEditor.getBottom() + margins * 2, width, labelsHeight);
    templateComboBox.setBounds(x, templateLabel.getBottom(), width, labelsHeight);
    fillComboBox();

    modalDialog->addComponentToModalDialog(nameLabel);
    modalDialog->addComponentToModalDialog(nameTextEditor);
    modalDialog->addComponentToModalDialog(templateLabel);
    modalDialog->addComponentToModalDialog(templateComboBox);

    ModalDialog::giveFocus(nameTextEditor);
}

void CreateNewExpressionView::fillComboBox() noexcept
{
    const auto& templateToName = ExpressionTemplate::getTemplateNames(isArpeggio);
    for (const auto& [expressionTemplate, templateName] : templateToName) {
        templateComboBox.addItem(templateName, static_cast<int>(expressionTemplate) + offsetTemplateId);
    }

    templateComboBox.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
}

void CreateNewExpressionView::onOkButtonClicked() const noexcept
{
    const auto name = nameTextEditor.getText();
    const auto expressionTemplate = static_cast<ExpressionTemplate::Template>(templateComboBox.getSelectedId() - offsetTemplateId);
    listener.onCreateNewExpressionValidated(index, name, expressionTemplate);
}

void CreateNewExpressionView::onDialogCancelled() const noexcept
{
    listener.onCreateNewExpressionCancelled();
}

}   // namespace arkostracker
