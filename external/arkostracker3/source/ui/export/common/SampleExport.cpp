#include "SampleExport.h"

#include "../../../utils/NumberUtil.h"
#include "../../../utils/StringUtil.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

SampleExport::SampleExport(const bool pShowExportSamplesOption, const bool pShowIgnoreUnusedAndGenerateIndexOptions) noexcept :
        showExportSamplesOption(pShowExportSamplesOption),
        showIgnoreUnusedAndGenerateIndexOptions(pShowIgnoreUnusedAndGenerateIndexOptions),
        group(juce::translate("Sample export")),
        exportSampleToggleButton(juce::translate("Export used samples")),
        toggleIgnoreUnusedSamples(juce::translate("Ignore unused samples")),
        toggleGenerateIndexTable(juce::translate("Generate index table")),

        amplitudeTextEditor(),
        amplitudeLabel(juce::String(), juce::translate("Amplitude (8 bits)")),
        fourBitsTextButton(juce::translate("4 bits")),
        sevenBitsTextButton(juce::translate("7 bits")),
        eightBitsTextButton(juce::translate("8 bits")),

        offsetTextEditor(),
        offsetLabel(juce::String(), juce::translate("Offset (8 bits)")),
        unsignedTextButton(juce::translate("Unsigned")),
        signedTextButton(juce::translate("Signed")),

        paddingLengthTextEditor(),
        paddingLengthLabel(juce::String(), juce::translate("Padding length")),
        fadeOutLengthTextEditor(),
        fadeOutLengthLabel(juce::String(), juce::translate("Fade out length")),

        forcePaddingAndFadeOutToLowestValueToggleButton(juce::translate("Padding/fade to min value")),
        onlyExportUsedLengthToggleButton(juce::translate("Exp. only used length"))
{
    addAndMakeVisible(group);
}

void SampleExport::resized()
{
    // The group takes all the space.
    group.setBounds(getLocalBounds());

    constexpr auto lineHeight = 20;
    constexpr auto separatorY = 8;
    const auto smallMargins = LookAndFeelConstants::smallMargins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto toggleHeight = LookAndFeelConstants::labelsHeight + 4;

    auto area = group.getGroupInnerArea();

    // The "export samples" at the top may not be shown.
    area.removeFromTop(smallMargins);
    if (showExportSamplesOption) {
        exportSampleToggleButton.setBounds(area.removeFromTop(labelsHeight));
        exportSampleToggleButton.addListener(this);
        area.removeFromTop(separatorY);
    } else if (showIgnoreUnusedAndGenerateIndexOptions) {
        toggleIgnoreUnusedSamples.setBounds(area.removeFromTop(toggleHeight));
        toggleGenerateIndexTable.setBounds(area.removeFromTop(toggleHeight));
        toggleIgnoreUnusedSamples.addListener(this);
        toggleGenerateIndexTable.addListener(this);
        area.removeFromTop(separatorY);
    }

    const auto middleX = (area.getWidth() / 2) + area.getX();
    const auto right = group.getGroupInnerArea().getWidth() - 20;       // Huh.
    constexpr auto buttonSeparator = 2;

    // The Amplitude and bit rates Buttons.
    constexpr auto bitRateLabelsWidth = 55;
    constexpr auto leftTextsWidth = 130;
    amplitudeLabel.setBounds(0, area.getY(), leftTextsWidth, lineHeight);
    eightBitsTextButton.setBounds(right - bitRateLabelsWidth, amplitudeLabel.getY(), bitRateLabelsWidth, lineHeight);
    sevenBitsTextButton.setBounds(eightBitsTextButton.getX() - bitRateLabelsWidth - buttonSeparator, eightBitsTextButton.getY(), bitRateLabelsWidth, lineHeight);
    fourBitsTextButton.setBounds(sevenBitsTextButton.getX() - bitRateLabelsWidth - buttonSeparator, eightBitsTextButton.getY(), bitRateLabelsWidth, lineHeight);

    amplitudeTextEditor.setBounds(amplitudeLabel.getRight(), amplitudeLabel.getY(), 40, lineHeight);
    amplitudeTextEditor.setLinkedComponent(&amplitudeLabel);

    amplitudeTextEditor.setInputRestrictions(3, StringUtil::integerNumbers);
    fourBitsTextButton.addListener(this);
    sevenBitsTextButton.addListener(this);
    eightBitsTextButton.addListener(this);

    // The offset and sign/unsigned RadioButtons.
    constexpr auto signedButtonWidth = 85;
    offsetLabel.setBounds(0, amplitudeLabel.getBottom() + separatorY, leftTextsWidth, lineHeight);
    offsetTextEditor.setBounds(offsetLabel.getRight(), offsetLabel.getY(), 35, lineHeight);
    offsetTextEditor.setInputRestrictions(3, StringUtil::integerNumbers);
    signedTextButton.setBounds(right - signedButtonWidth + buttonSeparator, offsetTextEditor.getY(), signedButtonWidth, lineHeight);
    unsignedTextButton.setBounds(signedTextButton.getX() - signedButtonWidth - buttonSeparator, signedTextButton.getY(), signedButtonWidth, lineHeight);
    unsignedTextButton.addListener(this);
    signedTextButton.addListener(this);
    offsetTextEditor.setLinkedComponent(&offsetLabel);

    // The padding length and fade out length.
    paddingLengthLabel.setBounds(0, offsetLabel.getBottom() + separatorY, leftTextsWidth, lineHeight);
    paddingLengthTextEditor.setBounds(paddingLengthLabel.getRight(), paddingLengthLabel.getY(), 50, lineHeight);
    paddingLengthTextEditor.setInputRestrictions(5, StringUtil::integerNumbers);
    paddingLengthTextEditor.setLinkedComponent(&paddingLengthLabel);
    constexpr auto fadeOutLengthTextLabelWidth = 132;
    constexpr auto fadeOutLengthTextEditorWidth = 40;
    fadeOutLengthTextEditor.setBounds(right - fadeOutLengthTextEditorWidth, paddingLengthLabel.getY(), fadeOutLengthTextEditorWidth, lineHeight);
    fadeOutLengthLabel.setBounds(fadeOutLengthTextEditor.getX() - fadeOutLengthTextLabelWidth, fadeOutLengthTextEditor.getY(), fadeOutLengthTextLabelWidth, lineHeight);
    fadeOutLengthTextEditor.setInputRestrictions(3, StringUtil::integerNumbers);
    fadeOutLengthTextEditor.setLinkedComponent(&fadeOutLengthLabel);

    // Force passing to the lowest value / export only used length.
    forcePaddingAndFadeOutToLowestValueToggleButton.setBounds(0, paddingLengthLabel.getBottom() + separatorY, middleX, lineHeight);
    constexpr auto onlyExportUsedLengthToggleButtonWidth = 170;
    onlyExportUsedLengthToggleButton.setBounds(right - onlyExportUsedLengthToggleButtonWidth,
                                               forcePaddingAndFadeOutToLowestValueToggleButton.getY(), onlyExportUsedLengthToggleButtonWidth, lineHeight);

    group.addComponentToGroup(exportSampleToggleButton);
    group.addComponentToGroup(toggleIgnoreUnusedSamples);
    group.addComponentToGroup(toggleGenerateIndexTable);
    group.addComponentToGroup(amplitudeLabel);
    group.addComponentToGroup(amplitudeTextEditor);
    group.addComponentToGroup(fourBitsTextButton);
    group.addComponentToGroup(sevenBitsTextButton);
    group.addComponentToGroup(eightBitsTextButton);
    group.addComponentToGroup(offsetLabel);
    group.addComponentToGroup(offsetTextEditor);
    group.addComponentToGroup(unsignedTextButton);
    group.addComponentToGroup(signedTextButton);
    group.addComponentToGroup(paddingLengthLabel);
    group.addComponentToGroup(paddingLengthTextEditor);
    group.addComponentToGroup(fadeOutLengthLabel);
    group.addComponentToGroup(fadeOutLengthTextEditor);
    group.addComponentToGroup(forcePaddingAndFadeOutToLowestValueToggleButton);
    group.addComponentToGroup(onlyExportUsedLengthToggleButton);

    // By default, no samples.
    exportSampleToggleButton.setToggleState(false, juce::NotificationType::sendNotification);
}


// Button::Listener method implementations.
// ============================================

void SampleExport::buttonClicked(juce::Button* button)
{
    if (button == &fourBitsTextButton) {
        amplitudeTextEditor.setText("16");
    } else if (button == &sevenBitsTextButton) {
        amplitudeTextEditor.setText("128");
    } else if (button == &eightBitsTextButton) {
        amplitudeTextEditor.setText("256");
    } else if (button == &unsignedTextButton) {
        offsetTextEditor.setText("0");
    } else if (button == &signedTextButton) {
        offsetTextEditor.setText("128");
    }

    updateButtonsEnability();
}


// ============================================

void SampleExport::updateButtonsEnability() noexcept
{
    // Enables/disables the other buttons from this Button.
    const auto optionsEnabled = exportSampleToggleButton.getToggleState();

    amplitudeTextEditor.setEnabled(optionsEnabled);
    unsignedTextButton.setEnabled(optionsEnabled);
    signedTextButton.setEnabled(optionsEnabled);
    fourBitsTextButton.setEnabled(optionsEnabled);
    sevenBitsTextButton.setEnabled(optionsEnabled);
    eightBitsTextButton.setEnabled(optionsEnabled);
    offsetTextEditor.setEnabled(optionsEnabled);
    paddingLengthTextEditor.setEnabled(optionsEnabled);
    fadeOutLengthTextEditor.setEnabled(optionsEnabled);
    forcePaddingAndFadeOutToLowestValueToggleButton.setEnabled(optionsEnabled);
    onlyExportUsedLengthToggleButton.setEnabled(optionsEnabled);
}

void SampleExport::setValuesAndRefreshUi(const SampleEncoderFlags& sampleEncoderFlags) noexcept
{
    // To know whether the samples are exported, use the flag state. However, if the "export sample" label is not shown, it means
    // we always export the samples.
    const auto areSamplesExported = (!showExportSamplesOption || sampleEncoderFlags.areSamplesExported());

    exportSampleToggleButton.setToggleState(areSamplesExported, juce::NotificationType::sendNotification);
    toggleIgnoreUnusedSamples.setToggleState(sampleEncoderFlags.isIgnoreUnusedSamples(), juce::NotificationType::sendNotification);
    toggleGenerateIndexTable.setToggleState(sampleEncoderFlags.isGenerateIndexTable(), juce::NotificationType::sendNotification);
    amplitudeTextEditor.setText(juce::String(sampleEncoderFlags.getAmplitude()), false);
    offsetTextEditor.setText(juce::String(sampleEncoderFlags.getOffset()), false);
    paddingLengthTextEditor.setText(juce::String(sampleEncoderFlags.getPaddingLength()), false);
    fadeOutLengthTextEditor.setText(juce::String(sampleEncoderFlags.getFadeOutSampleCountForNonLoopingSamples()), false);
    forcePaddingAndFadeOutToLowestValueToggleButton.setToggleState(sampleEncoderFlags.isForcePaddingAndFadeOutToLowestValue(), juce::NotificationType::dontSendNotification);
    onlyExportUsedLengthToggleButton.setToggleState(sampleEncoderFlags.isOnlyExportUsedLength(), juce::NotificationType::dontSendNotification);

    // Enables/disables the buttons.
    updateButtonsEnability();
}

SampleEncoderFlags SampleExport::getSampleEncoderFlags() const noexcept
{
    // Corrects the values, if needed.
    auto amplitude = amplitudeTextEditor.getText().getIntValue();
    amplitude = NumberUtil::correctNumber(amplitude, 2, 256);

    return SampleEncoderFlags(
            exportSampleToggleButton.getToggleState(),
            amplitude,
            offsetTextEditor.getText().getIntValue(),
            paddingLengthTextEditor.getText().getIntValue(),
            fadeOutLengthTextEditor.getText().getIntValue(),
            forcePaddingAndFadeOutToLowestValueToggleButton.getToggleState(),
            onlyExportUsedLengthToggleButton.getToggleState(),
            toggleIgnoreUnusedSamples.getToggleState(),
            toggleGenerateIndexTable.getToggleState()
    );
}

}   // namespace arkostracker
