#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../EditText.h"

namespace arkostracker 
{

class ErrorReport;

/** Dialog showing the given Error Report. */
class ErrorReportDialog final :  public juce::DialogWindow,
                                 juce::Button::Listener
{
public:
    /**
     * Constructor.
     * @param errorReport the StatusReport to display.
     * @param callback called when OK is clicked.
    */
    ErrorReportDialog(const ErrorReport& errorReport, std::function<void()> callback) noexcept;

private:
    /**
     * Fills the Report label with the data from the Status Report.
     * @param errorReport the Status Report.
     */
    void buildReportText(const ErrorReport& errorReport) noexcept;

    // Button::Listener method implementations.
    // ==============================================================================
    void buttonClicked(juce::Button* button) override;

    // DialogWindow methods override.
    // ==============================================================================
    void closeButtonPressed() override;

    std::function<void()> callback;                         // The callback to be notified of the result.
    Component mainComponent;                                // Holds all the views.
    juce::Label topReportLabel;                             // Label at the top.
    juce::TextButton okButton;                              // The "ok" Button at the bottom.
    juce::TextButton copyButton;                            // The "copy" Button at the bottom.
    EditText reportView;                                    // Where the report is shown.
};

}   // namespace arkostracker
