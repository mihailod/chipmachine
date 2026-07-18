#include "EditMarkerDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

EditMarkerDialog::EditMarkerDialog(EditMarkerDialog::Listener& pListener, int pPosition, const juce::String& pMarkerName, juce::Colour pMarkerColor) noexcept :
        ModalDialog(juce::translate("Edit marker"), 400, 110,
                    [&] { onDialogOk(); },
                    [&] { onDialogCancelled(); },
                    true, true),
        listener(pListener),
        position(pPosition),
        nameLabel(juce::String(), juce::translate("Name")),
        nameTextEditor(),
        colorView(*this, pMarkerColor),
        deleteButton(juce::translate("Delete")),
        modalDialog()
{
    const auto buttonsHeight = LookAndFeelConstants::buttonsHeight;
    const auto margins = LookAndFeelConstants::margins;

    const auto& usableBounds = getUsableModalDialogBounds();
    const auto left = usableBounds.getX();
    const auto width = usableBounds.getWidth();
    const auto right = usableBounds.getX() + width;
    const auto top = usableBounds.getY();
    constexpr auto colorWidth = ColorView::preferredSize;

    // Sets up the Delete Button.
    deleteButton.setBounds(left, getButtonsY(), 80, buttonsHeight);
    deleteButton.onClick = [&] { onDeleteButtonClicked(); };

    // Sets up the labels, TextEditor and ColorView.
    const auto nameLabelY = top;
    nameLabel.setBounds(left, top, 55, buttonsHeight);
    colorView.setBounds(right - colorWidth, nameLabelY, colorWidth, buttonsHeight);
    const auto textEditorX = nameLabel.getRight();
    // The TextEditor in the middle.
    nameTextEditor.setBounds(textEditorX, nameLabelY, colorView.getX() - 2 * margins - textEditorX, buttonsHeight);
    nameTextEditor.setText(pMarkerName);
    nameTextEditor.setInputRestrictions(20);
    nameTextEditor.addListener(this);
    nameTextEditor.setEscapeAndReturnKeysConsumed(false);

    addComponentToModalDialog(nameLabel);
    addComponentToModalDialog(nameTextEditor);
    addComponentToModalDialog(colorView);
    addComponentToModalDialog(deleteButton);

    updateOkButtonEnablementFromText();
}

void EditMarkerDialog::updateOkButtonEnablementFromText() noexcept
{
    const auto enabled = nameTextEditor.getText().trim().isNotEmpty();
    setOkButtonEnable(enabled);
}

void EditMarkerDialog::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &nameTextEditor) {
        updateOkButtonEnablementFromText();
    } else {
        ModalDialog::textEditorTextChanged(editor);
    }
}

void EditMarkerDialog::onDialogOk() noexcept
{
    listener.onWantToEditMarker(position, nameTextEditor.getText(), colorView.getColor());
}

void EditMarkerDialog::onDialogCancelled() noexcept
{
    listener.onWantToCancelEditMarker();
}

void EditMarkerDialog::onDeleteButtonClicked() noexcept
{
    listener.onWantToDeleteMarker(position);
}


// ColorView::Listener method implementations.
// ============================================

void EditMarkerDialog::onColorClicked(const juce::Colour& currentColor, int /*viewId*/) noexcept
{
    jassert(modalDialog == nullptr);            // Modal already present? Abnormal.
    modalDialog = std::make_unique<ColorChooser>(*this, &currentColor);
}


// ColorChooser::Listener method implementations.
// ================================================

void EditMarkerDialog::onColorSelectedInColorChooser(juce::Colour color) noexcept
{
    modalDialog.reset();
    colorView.setColorAndRepaint(color);
}

void EditMarkerDialog::onColorInColorChooserCancelled() noexcept
{
    modalDialog.reset();
}

}   // namespace arkostracker
