#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../export/samples/SampleEncoderFlags.h"
#include "../../components/EditText.h"
#include "../../components/GroupWithViewport.h"

namespace arkostracker 
{

/** A generic Component for the user to select how the samples must be exported. */
class SampleExport final : public juce::Component,
                           juce::Button::Listener
{
public:
    static constexpr auto componentDesiredHeight = 131 + 38;                      			// The height this Component should be.
    static constexpr auto componentDesiredHeightIfExportLabelRemoved = componentDesiredHeight - 28;  // The height this Component should be, if the "export samples" is not displayed.
    /** With "ignore unused samples" and "generate index table" presents. */
    static constexpr auto componentDesiredHeightIgnoreUnusedAndGenerateIndexPresent = componentDesiredHeight + 38;

    /**
     * Constructor.
     * @param showExportSamplesOption true to show the "export samples if used" at the top. If false, all the states are enabled.
     * @param showIgnoreUnusedAndGenerateIndexOptions true to show "ignore unused samples" and "generate index table".
     */
    explicit SampleExport(bool showExportSamplesOption = true, bool showIgnoreUnusedAndGenerateIndexOptions = false) noexcept;

    // Component method implementations.
    // =================================
    void resized() override;

    // Button::Listener method implementations.
    // ============================================
    void buttonClicked(juce::Button* button) override;

    // ============================================

    /**
     * Sets the values of the UI, refreshing it.
     * @param sampleEncoderFlags the flags object from which to export the values.
     */
    void setValuesAndRefreshUi(const SampleEncoderFlags& sampleEncoderFlags) noexcept;

    /** Returns the Sample Export flags, from the UI. */
    SampleEncoderFlags getSampleEncoderFlags() const noexcept;

private:
    /** Enables or disables the Buttons according to the state of the "export sample" Button. */
    void updateButtonsEnability() noexcept;

    bool showExportSamplesOption;							// True to show the "export samples if used" option.
    bool showIgnoreUnusedAndGenerateIndexOptions;

    GroupWithViewport group;						        // Group, with all the other items inside.
    juce::ToggleButton exportSampleToggleButton;                  // Toggle Button to know if the samples are exported or not.

    juce::ToggleButton toggleIgnoreUnusedSamples;
    juce::ToggleButton toggleGenerateIndexTable;

    EditText amplitudeTextEditor;                                 // TextEditor showing the amplitude.
    juce::Label amplitudeLabel;                                   // Label showing the amplitude.
    juce::TextButton fourBitsTextButton;                          // 4-bit Text Button.
    juce::TextButton sevenBitsTextButton;                         // 7-bit Text Button.
    juce::TextButton eightBitsTextButton;                         // 8-bit Text Button.

    EditText offsetTextEditor;                                    // TextEditor showing the offset.
    juce::Label offsetLabel;                                      // Label showing the offset.
    juce::TextButton unsignedTextButton;                          // Unsigned Text Button.
    juce::TextButton signedTextButton;                            // Signed Text Button.

    EditText paddingLengthTextEditor;                              // TextEditor showing the padding length.
    juce::Label paddingLengthLabel;                                // Label showing the padding length.
    EditText fadeOutLengthTextEditor;                              // TextEditor showing the fade-out length.
    juce::Label fadeOutLengthLabel;                                // Label showing the fade-out length.

    juce::ToggleButton forcePaddingAndFadeOutToLowestValueToggleButton;       // Toggle Button to know if the padding/fade out goes to the lowest value.
    juce::ToggleButton onlyExportUsedLengthToggleButton;  		 // Toggle Button to know if only the used length are exported.
};

}   // namespace arkostracker
