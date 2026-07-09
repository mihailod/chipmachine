#include "ProgressDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ProgressDialog::ProgressDialog(const juce::String& title, const int progressMaximumValue, const std::function<void()>& onCancelClicked) noexcept :
    ModalDialog(title, dialogWidth, calculateHeight((onCancelClicked != nullptr), true), nullptr, onCancelClicked, false, true, false),
    showProgressBar(true),
    progressBar(std::make_unique<ProgressBarView>()),
    progressLabel()
{
    initialize();

    progressBar->setMaximumProgressValue(progressMaximumValue);

    const auto width = getUsableModalDialogBounds().getWidth();
    const auto margins = LookAndFeelConstants::margins;

    progressBar->setBounds(margins, margins, width, progressBarHeight);
    addComponentToModalDialog(*progressBar);
}

ProgressDialog::ProgressDialog(const juce::String& title, const juce::String& text, const std::function<void()>& onCancelClicked) noexcept :
    ModalDialog(title, dialogWidth, calculateHeight((onCancelClicked != nullptr), false), nullptr, onCancelClicked, false, true, false),
    showProgressBar(false),
    progressBar(),
    progressLabel(std::make_unique<juce::Label>(juce::String(), text))
{
    initialize();

    progressLabel->setJustificationType(juce::Justification::centred);

    const auto bounds = getUsableModalDialogBounds();
    const auto width = bounds.getWidth();
    const auto left = bounds.getX();
    const auto top = bounds.getY();

    progressLabel->setBounds(left, top, width, textHeight);
    addComponentToModalDialog(*progressLabel);
}

void ProgressDialog::initialize()
{
    constexpr auto buttonWidth = 100;
    const auto bounds = getUsableModalDialogBounds();
    const auto width = bounds.getWidth();
    const auto x = bounds.getX();
    setCancelButtonWidth(buttonWidth);
    setCancelButtonX(((width - buttonWidth) / 2) + x);
}

int ProgressDialog::calculateHeight(const bool canCancel, const bool showProgressBar) noexcept
{
    auto height = (canCancel ? 70 : 0);
    if (showProgressBar) {
        height += progressBarHeight + (2 * LookAndFeelConstants::margins);
    } else {
        height += textHeight;
    }
    return height;
}

void ProgressDialog::setProgress(const int progress) const noexcept
{
    if (showProgressBar) {
        progressBar->setProgressValue(progress);
    }
}

void ProgressDialog::setProgressMaximumValue(const int progressMaximumValue) const noexcept
{
    if (showProgressBar) {
        progressBar->setMaximumProgressValue(progressMaximumValue);
    }
}

}   // namespace arkostracker
