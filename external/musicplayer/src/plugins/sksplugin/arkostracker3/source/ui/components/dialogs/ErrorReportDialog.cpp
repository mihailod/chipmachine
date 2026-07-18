#include "ErrorReportDialog.h"

#include "../../../utils/ErrorReport.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ErrorReportDialog::ErrorReportDialog(const ErrorReport& pErrorReport, std::function<void()> pCallback) noexcept :
        DialogWindow(juce::translate("Report"),
                     juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground)),
                     true, true),
        callback(std::move(pCallback)),
        mainComponent(),
        topReportLabel(),
        okButton(juce::translate("OK")),
        copyButton(juce::translate("Copy")),
        reportView()
{
    constexpr int width = 480;
    constexpr int height = 350;
    mainComponent.setSize(width, height);

    setResizable(false, false);
    centreAroundComponent(nullptr, width, height);

    setContentNonOwned(&mainComponent, true);       // The Content is owned by this class.

    enterModalState(true, nullptr, false);

    reportView.setMultiLine(true, true);
    reportView.setReadOnly(true);
    reportView.setCaretVisible(false);

    okButton.addListener(this);
    copyButton.addListener(this);

    constexpr int middleWidth = width / 2;
    constexpr int buttonsHeight = 25;
    constexpr int okButtonWidth = 60;
    constexpr int copyButtonWidth = 70;
    constexpr int margins = 10;

    okButton.setBounds(middleWidth - okButtonWidth / 2, height - buttonsHeight - margins, okButtonWidth, buttonsHeight);
    copyButton.setBounds(margins, okButton.getY(), copyButtonWidth, buttonsHeight);

    topReportLabel.setBounds(margins, margins, width - 2 * margins, 60);

    reportView.setBounds(margins, topReportLabel.getBottom() + margins, width - 2 * margins, okButton.getY() - topReportLabel.getBottom() - margins * 2);

    // Builds the text.
    buildReportText(pErrorReport);

    mainComponent.addAndMakeVisible(&okButton);
    mainComponent.addAndMakeVisible(&copyButton);
    mainComponent.addAndMakeVisible(&topReportLabel);
    mainComponent.addAndMakeVisible(&reportView);
}


void ErrorReportDialog::buildReportText(const ErrorReport& errorReport) noexcept
{
    constexpr auto limit = 100;

    const auto originalCount = errorReport.getLineCount();
    const auto count = std::min(originalCount, limit);

    // Updates the top Label.
    {
        auto topText = juce::String(errorReport.getErrorCount()) + juce::translate(" errors and ");
        topText += juce::String(errorReport.getWarningCount()) + juce::translate(" warnings have been found while performing the action.");
        topReportLabel.setText(topText, juce::NotificationType::dontSendNotification);
    }

    // Adds each line, with a limit.
    juce::String text;
    for (auto lineIndex = 0; lineIndex < count; ++lineIndex) {
        const auto& line = errorReport.getLine(lineIndex);
        const auto lineNumberOptional = line.getLineNumber();
        const auto lineText = lineNumberOptional.isAbsent()
                                  ? juce::String()
                                  : // Encodes the line, if given.
                                  (" (" + juce::String(lineNumberOptional.getValue()) + ")");

        const auto level = line.getLevel();
        text += (level == ReportLevel::error) ? "E" : "W";
        text += lineText;
        text += ": ";
        text += line.getText();
        text += "\n";
    }
    // If there was more than what is shown, adds a little message.
    if (originalCount > limit) {
        text += "... and " + juce::String(originalCount - limit) + " more.";
    }

    reportView.setText(text, juce::NotificationType::dontSendNotification);
}

void ErrorReportDialog::closeButtonPressed()
{
    if (callback != nullptr) {
        callback();
    }
    exitModalState(0);
}


// Button::Listener method implementations.
// ------------------------------------------------------

void ErrorReportDialog::buttonClicked(juce::Button* button)
{
    if (button == &copyButton) {
        juce::SystemClipboard::copyTextToClipboard(reportView.getText());
    } else if (button == &okButton) {
        if (callback != nullptr) {
            callback();
        }
    } else {
        jassert(false);     // Shouldn't happen.
    }
}

}   // namespace arkostracker
