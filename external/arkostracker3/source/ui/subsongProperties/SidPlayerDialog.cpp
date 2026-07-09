#include "SidPlayerDialog.h"

#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "../utils/TextEditorUtil.h"

namespace arkostracker
{

SidPlayerDialog::SidPlayerDialog(Listener& pListener, const SidPlayerCapability& currentSidPlayerFrequency) noexcept :
        ModalDialog(juce::translate("Edit SID player frequency"), 370, 310,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                          true, true),
        listener(pListener),
        restrictionIntFrequency(TextEditorUtil::buildRestrictionForInt(5)),
        restrictionIntPeriods(TextEditorUtil::buildRestrictionForInt(4)),
        topLabel(juce::String(), juce::translate("Select your SID player capabilities:")),
        cpcToggle(juce::translate("Amstrad CPC (15.6 kHz), 8-bit period (2-255)")),
        amstradPlusToggle(juce::translate("Amstrad Plus (15.6 kHz), 12-bit period (2-4095)")),
        atariStToggle(juce::translate("Atari ST (MPF-bound), 12-bit period (1-4095)")),
        customToggle(juce::translate("Custom")),
        customFrequencyLabel(juce::String(), juce::translate("Player frequency (Hz)")),
        customFrequencyTextEditor(),
        customMinimumPeriodLabel(juce::String(), juce::translate("Minimum period (decimal)")),
        customMinimumPeriodTextEditor(),
        customMaximumPeriodLabel(juce::String(), juce::translate("Maximum period (decimal)")),
        customMaximumPeriodTextEditor()
{
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = margins / 2;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    const auto innerBounds = getUsableModalDialogBounds();
    const auto left = innerBounds.getX();
    const auto top = innerBounds.getY();
    const auto width = innerBounds.getWidth();

    topLabel.setBounds(left, top, width, labelsHeight);
    cpcToggle.setBounds(left, topLabel.getBottom() + margins, width, labelsHeight);
    amstradPlusToggle.setBounds(left, cpcToggle.getBottom(), width, labelsHeight);
    atariStToggle.setBounds(left, amstradPlusToggle.getBottom(), width, labelsHeight);
    customToggle.setBounds(left, atariStToggle.getBottom(), width, labelsHeight);
    const auto leftCustoms = left + (margins * 3);
    constexpr auto customLabelWidth = 180;
    constexpr auto customFieldsWidth = 80;
    customFrequencyLabel.setBounds(leftCustoms, customToggle.getBottom(), customLabelWidth, labelsHeight);
    customFrequencyTextEditor.setBounds(customFrequencyLabel.getRight(), customFrequencyLabel.getY(), customFieldsWidth, labelsHeight);
    customMinimumPeriodLabel.setBounds(leftCustoms, customFrequencyLabel.getBottom() + smallMargins, customLabelWidth, labelsHeight);
    customMinimumPeriodTextEditor.setBounds(customMinimumPeriodLabel.getRight(), customMinimumPeriodLabel.getY(), customFieldsWidth, labelsHeight);
    customMaximumPeriodLabel.setBounds(leftCustoms, customMinimumPeriodLabel.getBottom() + smallMargins, customLabelWidth, labelsHeight);
    customMaximumPeriodTextEditor.setBounds(customMaximumPeriodLabel.getRight(), customMaximumPeriodLabel.getY(), customFieldsWidth, labelsHeight);

    addComponentToModalDialog(topLabel);
    for (auto* toggle : { &cpcToggle, &amstradPlusToggle, &atariStToggle, &customToggle }) {
        addComponentToModalDialog(*toggle);
        toggle->setRadioGroupId(451);
    }
    addComponentToModalDialog(customFrequencyLabel);
    addComponentToModalDialog(customFrequencyTextEditor);
    addComponentToModalDialog(customMinimumPeriodLabel);
    addComponentToModalDialog(customMinimumPeriodTextEditor);
    addComponentToModalDialog(customMaximumPeriodLabel);
    addComponentToModalDialog(customMaximumPeriodTextEditor);

    customToggle.onStateChange = [&] { onCustomToggleStateChanged(); };

    customFrequencyTextEditor.setInputFilter(&restrictionIntFrequency, false);
    customMinimumPeriodTextEditor.setInputFilter(&restrictionIntPeriods, false);
    customMaximumPeriodTextEditor.setInputFilter(&restrictionIntPeriods, false);

    // Fills the custom fields with a default (using the given one would not work with Atari ST!), or Custom if present.
    const auto customSidPlayerCapabilityToUse = (currentSidPlayerFrequency.getSidPlayerProfile() == SidPlayerProfile::custom)
            ? currentSidPlayerFrequency : SidPlayerCapability::buildForCpc();
    customFrequencyTextEditor.setText(juce::String(customSidPlayerCapabilityToUse.getFrequencyHz()), false);
    customMinimumPeriodTextEditor.setText(juce::String(customSidPlayerCapabilityToUse.getMinimumPeriod()), false);
    customMaximumPeriodTextEditor.setText(juce::String(customSidPlayerCapabilityToUse.getMaximumPeriod()), false);

    customFrequencyTextEditor.onFocusLost = [&] { onTextFieldChanged(); };
    customMinimumPeriodTextEditor.onFocusLost = [&] { onTextFieldChanged(); };
    customMaximumPeriodTextEditor.onFocusLost = [&] { onTextFieldChanged(); };

    customFrequencyTextEditor.onReturnKey = [&] { onOkButtonClicked(); };
    customMinimumPeriodTextEditor.onReturnKey = [&] { onOkButtonClicked(); };
    customMaximumPeriodTextEditor.onReturnKey = [&] { onOkButtonClicked(); };

    // Selects one.
    juce::ToggleButton* selectedToggle = nullptr;
    switch (currentSidPlayerFrequency.getSidPlayerProfile()) {
        case SidPlayerProfile::cpc:
            selectedToggle = &cpcToggle;
            break;
        case SidPlayerProfile::amstradPlus:
            selectedToggle = &amstradPlusToggle;
            break;
        case SidPlayerProfile::atariSt:
            selectedToggle = &atariStToggle;
            break;
        case SidPlayerProfile::custom:
            selectedToggle = &customToggle;
            break;
        default:
            jassertfalse;
    }

    if (selectedToggle != nullptr) {
        selectedToggle->setToggleState(true, juce::NotificationType::sendNotification);
    }

    onCustomToggleStateChanged();       // Hack, else the component would be refreshed at first.
}

void SidPlayerDialog::onOkButtonClicked() noexcept
{
    correctFields();

    auto capability = SidPlayerCapability::buildForCpc();
    if (cpcToggle.getToggleState()) {
        // Nothing to do.
    } else if (amstradPlusToggle.getToggleState()) {
        capability = SidPlayerCapability::buildForAmstradPlus();
    } else if (atariStToggle.getToggleState()) {
        capability = SidPlayerCapability::buildForAtariSt();
    } else if (customToggle.getToggleState()) {
        const auto frequency = customFrequencyTextEditor.getText().getIntValue();
        const auto minimumPeriod = customMinimumPeriodTextEditor.getText().getIntValue();
        const auto maximumPeriod = customMaximumPeriodTextEditor.getText().getIntValue();
        capability = SidPlayerCapability::buildForCustom(frequency, minimumPeriod, maximumPeriod);
    } else {
        jassertfalse;   // Toggle not handled!
    }

    listener.onSidPlayerSelected(capability);
}

void SidPlayerDialog::onCancelButtonClicked() const noexcept
{
    listener.onSidPlayerCanceled();
}

void SidPlayerDialog::onCustomToggleStateChanged() noexcept
{
    const auto enabled = customToggle.getToggleState();

    for (Component* view : { &customFrequencyLabel, &customFrequencyLabel,  &customMinimumPeriodLabel, &customMaximumPeriodLabel }) {
        view->setEnabled(enabled);
    }
    for (Component* view : { &customFrequencyTextEditor, &customMinimumPeriodTextEditor,&customMaximumPeriodTextEditor }) {
        view->setEnabled(enabled);
    }
}

void SidPlayerDialog::onTextFieldChanged() noexcept
{
    correctFields();
}

void SidPlayerDialog::correctFields() noexcept
{
    const auto frequency = NumberUtil::correctNumber(customFrequencyTextEditor.getText().getIntValue(), PsgValues::sidPlayerMinimumFrequencyHz,
        PsgValues::sidPlayerMaximumFrequencyHz);
    constexpr auto minimumAllowedPeriod = PsgValues::sidMinimumPeriod;
    constexpr auto maximumAllowedPeriod = PsgValues::sidMaximumPeriod;
    const auto minimumPeriod = NumberUtil::correctNumber(customMinimumPeriodTextEditor.getText().getIntValue(), minimumAllowedPeriod, maximumAllowedPeriod);
    const auto maximumPeriod = NumberUtil::correctNumber(customMaximumPeriodTextEditor.getText().getIntValue(), minimumPeriod, maximumAllowedPeriod);

    customFrequencyTextEditor.setText(juce::String(frequency), false);
    customMinimumPeriodTextEditor.setText(juce::String(minimumPeriod), false);
    customMaximumPeriodTextEditor.setText(juce::String(maximumPeriod), false);
}

}   // namespace arkostracker
