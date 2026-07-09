#include "SubsongPropertiesDialog.h"

#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "EditPsgDialog.h"
#include "SidPlayerDialog.h"

namespace arkostracker 
{

const int SubsongPropertiesDialog::leftSlidersWidth = 90;
const int SubsongPropertiesDialog::rightSlidersWidth = 190;

const int SubsongPropertiesDialog::minimumPrimaryHighlight = 2;
const int SubsongPropertiesDialog::maximumPrimaryHighlight = 32;
const int SubsongPropertiesDialog::minimumSecondaryHighlight = 2;
const int SubsongPropertiesDialog::maximumSecondaryHighlight = 8;

SubsongPropertiesDialog::SubsongPropertiesDialog(const OptionalValue<Id>& pSubsongIdOptional, Subsong::Metadata pMetadata, std::vector<Psg> pPsgs,
    Listener& pListener, const bool pCanDelete) noexcept :
        ModalDialog(pSubsongIdOptional.isPresent() ? juce::translate("Edit subsong") : juce::translate("Add new subsong"),
                    520, 540,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        originalMetadata(std::move(pMetadata)),
        subsongPsgs(std::move(pPsgs)),
        sidPlayerCapability(originalMetadata.getSidPlayerCapability()),
        subsongIdOptional(pSubsongIdOptional),
        listener(pListener),
        deleteButton(juce::translate("Delete")),
        titleLabel(juce::String(), juce::translate("Title")),
        titleTextEditor(),
        initialSpeedSlider(*this, juce::translate("Initial speed"), 2, 1.0, 2.0, SliderIncDec::Filter::hexadecimal,
                           leftSlidersWidth),
        digiChannelSlider(*this, juce::translate("Digichannel"), 2, 1.0, 2.0, SliderIncDec::Filter::hexadecimal,
                          leftSlidersWidth),
        primaryHighlightSlider(*this, juce::translate("Rows per beat for highlight"), 2, 1.0, 2.0, SliderIncDec::Filter::hexadecimal,
                               rightSlidersWidth),
        secondaryHighlightSlider(*this, juce::translate("Secondary highlight"), 2, 1.0, 2.0,
                                 SliderIncDec::Filter::hexadecimal, rightSlidersWidth),
        replayFrequenciesGroupComponent(juce::String(), juce::translate("Replay frequency")),
        replayFrequency12p5HzToggleButton("12.5 Hz"),
        replayFrequency25HzToggleButton("25 Hz"),
        replayFrequency50HzToggleButton("50 Hz"),
        replayFrequency60HzToggleButton("60 Hz"),
        replayFrequency100HzToggleButton("100 Hz"),
        replayFrequency150HzToggleButton("150 Hz"),
        replayFrequency200HzToggleButton("200 Hz"),
        replayFrequency300HzToggleButton("300 Hz"),
        replayFrequencyCustomToggleButton(juce::translate("Custom (Hz):")),
        replayFrequencyCustomTextEditor(),
        psgsGroup(juce::String(), false, false),        // Always shows it, else it looks ugly.
        psgViews(),
        editDialog(),
        minimumDigiChannel(0),
        maximumDigiChannel(2),
        sidPlayerFrequencyGroup(juce::String(), juce::translate("SID player frequency")),
        sidPlayerFrequencyText(),
        sidPlayerFrequencyEditButton(juce::translate("Edit"))
{
    // The Delete Button is added below, under condition.
    addComponentToModalDialog(titleLabel);
    addComponentToModalDialog(titleTextEditor);
    addComponentToModalDialog(initialSpeedSlider);
    addComponentToModalDialog(digiChannelSlider);
    addComponentToModalDialog(primaryHighlightSlider);
    addComponentToModalDialog(secondaryHighlightSlider);

    addComponentToModalDialog(replayFrequenciesGroupComponent);
    addComponentToModalDialog(replayFrequency12p5HzToggleButton);
    addComponentToModalDialog(replayFrequency25HzToggleButton);
    addComponentToModalDialog(replayFrequency50HzToggleButton);
    addComponentToModalDialog(replayFrequency60HzToggleButton);
    addComponentToModalDialog(replayFrequency100HzToggleButton);
    addComponentToModalDialog(replayFrequency150HzToggleButton);
    addComponentToModalDialog(replayFrequency200HzToggleButton);
    addComponentToModalDialog(replayFrequency300HzToggleButton);
    addComponentToModalDialog(replayFrequencyCustomToggleButton);
    addComponentToModalDialog(replayFrequencyCustomTextEditor);
    addComponentToModalDialog(psgsGroup);
    addComponentToModalDialog(sidPlayerFrequencyGroup);
    addComponentToModalDialog(sidPlayerFrequencyText);
    addComponentToModalDialog(sidPlayerFrequencyEditButton);

    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto bounds = getUsableModalDialogBounds();
    const auto usableWidth = bounds.getWidth();
    const auto left = bounds.getX();
    const auto y = bounds.getY();
    const auto middleX = (usableWidth / 2) + left - 20;
    deleteButton.setBounds(left, getButtonsY(), 90, getButtonsHeight());

    titleLabel.setBounds(left, y, 70, labelsHeight);
    const auto titleTextEditorX = titleLabel.getRight();
    titleTextEditor.setBounds(titleTextEditorX, titleLabel.getY(), usableWidth - titleTextEditorX + 10, labelsHeight);

    initialSpeedSlider.setTopLeftPosition(left, titleLabel.getBottom() + margins);
    primaryHighlightSlider.setTopLeftPosition(middleX, initialSpeedSlider.getY());

    digiChannelSlider.setTopLeftPosition(left, initialSpeedSlider.getBottom() + margins);
    secondaryHighlightSlider.setTopLeftPosition(middleX, digiChannelSlider.getY());

    titleTextEditor.setMultiLine(false);
    titleTextEditor.setInputRestrictions(60);

    // Shows the Delete button if allowed.
    if (pCanDelete) {
        addComponentToModalDialog(deleteButton);
        deleteButton.onClick = [&] { onDeleteSubsongButtonClicked(); };
    }

    // Displays the replay rates.
    constexpr auto replayFrequenciesGroupHeight = 80;
    constexpr auto toggleButtonsWidth = 90;
    const auto toggleButtonsHeight = labelsHeight;
    replayFrequenciesGroupComponent.setBounds(left, secondaryHighlightSlider.getBottom() + margins, usableWidth, replayFrequenciesGroupHeight);
    // The line of replay rates.
    const auto replayFrequenciesLeft = replayFrequenciesGroupComponent.getX() + margins;
    replayFrequency12p5HzToggleButton.setBounds(replayFrequenciesLeft, replayFrequenciesGroupComponent.getY() + (2 * margins), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency25HzToggleButton.setBounds(replayFrequency12p5HzToggleButton.getRight(), replayFrequency12p5HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency50HzToggleButton.setBounds(replayFrequency25HzToggleButton.getRight(), replayFrequency12p5HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency60HzToggleButton.setBounds(replayFrequency50HzToggleButton.getRight(), replayFrequency12p5HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency100HzToggleButton.setBounds(replayFrequency60HzToggleButton.getRight(), replayFrequency12p5HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);

    replayFrequency150HzToggleButton.setBounds(replayFrequenciesLeft, replayFrequency12p5HzToggleButton.getBottom(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency200HzToggleButton.setBounds(replayFrequency150HzToggleButton.getRight(), replayFrequency150HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequency300HzToggleButton.setBounds(replayFrequency200HzToggleButton.getRight(), replayFrequency150HzToggleButton.getY(), toggleButtonsWidth, toggleButtonsHeight);
    replayFrequencyCustomToggleButton.setBounds(replayFrequency300HzToggleButton.getRight(), replayFrequency150HzToggleButton.getY(), 115, toggleButtonsHeight);
    replayFrequencyCustomTextEditor.setBounds(replayFrequencyCustomToggleButton.getRight(), replayFrequency150HzToggleButton.getY(), 70, toggleButtonsHeight);

    psgsGroup.setBounds(left, replayFrequenciesGroupComponent.getBottom() + margins, usableWidth, 200);

    // SID group.
    constexpr auto sidPlayerFrequencyButtonWidth = 80;
    sidPlayerFrequencyGroup.setBounds(left, psgsGroup.getBottom() + margins, usableWidth, 52);
    sidPlayerFrequencyText.setBounds(sidPlayerFrequencyGroup.getX() + LookAndFeelConstants::groupMarginsX, sidPlayerFrequencyGroup.getY() + LookAndFeelConstants::groupMarginsY,
        sidPlayerFrequencyGroup.getWidth() - (2 * LookAndFeelConstants::groupMarginsY) - sidPlayerFrequencyButtonWidth - margins, labelsHeight);
    sidPlayerFrequencyEditButton.setBounds(sidPlayerFrequencyGroup.getRight() - LookAndFeelConstants::groupMarginsX - sidPlayerFrequencyButtonWidth,
        sidPlayerFrequencyText.getY(), sidPlayerFrequencyButtonWidth, labelsHeight);
    sidPlayerFrequencyText.setText(sidPlayerCapability.toDisplayedString(), juce::NotificationType::dontSendNotification);
    sidPlayerFrequencyEditButton.onClick = [&] { onSidPlayerFrequencyEditButtonClicked(); };

    // Fills the values.
    const auto primaryHighlight = originalMetadata.getHighlightSpacing();
    const auto secondaryHighlight = originalMetadata.getSecondaryHighlight();
    const auto digiChannel = originalMetadata.getDigiChannel();
    const auto initialSpeed = originalMetadata.getInitialSpeed();
    const auto title = originalMetadata.getName();

    titleTextEditor.setText(title);
    initialSpeedSlider.setShownValue(initialSpeed);
    digiChannelSlider.setShownValue(digiChannel + 1);      // More user-friendly.
    primaryHighlightSlider.setShownValue(primaryHighlight);
    secondaryHighlightSlider.setShownValue(secondaryHighlight);

    // Sets up the replay frequencies.
    auto toggleIndex = 0;
    for (auto* toggle : { &replayFrequency12p5HzToggleButton, &replayFrequency25HzToggleButton, &replayFrequency50HzToggleButton, &replayFrequency60HzToggleButton,
        &replayFrequency100HzToggleButton, &replayFrequency150HzToggleButton, &replayFrequency200HzToggleButton, &replayFrequency300HzToggleButton,
        &replayFrequencyCustomToggleButton }) {
        constexpr auto radioGroupId = 1749;
        toggle->setRadioGroupId(radioGroupId, juce::NotificationType::dontSendNotification);
        ++toggleIndex;
    }
    replayFrequencyCustomToggleButton.onClick = [&] { replayFrequencyCustomTextEditor.grabKeyboardFocus(); };
    
    replayFrequencyCustomTextEditor.onFocusLost = [&] { onCustomReplayFrequencyExit(); };
    replayFrequencyCustomTextEditor.onReturnKey = [&] { onCustomReplayFrequencyExit(); };
    replayFrequencyCustomTextEditor.onTextChange = [&] { onCustomReplayFrequencyChanged(); };
    replayFrequencyCustomTextEditor.setInputRestrictions(6, juce::String("0123456789."));
    // Updates the replay rate.
    updateReplayFrequencyToggles();
    onPsgsChanged();
}


// SliderIncDec::Listener method implementations.
// =================================================
void SubsongPropertiesDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    const auto value = static_cast<int>(valueDouble);

    auto minimumValue = 0;
    auto maximumValue = 1;
    if (&slider == &primaryHighlightSlider) {
        minimumValue = minimumPrimaryHighlight;
        maximumValue = maximumPrimaryHighlight;
    } else if (&slider == &secondaryHighlightSlider) {
        minimumValue = minimumSecondaryHighlight;
        maximumValue = maximumSecondaryHighlight;
    } else if (&slider == &initialSpeedSlider) {
        minimumValue = SubsongConstants::minimumSpeed;
        maximumValue = SubsongConstants::maximumSpeed;
    } else if (&slider == &digiChannelSlider) {
        minimumValue = minimumDigiChannel + 1;
        maximumValue = maximumDigiChannel + 1;
    } else {
        jassertfalse;               // Missing slider?
    }

    slider.setShownValue(NumberUtil::correctNumber(value, minimumValue, maximumValue));
}


// =================================================

void SubsongPropertiesDialog::onOkButtonClicked() const noexcept
{
    // Extracts the metadata from the UI.
    float newReplayFrequencyHz;     // NOLINT(*-init-variables)
    if (replayFrequency12p5HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 12.5F;
    } else if (replayFrequency25HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 25.0F;
    } else if (replayFrequency50HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 50.0F;
    } else if (replayFrequency60HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 60.0F;
    } else if (replayFrequency100HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 100.0F;
    } else if (replayFrequency150HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 150.0F;
    } else if (replayFrequency200HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 200.0F;
    } else if (replayFrequency300HzToggleButton.getToggleState()) {
        newReplayFrequencyHz = 300.0F;
    } else {
        jassert(replayFrequencyCustomToggleButton.getToggleState());
        newReplayFrequencyHz = replayFrequencyCustomTextEditor.getText().getFloatValue();
    }

    const Properties newMetadata(titleTextEditor.getText(),
                                 static_cast<int>(initialSpeedSlider.getValue()),
                                 newReplayFrequencyHz,
                                 static_cast<int>(digiChannelSlider.getValue()) - 1,
                                 static_cast<int>(primaryHighlightSlider.getValue()),
                                 static_cast<int>(secondaryHighlightSlider.getValue()),
                                 sidPlayerCapability
    );

    listener.onSubsongPropertiesDialogOk(subsongPsgs, newMetadata);
}

void SubsongPropertiesDialog::onCancelButtonClicked() const noexcept
{
    listener.onSubsongPropertiesDialogCanceled();
}

void SubsongPropertiesDialog::onDeleteSubsongButtonClicked() const noexcept
{
    listener.onSubsongPropertiesDialogDeleteWanted();
}

void SubsongPropertiesDialog::onCustomReplayFrequencyExit() noexcept
{
    // Validates and corrects the typed number.
    const auto extractedText = replayFrequencyCustomTextEditor.getText();
    const auto extractedValue = extractedText.getFloatValue();
    const auto newFrequency = NumberUtil::correctNumber(extractedValue, PsgFrequency::minimumReplayFrequencyHz, PsgFrequency::maximumReplayFrequencyHz);
    const auto newText = juce::String(newFrequency);

    if (extractedText != newText) {
        replayFrequencyCustomTextEditor.setText(newText, false);
    }
}

void SubsongPropertiesDialog::onCustomReplayFrequencyChanged() noexcept
{
    // Only selects the Editor and Custom Toggle.
    replayFrequencyCustomTextEditor.grabKeyboardFocus();
    replayFrequencyCustomToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
}

void SubsongPropertiesDialog::updateReplayFrequencyToggles() noexcept
{
    if (!subsongIdOptional.isPresent()) {
        // Not present: new song. Uses 50Hz as default.
        replayFrequency50HzToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        return;
    }

    // Not a new song. Gets the replay frequency.
    const auto replayFrequencyHz = originalMetadata.getReplayFrequencyHz();

    const std::unordered_map<float, juce::ToggleButton*> frequencyToToggle = {
            { 12.5F, &replayFrequency12p5HzToggleButton },
            { 25.0F, &replayFrequency25HzToggleButton },
            { 50.0F, &replayFrequency50HzToggleButton },
            { 60.0F, &replayFrequency60HzToggleButton },
            { 100.0F, &replayFrequency100HzToggleButton },
            { 150.0F, &replayFrequency150HzToggleButton },
            { 200.0F, &replayFrequency200HzToggleButton },
            { 300.0F, &replayFrequency300HzToggleButton },
    };
    if (const auto iterator = frequencyToToggle.find(replayFrequencyHz); iterator != frequencyToToggle.cend()) {
        iterator->second->setToggleState(true, juce::NotificationType::dontSendNotification);
    } else {
        // Not found: it must be a custom value.
        replayFrequencyCustomTextEditor.setText(juce::String(replayFrequencyHz), false);
        replayFrequencyCustomToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    }
}

void SubsongPropertiesDialog::fillPsgViews() noexcept
{
    // Stores the Y position. Nice to go back there after a refresh.
    const auto scrollingY = psgsGroup.getScrollingY();

    psgViews.clear();
    psgsGroup.removeAllGroupChildren();

    auto psgIndex = 0;
    const auto psgCount = static_cast<int>(subsongPsgs.size());
    jassert(psgCount > 0);

    const auto innerBounds = psgsGroup.getGroupInnerArea();
    const auto psgViewWidth = innerBounds.getWidth() - 20 - 20;
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = margins / 2;
    auto y = margins / 2;

    for (const auto& psg : subsongPsgs) {
        constexpr auto psgViewHeight = 76;
        // Creates a View.
        auto psgView = std::make_unique<PsgView>(*this);
        const auto canDelete = ((psgCount > 1) && (psgIndex == (psgCount - 1)));        // Only the last one can be deleted, but only if there is more than one.
        psgView->setDisplayedData(psgIndex, psgCount, psg, canDelete);
        psgView->setBounds(0, y, psgViewWidth, psgViewHeight);

        psgsGroup.addComponentToGroup(*psgView);

        psgViews.push_back(std::move(psgView));
        ++psgIndex;
        y += psgViewHeight + smallMargins;
    }

    // Tries to force the Y position.
    psgsGroup.setScrollingY(scrollingY);
}


// PsgView::Listener method implementations.
// ============================================

void SubsongPropertiesDialog::onEditPsgButtonClicked(int psgIndex) noexcept
{
    jassert(editDialog == nullptr);

    const auto& psg = subsongPsgs.at(static_cast<size_t>(psgIndex));
    editDialog = std::make_unique<EditPsgDialog>(*this, psg, psgIndex);
}

void SubsongPropertiesDialog::onDeletePsgButtonClicked(const int psgIndex) noexcept
{
    if ((psgIndex < 0) && (psgIndex >= static_cast<int>(subsongPsgs.size()))) {
        jassertfalse;           // Should never happen!
        return;
    }

    // Only the last PSG can be deleted. This should have been tested before.
    if (psgIndex != (static_cast<int>(subsongPsgs.size()) - 1)) {
        jassertfalse;           // Should have been tested before!
        return;
    }

    subsongPsgs.erase(subsongPsgs.begin() + psgIndex, subsongPsgs.begin() + psgIndex + 1);
    onPsgsChanged();
}

void SubsongPropertiesDialog::onAddPsgButtonClicked(const int psgIndex) noexcept
{
    // Duplicates the PSG.
    const auto newPsg = subsongPsgs.at(static_cast<size_t>(psgIndex));

    // Inserts it after the selected one.
    subsongPsgs.insert(subsongPsgs.begin() + psgIndex, newPsg);
    onPsgsChanged();
}


// EditPsgDialog::Listener method implementations.
// ==================================================

void SubsongPropertiesDialog::onPsgValidated(const int psgIndex, const Psg newPsg) noexcept
{
    editDialog.reset();

    // Same PSG?
    const auto& existingPsg = subsongPsgs.at(static_cast<size_t>(psgIndex));
    if (newPsg == existingPsg) {
        return;         // Nothing to do.
    }

    // Replaces the PSG.
    subsongPsgs.at(static_cast<size_t>(psgIndex)) = newPsg;
    onPsgsChanged();
}

void SubsongPropertiesDialog::onPsgNotModified() noexcept
{
    editDialog.reset();
}


// ============================================

void SubsongPropertiesDialog::onPsgsChanged() noexcept
{
    fillPsgViews();

    // Updates the group title.
    const auto psgCount = static_cast<int>(subsongPsgs.size());
    const auto channelCount = PsgValues::getChannelCount(psgCount);
    psgsGroup.setGroupTitle(juce::translate("Psgs. Count: ") + juce::String(psgCount) + " (" + juce::String(channelCount) + juce::translate(" channels)"));

    // Updates the digichannel maximum and value.
    maximumDigiChannel = channelCount - 1;
    digiChannelSlider.setShownValue(NumberUtil::correctNumber(digiChannelSlider.getValue(),
                                                              static_cast<double>(minimumDigiChannel) + 1.0, static_cast<double>(maximumDigiChannel) + 1.0));
}

void SubsongPropertiesDialog::onSidPlayerFrequencyEditButtonClicked() noexcept
{
    jassert(editDialog == nullptr);

    editDialog = std::make_unique<SidPlayerDialog>(*this, sidPlayerCapability);
}


// SidPlayerDialog::Listener method implementations.
// ==================================================

void SubsongPropertiesDialog::onSidPlayerSelected(const SidPlayerCapability pSidPlayerFrequency) noexcept
{
    sidPlayerCapability = pSidPlayerFrequency;

    editDialog.reset();

    sidPlayerFrequencyText.setText(sidPlayerCapability.toDisplayedString(), juce::NotificationType::dontSendNotification);
}

void SubsongPropertiesDialog::onSidPlayerCanceled() noexcept
{
    editDialog.reset();
}

}   // namespace arkostracker
