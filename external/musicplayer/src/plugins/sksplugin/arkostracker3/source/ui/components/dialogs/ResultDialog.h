#pragma once

#include "SimpleTextDialog.h"

namespace arkostracker 
{

class ErrorReport;

/**
 * A dialog showing a message, and a possible button to show the StatusReport Dialog, if there is any to show.
 * This is typically used on Export.
 */
class ResultDialog final : public SimpleTextDialog
{
public:
    /**
     * Constructor.
     * @param title the title.
     * @param text the text ("export to XXX successful" for example). Another line may be added if warnings and errors.
     * @param errorReport the possible error report.
     * @param callback called once OK is clicked.
     */
    ResultDialog(const juce::String& title, const juce::String& text, const ErrorReport& errorReport, std::function<void()> callback) noexcept;

private:
    /**
     * @return the text to display. It depends on the possible warnings and errors of the possible ErrorReport.
     * @param originalText the original text.
     * @param errorReport the error report.
     */
    static juce::String buildText(const juce::String& originalText, const ErrorReport& errorReport) noexcept;

    /** Called when the Report Button is clicked. */
    void onReportButtonClicked() noexcept;
    /** Called when the Error Report Dialog OK Button is clicked. */
    void onErrorReportOkClicked() noexcept;

    const ErrorReport& errorReport;
    std::function<void()> callback;

    juce::TextButton reportButton;
    std::unique_ptr<juce::DialogWindow> errorReportDialog;
};



}   // namespace arkostracker

