#pragma once

#include <memory>

#include "../ProgressBarView.h"
#include "ModalDialog.h"

namespace arkostracker 
{

/** A modal dialog which shows a Progress Bar, and optionally a Cancel button. */
class ProgressDialog final : public ModalDialog
{
public:
    /**
     * Constructor for a dialog with a progress bar.
     * @param title the title.
     * @param progressMaximumValue the progress maximum value. Ignored if there is no progress bar.
     * @param onCancelClicked called when Cancel is clicked, if shown, or nullptr to hide the Cancel button.
    */
    ProgressDialog(const juce::String& title, int progressMaximumValue, const std::function<void()>& onCancelClicked) noexcept;

    /**
     * Constructor for a dialog with a text.
     * @param title the title.
     * @param text the text.
     * @param onCancelClicked called when Cancel is clicked, if shown, or nullptr to hide the Cancel button.
    */
    ProgressDialog(const juce::String& title, const juce::String& text, const std::function<void()>& onCancelClicked) noexcept;

    /** Sets the progress. This performs a repaint. */
    void setProgress(int progress) const noexcept;

    /** Sets the progress maximum value. This does NOT perform a repaint. */
    void setProgressMaximumValue(int progressMaximumValue) const noexcept;

private:
    static constexpr auto progressBarHeight = 40;   // Height of the ProgressBar.
    static constexpr auto dialogWidth = 500;
    static constexpr auto textHeight = 25 * 2;

    /** Call this in the constructor. */
    void initialize();

    /**
     * @return the height of the dialog.
     * @param canCancel true is the user can cancel.
     * @param showProgressBar true is the progress bar is shown.
     */
    static int calculateHeight(bool canCancel, bool showProgressBar) noexcept;

    bool showProgressBar;
    std::unique_ptr<ProgressBarView> progressBar;   // The progress bar.
    std::unique_ptr<juce::Label> progressLabel;     // A progress label, shown if there is no progress bar.
};

}   // namespace arkostracker
