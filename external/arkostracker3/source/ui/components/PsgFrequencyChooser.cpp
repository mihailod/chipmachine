#include "PsgFrequencyChooser.h"

#include "../../song/psg/PsgFrequency.h"
#include "../../utils/NumberUtil.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "../utils/TextEditorUtil.h"

namespace arkostracker
{

const int PsgFrequencyChooser::desiredHeightWithoutDontChangeToggle = 230;
const int PsgFrequencyChooser::desiredHeightWithDontChangeToggle = 260;

PsgFrequencyChooser::PsgFrequencyChooser(const bool pShowDontChangeToggle, const juce::String& groupTitle) noexcept :
        showDontChangeToggle(pShowDontChangeToggle),
        restrictionInt(TextEditorUtil::buildRestrictionForInt(7)),
        psgFrequencyGroup(juce::String(), groupTitle),
        psgFrequencyDontChangeToggle(juce::translate("Keep YM original frequency")),
        psgFrequencyCpcToggle(juce::translate("1000000 Hz (Amstrad CPC / PCW)")),
        psgFrequencySpectrumToggle(juce::translate("1773400 Hz (ZX Spectrum)")),
        psgFrequencyPentagonToggle(juce::translate("1750000 Hz (Pentagon 128k)")),
        psgFrequencyMsxToggle(juce::translate("1789773 Hz (MSX / SVI)")),
        psgFrequencyAtariToggle(juce::translate("2000000 Hz (Atari ST)")),
        psgFrequencyCustomToggle(juce::translate("Custom (in Hz):")),
        psgFrequencyCustomTextEditor()
{
    addAndMakeVisible(psgFrequencyGroup);
    if (showDontChangeToggle) {
        addAndMakeVisible(psgFrequencyDontChangeToggle);
    }
    addAndMakeVisible(psgFrequencyCpcToggle);
    addAndMakeVisible(psgFrequencySpectrumToggle);
    addAndMakeVisible(psgFrequencyPentagonToggle);
    addAndMakeVisible(psgFrequencyMsxToggle);
    addAndMakeVisible(psgFrequencyAtariToggle);
    addAndMakeVisible(psgFrequencyCustomToggle);
    addAndMakeVisible(psgFrequencyCustomTextEditor);

    psgFrequencyCustomTextEditor.setInputFilter(&restrictionInt, false);
}


// Component method implementations.
// ==================================================

void PsgFrequencyChooser::resized()
{
    // Sets up the PSG Frequency group.
    const auto groupMarginsX = LookAndFeelConstants::groupMarginsX;
    const auto groupMarginsY = LookAndFeelConstants::groupMarginsY;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto interline = 5;
    constexpr auto left = 0;
    constexpr auto top = 0;
    const auto width = getWidth();
    const auto height = getHeight();
    const auto firstColumnContentWidth = width - (groupMarginsX * 2);
    constexpr auto offsetXCustomEditor = 30;

    psgFrequencyGroup.setBounds(left, top, width, height);
    auto y = psgFrequencyGroup.getY() + groupMarginsY;
    if (showDontChangeToggle) {
        psgFrequencyDontChangeToggle.setBounds(psgFrequencyGroup.getX() + groupMarginsX, y, firstColumnContentWidth, labelsHeight);
        y += labelsHeight + interline;
    }
    psgFrequencyCpcToggle.setBounds(psgFrequencyGroup.getX() + groupMarginsX, y, firstColumnContentWidth, labelsHeight);
    psgFrequencyPentagonToggle.setBounds(psgFrequencyCpcToggle.getX(), psgFrequencyCpcToggle.getBottom() + interline, firstColumnContentWidth, labelsHeight);
    psgFrequencySpectrumToggle.setBounds(psgFrequencyPentagonToggle.getX(), psgFrequencyPentagonToggle.getBottom() + interline, firstColumnContentWidth, labelsHeight);
    psgFrequencyMsxToggle.setBounds(psgFrequencySpectrumToggle.getX(), psgFrequencySpectrumToggle.getBottom() + interline, firstColumnContentWidth, labelsHeight);
    psgFrequencyAtariToggle.setBounds(psgFrequencyMsxToggle.getX(), psgFrequencyMsxToggle.getBottom() + interline, firstColumnContentWidth, labelsHeight);
    psgFrequencyCustomToggle.setBounds(psgFrequencyAtariToggle.getX(), psgFrequencyAtariToggle.getBottom() + interline, firstColumnContentWidth, labelsHeight);
    psgFrequencyCustomTextEditor.setBounds(psgFrequencyCustomToggle.getX() + offsetXCustomEditor, psgFrequencyCustomToggle.getBottom(), 100, labelsHeight);
    for (auto* toggle : { &psgFrequencyDontChangeToggle, &psgFrequencyCpcToggle, &psgFrequencySpectrumToggle, &psgFrequencyPentagonToggle, &psgFrequencyMsxToggle,
        &psgFrequencyAtariToggle, &psgFrequencyCustomToggle}) {
        constexpr auto psgFrequencyRadioId = 1239;
        toggle->setRadioGroupId(psgFrequencyRadioId);
    }

    // Listeners.
    psgFrequencyCustomToggle.onClick = [&] { psgFrequencyCustomTextEditor.grabKeyboardFocus(); };
    psgFrequencyCustomTextEditor.onTextChange = [&] {
        psgFrequencyCustomToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    };
    psgFrequencyCustomTextEditor.onFocusLost = [&] { checkAndModifyPsgFrequencyValueAndSelectIfNecessary(); };
    psgFrequencyCustomTextEditor.onReturnKey = [&] { checkAndModifyPsgFrequencyValueAndSelectIfNecessary(); };
}

// ==================================================

void PsgFrequencyChooser::setFrequencyHz(const OptionalInt psgFrequencyHz) noexcept
{
    if (psgFrequencyHz.isAbsent() && showDontChangeToggle) {
        psgFrequencyDontChangeToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (psgFrequencyHz == PsgFrequency::psgFrequencyCPC) {
        psgFrequencyCpcToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (psgFrequencyHz == PsgFrequency::psgFrequencySpectrum) {
        psgFrequencySpectrumToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (psgFrequencyHz == PsgFrequency::psgFrequencyPentagon128K) {
        psgFrequencyPentagonToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (psgFrequencyHz == PsgFrequency::psgFrequencyMsxAndSvi) {
        psgFrequencyMsxToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else if (psgFrequencyHz == PsgFrequency::psgFrequencyAtariST) {
        psgFrequencyAtariToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else {
        psgFrequencyCustomToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
        const auto frequencyHz = psgFrequencyHz.getValue();
        psgFrequencyCustomTextEditor.setText(juce::String(frequencyHz));
    }
}

OptionalInt PsgFrequencyChooser::getSelectedFrequencyHz() const noexcept
{
    auto psgFrequency = psgFrequencyCustomTextEditor.getText().getIntValue();         // Default.
    if (psgFrequencyDontChangeToggle.getToggleState() && showDontChangeToggle) {
        psgFrequency = 0;
    } else if (psgFrequencyCpcToggle.getToggleState()) {
        psgFrequency = PsgFrequency::psgFrequencyCPC;
    } else if (psgFrequencySpectrumToggle.getToggleState()) {
        psgFrequency = PsgFrequency::psgFrequencySpectrum;
    } else if (psgFrequencyPentagonToggle.getToggleState()) {
        psgFrequency = PsgFrequency::psgFrequencyPentagon128K;
    } else if (psgFrequencyMsxToggle.getToggleState()) {
        psgFrequency = PsgFrequency::psgFrequencyMsxAndSvi;
    } else if (psgFrequencyAtariToggle.getToggleState()) {
        psgFrequency = PsgFrequency::psgFrequencyAtariST;
    } else {
        jassert(psgFrequencyCustomToggle.getToggleState());         // Problem!!!
    }

    return (psgFrequency <= 0) ? OptionalInt() : psgFrequency;
}

void PsgFrequencyChooser::checkAndModifyPsgFrequencyValueAndSelectIfNecessary() noexcept
{
    auto psgFrequency = psgFrequencyCustomTextEditor.getText().getIntValue();
    psgFrequency = NumberUtil::correctNumber(psgFrequency, PsgFrequency::minimumPsgFrequency, PsgFrequency::maximumPsgFrequency);

    psgFrequencyCustomTextEditor.setText(juce::String(psgFrequency), false);
}

}   // namespace arkostracker
