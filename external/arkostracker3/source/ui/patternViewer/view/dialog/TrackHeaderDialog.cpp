#include "TrackHeaderDialog.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

TrackHeaderDialog::TrackHeaderDialog(const juce::String& pCurrentName, const OptionalValue<juce::Colour> pCurrentColor,
        const std::function<void(Result)>& pOkCallback, const std::function<void()>& pCancelCallback) noexcept :
        ModalDialog(juce::translate("Edit track"), 280, 180,
                [&] { onOkClicked(); },
                [&] { onCancelClicked(); },
                true, true
        ),
        okCallback(pOkCallback),
        cancelCallback(pCancelCallback),
        currentColor(pCurrentColor),
        trackNameLabel(juce::String(), juce::translate("Track name")),
        trackNameEditText(pCurrentName),
        trackColorToggle(juce::translate("Use color?")),
        trackColorView(*this, juce::Colour(0))      // Unused color, so that OK is not possible.
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto left = bounds.getX();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    trackNameLabel.setBounds(left, top, width, labelsHeight);
    trackNameEditText.setBounds(left, trackNameLabel.getBottom(), width, labelsHeight);
    trackColorToggle.setBounds(left, trackNameEditText.getBottom() + (margins * 2), 120, labelsHeight);
    trackColorView.setBounds(trackColorToggle.getRight(), trackColorToggle.getY(), ColorView::preferredSize, ColorView::preferredSize);

    addComponentToModalDialog(trackNameLabel);
    addComponentToModalDialog(trackNameEditText);
    addComponentToModalDialog(trackColorToggle);
    addComponentToModalDialog(trackColorView);

    trackColorToggle.onStateChange = [&] { onTrackColorToggleChanged(); };
    trackColorToggle.setToggleState(currentColor.isPresent(), juce::NotificationType::sendNotification);
    refreshViews();

    trackNameEditText.onReturnKey = [&] { onOkClicked(); };
    trackNameEditText.onEscapeKey = [&] { onCancelClicked(); };
    giveFocus(trackNameEditText);
}

void TrackHeaderDialog::onOkClicked() const noexcept
{
    const Result result = {
        trackNameEditText.getText().trim(),
        trackColorToggle.getToggleState() ? currentColor : OptionalValue<juce::Colour>()
    };

    okCallback(result);
}

void TrackHeaderDialog::onCancelClicked() const noexcept
{
    cancelCallback();
}

void TrackHeaderDialog::onTrackColorToggleChanged() noexcept
{
    refreshViews();

    // As long as there are no valid color, if the toggle is set to on, shows the dialog.
    if (currentColor.isAbsent() && trackColorToggle.getToggleState()) {
        showColorDialog();
    }
}

void TrackHeaderDialog::showColorDialog() noexcept
{
    const auto& tempColor = currentColor.getValueRef();
    modalDialog = std::make_unique<ColorChooser>(*this, currentColor.isPresent() ? &tempColor : nullptr);
}

void TrackHeaderDialog::refreshViews() noexcept
{
    trackColorView.setVisible(currentColor.isPresent() && trackColorToggle.getToggleState());
    if (currentColor.isPresent()) {
        trackColorView.setColorAndRepaint(currentColor.getValue());
    }
}

// ColorView::Listener method implementations.
// =============================================

void TrackHeaderDialog::onColorClicked(const juce::Colour& /*color*/, int /*viewId*/) noexcept
{
    showColorDialog();
}


// ColorChooser::Listener method implementations.
// ==================================================

void TrackHeaderDialog::onColorSelectedInColorChooser(const juce::Colour color) noexcept
{
    currentColor = color;
    refreshViews();
    trackColorToggle.setToggleState(true, juce::sendNotification);        // After selecting, shows the color view, more user-friendly.

    modalDialog.reset();
}

void TrackHeaderDialog::onColorInColorChooserCancelled() noexcept
{
    modalDialog.reset();

    if (currentColor.isAbsent()) {
        trackColorToggle.setToggleState(false, juce::NotificationType::sendNotification);
    }
}

}       // namespace arkostracker