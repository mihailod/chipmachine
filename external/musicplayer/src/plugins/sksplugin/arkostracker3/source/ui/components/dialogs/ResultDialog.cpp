#include "ResultDialog.h"

#include "../../../utils/ErrorReport.h"
#include "ErrorReportDialog.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ResultDialog::ResultDialog(const juce::String& pTitle, const juce::String& pText, const ErrorReport& pErrorReport, std::function<void()> pCallback) noexcept :
        SimpleTextDialog(pTitle, buildText(pText, pErrorReport), nullptr, std::move(pCallback),
                         500, pErrorReport.isEmpty() ? 150 : 130, false, true),
        errorReport(pErrorReport),
        reportButton(juce::translate("Show report")),
        errorReportDialog()
{
    setCancelButtonText(juce::translate("OK"));

    // Adds a Report button, if needed.
    if (!errorReport.isEmpty()) {
        reportButton.onClick = [&] { onReportButtonClicked(); };

        const auto margins = LookAndFeelConstants::margins;
        reportButton.setBounds(margins, getButtonsY(), 130, getButtonsHeight());
        addComponentToModalDialog(reportButton);
    }
}

juce::String ResultDialog::buildText(const juce::String& originalText, const ErrorReport& errorReport) noexcept
{
    // If no warning/error/report, simply uses the original text.
    if (errorReport.isEmpty()) {
        return originalText;
    }

    const auto warningCount = errorReport.getWarningCount();
    const auto errorCount = errorReport.getErrorCount();
    auto additionalText = juce::translate("However, ");
    if (warningCount > 0) {
        additionalText += juce::String(warningCount) + " warning";
        if (warningCount > 1) {
            additionalText += "s";
        }
    }
    if (errorCount > 0) {
        if (warningCount > 0) {
            additionalText += " and ";
        }
        additionalText += juce::String(errorCount) + " error";
        if (errorCount > 1) {
            additionalText += "s";
        }
    }
    additionalText += ((warningCount + errorCount) > 1) ? juce::translate(" were found.") : juce::translate(" was found.");

    return originalText + "\r\n" + additionalText;
}

void ResultDialog::onReportButtonClicked() noexcept
{
    errorReportDialog = std::make_unique<ErrorReportDialog>(errorReport, [&] { onErrorReportOkClicked(); });
}

void ResultDialog::onErrorReportOkClicked() noexcept
{
    errorReportDialog.reset();
}


}   // namespace arkostracker

