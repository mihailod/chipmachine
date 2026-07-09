#include "EditPsgDialog.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../business/period/GeneratePeriods.h"
#include "../../utils/NumberUtil.h"
#include "../components/UiUtil.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "../utils/TextEditorUtil.h"

namespace arkostracker 
{

const int EditPsgDialog::offsetMixingOutputId = 1;

EditPsgDialog::EditPsgDialog(EditPsgDialog::Listener& pListener, const Psg& pPsg, const int pPsgIndex) noexcept :
        ModalDialog(juce::translate("Edit PSG"), 480, 370,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                          true, true),
        listener(pListener),
        psgIndex(pPsgIndex),
        psgTypeGroup(juce::String(), juce::translate("Type")),
        ayToggle(juce::translate("AY (CPC, MSX, Spectrum...)")),
        ymToggle(juce::translate("YM (Atari ST)")),
        psgFrequencyChooser(false, juce::translate("Frequency")),

        referenceFrequencyGroup(juce::String(), juce::translate("Reference frequency")),
        referenceFrequency440Toggle(juce::translate("440 Hz")),
        referenceFrequencyCustomToggle(juce::translate("Custom (in Hz):")),
        referenceFrequencyCustomTextEditor(),

        samplePlayerFrequencyGroup(juce::String(), juce::translate("Sample player freq.")),
        samplePlayerFrequencyEditor(),

        outputMixGroup(juce::String(), juce::translate("Output mix")),
        outputMixComboBox(),

        restrictionInt(TextEditorUtil::buildRestrictionForInt(7)),
        restrictionFloat(TextEditorUtil::buildRestrictionForPositiveFloat(7)),

        copyFrequenciesButton(juce::translate("Copy freqs."), juce::translate("Get the frequencies in the clipboard.")),
        messageBubble()
{
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = margins / 2;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto groupMarginsX = LookAndFeelConstants::groupMarginsX;
    const auto groupMarginsY = LookAndFeelConstants::groupMarginsY;

    const auto innerBounds = getUsableModalDialogBounds();
    const auto left = innerBounds.getX();
    const auto top = innerBounds.getY();
    const auto width = innerBounds.getWidth();
    const auto firstColumnWidth = (width / 2) - (margins / 2) + 60;
    const auto secondColumnX = left + firstColumnWidth + (margins / 2);
    const auto secondColumnWidth = width - secondColumnX;
    const auto secondColumnContentWidth = secondColumnWidth - (groupMarginsX * 2);
    constexpr auto offsetXCustomEditor = 30;

    // Sets up the PSG type group.
    constexpr auto typeRadioId = 451;
    psgTypeGroup.setBounds(left, top, width, 51);
    ayToggle.setBounds(psgTypeGroup.getX() + groupMarginsX, psgTypeGroup.getY() + groupMarginsY, 240, labelsHeight);
    ymToggle.setBounds(ayToggle.getRight(), ayToggle.getY(), 150, labelsHeight);
    ayToggle.setRadioGroupId(typeRadioId);
    ymToggle.setRadioGroupId(typeRadioId);

    // Sets up the PSG Frequency group.
    psgFrequencyChooser.setBounds(left, psgTypeGroup.getBottom() + margins, firstColumnWidth, PsgFrequencyChooser::desiredHeightWithoutDontChangeToggle);

    // Sets up the Reference Frequency group.
    constexpr auto referenceFrequencyRadioId = 954;
    referenceFrequencyGroup.setBounds(secondColumnX, psgFrequencyChooser.getY(), secondColumnWidth, 100);
    referenceFrequency440Toggle.setBounds(referenceFrequencyGroup.getX() + groupMarginsX, referenceFrequencyGroup.getY() + groupMarginsY, secondColumnContentWidth,
                                          labelsHeight);
    referenceFrequencyCustomToggle.setBounds(referenceFrequency440Toggle.getX(), referenceFrequency440Toggle.getBottom(), secondColumnContentWidth, labelsHeight);
    referenceFrequencyCustomTextEditor.setBounds(referenceFrequencyCustomToggle.getX() + offsetXCustomEditor, referenceFrequencyCustomToggle.getBottom(),
                                                 100, labelsHeight);
    referenceFrequency440Toggle.setRadioGroupId(referenceFrequencyRadioId);
    referenceFrequencyCustomToggle.setRadioGroupId(referenceFrequencyRadioId);

    // Sets up the sample frequency group.
    samplePlayerFrequencyGroup.setBounds(secondColumnX, referenceFrequencyGroup.getBottom() + smallMargins, secondColumnWidth, 54);
    samplePlayerFrequencyEditor.setBounds(samplePlayerFrequencyGroup.getX() + groupMarginsX, samplePlayerFrequencyGroup.getY() + groupMarginsY,
                                          secondColumnContentWidth, labelsHeight);

    // Sets up the Mixing Output group.
    outputMixGroup.setBounds(secondColumnX, samplePlayerFrequencyGroup.getBottom() + smallMargins, secondColumnWidth, 54);
    outputMixComboBox.setBounds(outputMixGroup.getX() + groupMarginsX, outputMixGroup.getY() + groupMarginsY + 4,
                                100, labelsHeight);

    copyFrequenciesButton.setBounds(left, getButtonsY(), 110, getButtonsHeight());

    // Fills the Views.
    fillViews(pPsg);

    // Listeners.
    referenceFrequencyCustomToggle.onClick = [&] { referenceFrequencyCustomTextEditor.grabKeyboardFocus(); };
    referenceFrequencyCustomTextEditor.onTextChange = [&] { selectReferenceFrequencyCustomToggle(); };
    referenceFrequencyCustomTextEditor.onFocusLost = [&] { checkAndModifyReferencePsgFrequencyValueAndSelectIfNecessary(); };
    referenceFrequencyCustomTextEditor.onReturnKey = [&] { checkAndModifyReferencePsgFrequencyValueAndSelectIfNecessary(); };

    samplePlayerFrequencyEditor.onFocusLost = [&] { checkAndModifySamplePlayerFrequencyValueAndSelectIfNecessary(); };
    samplePlayerFrequencyEditor.onReturnKey = [&] { checkAndModifySamplePlayerFrequencyValueAndSelectIfNecessary(); };

    copyFrequenciesButton.onClick = [&] { onCopyFrequenciesButtonClicked(); };

    // Restricts the Text Editors.
    referenceFrequencyCustomTextEditor.setInputFilter(&restrictionFloat, false);
    samplePlayerFrequencyEditor.setInputFilter(&restrictionInt, false);

    addComponentToModalDialog(psgTypeGroup);
    addComponentToModalDialog(ayToggle);
    addComponentToModalDialog(ymToggle);
    addComponentToModalDialog(psgFrequencyChooser);
    addComponentToModalDialog(referenceFrequencyGroup);
    addComponentToModalDialog(referenceFrequency440Toggle);
    addComponentToModalDialog(referenceFrequencyCustomToggle);
    addComponentToModalDialog(referenceFrequencyCustomTextEditor);
    addComponentToModalDialog(samplePlayerFrequencyGroup);
    addComponentToModalDialog(samplePlayerFrequencyEditor);
    addComponentToModalDialog(outputMixGroup);
    addComponentToModalDialog(outputMixComboBox);
    addComponentToModalDialog(copyFrequenciesButton);
}

void EditPsgDialog::onOkButtonClicked() const noexcept
{
    // Creates the PSG object from the UI.
    const auto psg = buildPsgFromUi();
    listener.onPsgValidated(psgIndex, psg);
}

Psg EditPsgDialog::buildPsgFromUi() const noexcept
{
    const auto psgType = ayToggle.getToggleState() ? PsgType::ay : PsgType::ym;
    const auto referenceFrequency = referenceFrequency440Toggle.getToggleState() ? 440.0F : referenceFrequencyCustomTextEditor.getText().getFloatValue();
    const auto samplePlayerFrequency = samplePlayerFrequencyEditor.getText().getIntValue();
    const auto psgMixingOutput = static_cast<PsgMixingOutput>(outputMixComboBox.getSelectedId() - offsetMixingOutputId);

    const auto psgFrequencyOptional = psgFrequencyChooser.getSelectedFrequencyHz();
    auto psgFrequency = PsgFrequency::psgFrequencyCPC;      // Fallback, shouldn't be used.
    if (psgFrequencyOptional.isPresent()) {
        psgFrequency = psgFrequencyOptional.getValue();
    } else {
        jassertfalse;       // Shouldn't happen.
    }

    return Psg(psgType, psgFrequency, referenceFrequency, samplePlayerFrequency, psgMixingOutput);
}

void EditPsgDialog::onCancelButtonClicked() const noexcept
{
    listener.onPsgNotModified();
}

void EditPsgDialog::fillViews(const Psg& psg) noexcept
{
    // PSG type.
    const auto psgType = psg.getType();
    jassert((psgType == PsgType::ay) || (psgType == PsgType::ym));
    if (psgType == PsgType::ay) {
        ayToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else {
        ymToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    }

    // PSG frequency.
    const auto psgFrequency = psg.getPsgFrequency();
    psgFrequencyChooser.setFrequencyHz(psgFrequency);

    // Reference frequency.
    if (const auto referenceFrequency = psg.getReferenceFrequency(); juce::exactlyEqual(referenceFrequency, 440.0F)) {
        referenceFrequency440Toggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    } else {
        referenceFrequencyCustomToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
        referenceFrequencyCustomTextEditor.setText(juce::String(referenceFrequency));
    }

    // Sample player frequency.
    samplePlayerFrequencyEditor.setText(juce::String(psg.getSamplePlayerFrequency()));

    // Mixing output.
    for (const auto mixingOutput : { PsgMixingOutput::ABC, PsgMixingOutput::ACB, PsgMixingOutput::BAC, PsgMixingOutput::BCA,
                                     PsgMixingOutput::CAB, PsgMixingOutput::CBA,
                                     PsgMixingOutput::threeChannelsToLeft, PsgMixingOutput::threeChannelsToMiddle, PsgMixingOutput::threeChannelsToRight}) {
        const auto id = static_cast<int>(mixingOutput) + offsetMixingOutputId;
        outputMixComboBox.addItem(PsgMixingOutputUtil::psgMixingOutputToDisplayedText(mixingOutput), id);
    }
    const auto selectedMixingOutput = psg.getPsgMixingOutput();
    outputMixComboBox.setSelectedId(static_cast<int>(selectedMixingOutput) + offsetMixingOutputId, juce::NotificationType::dontSendNotification);
}

void EditPsgDialog::checkAndModifyReferencePsgFrequencyValueAndSelectIfNecessary() noexcept
{
    auto referenceFrequency = referenceFrequencyCustomTextEditor.getText().getFloatValue();
    referenceFrequency = NumberUtil::correctNumber(referenceFrequency, PsgFrequency::minimumReferenceFrequency, PsgFrequency::maximumReferenceFrequency);

    referenceFrequencyCustomTextEditor.setText(juce::String(referenceFrequency), false);
}

void EditPsgDialog::selectReferenceFrequencyCustomToggle() noexcept
{
    referenceFrequencyCustomToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
}

void EditPsgDialog::checkAndModifySamplePlayerFrequencyValueAndSelectIfNecessary() noexcept
{
    auto samplePlayerFrequency = samplePlayerFrequencyEditor.getText().getIntValue();
    samplePlayerFrequency = NumberUtil::correctNumber(samplePlayerFrequency, PsgFrequency::minimumSamplePlayerFrequency, PsgFrequency::maximumSamplePlayerFrequency);

    samplePlayerFrequencyEditor.setText(juce::String(samplePlayerFrequency), false);
}

void EditPsgDialog::onCopyFrequenciesButtonClicked() noexcept
{
    const auto sourceProfile = PreferencesManager::getInstance().getCurrentSourceProfile();
    const auto psg = buildPsgFromUi();

    // Generates the source.
    const auto outputStream = GeneratePeriods::generate(psg.getPsgFrequency(), psg.getReferenceFrequency(), sourceProfile);

    // Saves to clipboard.
    juce::SystemClipboard::copyTextToClipboard(outputStream->getMemoryBlock().toString());

    messageBubble = UiUtil::createBubble(copyFrequenciesButton, juce::translate("The frequencies have been copied to the clipboard, as source."));
}

}   // namespace arkostracker
