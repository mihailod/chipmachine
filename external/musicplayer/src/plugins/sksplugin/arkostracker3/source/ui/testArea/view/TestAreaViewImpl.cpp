#include "TestAreaViewImpl.h"

#include <BinaryData.h>

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../lookAndFeel/CustomLookAndFeel.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../controller/TestAreaController.h"

namespace arkostracker 
{

const int TestAreaViewImpl::activityIndicatorFadeOutSpeedMs = 300;
const float TestAreaViewImpl::activityIndicatorFadeInAlpha = 0.7F;

TestAreaViewImpl::TestAreaViewImpl(TestAreaController& pController, const DisplayedData& pDisplayedData) noexcept :
        controller(pController),
        useSelectedArpButton(BinaryData::IconArpeggioOff_png, BinaryData::IconArpeggioOff_pngSize,
                             BinaryData::IconArpeggioOn_png, BinaryData::IconArpeggioOn_pngSize,
                             juce::translate("On to use the selected arpeggio, off to use none"),
                             [&](int, bool, bool) { onArpeggioButtonClicked(); }),
        useSelectedPitchButton(BinaryData::IconPitchOff_png, BinaryData::IconPitchOff_pngSize,
                               BinaryData::IconPitchOn_png, BinaryData::IconPitchOn_pngSize,
                             juce::translate("On to use the selected pitch, off to use none"),
                             [&](int, bool, bool) { onPitchButtonClicked(); }),
        usedArpeggioIndexLabel(),
        usedPitchIndexLabel(),
        testButton(juce::String(), juce::translate("Press to play the instrument")),
        monophonicButton(juce::String(), juce::translate("Press to toggle between monophonic and polyphonic")),
        editButton(BinaryData::IconToolboxOn_png, BinaryData::IconToolboxOn_pngSize,
                   juce::translate("Click to edit the mono/polyphonic settings"),
                   [&](int, bool, bool) { onEditButtonClicked(); }),
        keyboardView(CellConstants::maximumNote + 1, [&](const int noteNumber) { onKeyboardKeyPressed(noteNumber); }),
        activityIndicatorImage(),
        animator(juce::Desktop::getInstance().getAnimator())
{
    activityIndicatorImage.setImage(juce::ImageFileFormat::loadFrom(BinaryData::ActivityIndicator_png, BinaryData::ActivityIndicator_pngSize),
        juce::RectanglePlacement::centred);
    activityIndicatorImage.setAlpha(0.0);       // Invisible at first.

    updateUi(pDisplayedData);

    testButton.onClick = [&] { onTestButtonClicked(); };
    monophonicButton.onClick = [&] { onMonoButtonClicked(); };

    // Sets-up the expression labels.
    constexpr auto expressionLabelFontSizeDelta = -4.0F;
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
    const auto& baseFont = currentLookAndFeel.getBaseFont();
    const auto baseFontHeight = baseFont.getHeight();

    const auto expressionLabelFont = baseFont.withHeight(baseFontHeight + expressionLabelFontSizeDelta);
    for (auto* label : { &usedArpeggioIndexLabel, &usedPitchIndexLabel }) {
        label->setJustificationType(juce::Justification::centredTop);
        label->setFont(expressionLabelFont);

        label->getProperties().set(LookAndFeelConstants::labelPropertyUseCustomFont, true);
    }

    addAndMakeVisible(useSelectedArpButton);
    addAndMakeVisible(useSelectedPitchButton);
    addAndMakeVisible(usedArpeggioIndexLabel);
    addAndMakeVisible(usedPitchIndexLabel);
    addAndMakeVisible(testButton);
    addAndMakeVisible(monophonicButton);
    addAndMakeVisible(editButton);
    addAndMakeVisible(keyboardView);
    addAndMakeVisible(&activityIndicatorImage);
}


// Component method implementations.
// ====================================

void TestAreaViewImpl::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto largerMargins = margins * 2;
    const auto smallMargin = margins / 2;
    const auto itemsHeight = LookAndFeelConstants::buttonsHeight;

    constexpr auto buttonsWidth = 30;

    constexpr auto keyboardYMargins = 5;
    constexpr auto keyboardY = keyboardYMargins;
    constexpr auto keyboardMinHeight = 25;
    constexpr auto keyboardMaxHeight = 60;
    const auto keyboardHeight = std::min(std::max(keyboardMinHeight, height - keyboardY - keyboardYMargins), keyboardMaxHeight);

    // Looks better if these components are in the Y middle of the keyboard.
    const auto itemsY = keyboardY + (keyboardHeight - itemsHeight) / 2;
    useSelectedArpButton.setBounds(margins, itemsY, buttonsWidth, itemsHeight);
    useSelectedPitchButton.setBounds(useSelectedArpButton.getRight() + smallMargin, itemsY, buttonsWidth, itemsHeight);
    usedArpeggioIndexLabel.setBounds(useSelectedArpButton.getX(), useSelectedArpButton.getBottom(), useSelectedArpButton.getWidth(), labelsHeight);
    usedPitchIndexLabel.setBounds(useSelectedPitchButton.getX(), useSelectedPitchButton.getBottom(), useSelectedPitchButton.getWidth(), labelsHeight);
    testButton.setBounds(useSelectedPitchButton.getRight() + largerMargins, itemsY, 60, itemsHeight);
    const auto activityIndicatorImageHeight = activityIndicatorImage.getImage().getHeight();
    activityIndicatorImage.setBounds(testButton.getRight() + smallMargin, testButton.getY() + (itemsHeight - activityIndicatorImageHeight) / 2,
        activityIndicatorImage.getImage().getWidth(), activityIndicatorImageHeight);
    monophonicButton.setBounds(activityIndicatorImage.getRight() + smallMargin, itemsY, 60, itemsHeight);
    editButton.setBounds(monophonicButton.getRight() + 2, itemsY, buttonsWidth, itemsHeight);

    const auto keyboardX = editButton.getRight() + largerMargins;
    const auto keyboardWidth = width - keyboardX - margins;
    keyboardView.setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);
}


// TestAreaView method implementations.
// =======================================

void TestAreaViewImpl::setDisplayedData(const DisplayedData& displayedData) noexcept
{
    updateUi(displayedData);
}


// =======================================

void TestAreaViewImpl::updateUi(const DisplayedData& displayedData) noexcept
{
    useSelectedArpButton.setState(displayedData.useSelectedArp);
    useSelectedPitchButton.setState(displayedData.useSelectedPitch);
    setExpressionIndex(true, displayedData.usedArpeggioIndex, displayedData.useSelectedArp);
    setExpressionIndex(false, displayedData.usedPitchIndex, displayedData.useSelectedPitch);

    const auto noteOptional = displayedData.noteToShow;
    testButton.setButtonText(noteOptional.isAbsent() ? juce::translate("Play!") : NoteUtil::getStringFromNote(noteOptional.getValue()));

    monophonicButton.setButtonText(displayedData.useMono ? juce::translate("Mono") : juce::translate("Poly"));

    // Shows a possible pressed note.
    if (displayedData.pendingNoteToShow.isPresent()) {
        keyboardView.showKeyAsPressed(displayedData.pendingNoteToShow.getValue());
        // Triggers the small activity indicator.
        activityIndicatorImage.setAlpha(activityIndicatorFadeInAlpha);
        animator.animateComponent(&activityIndicatorImage, activityIndicatorImage.getBounds(), 0.0F,
            activityIndicatorFadeOutSpeedMs, false, 0, 0);
    }
}

void TestAreaViewImpl::onArpeggioButtonClicked() const noexcept
{
    controller.onUserWantsToToggleUseSelectedArp();
}

void TestAreaViewImpl::onPitchButtonClicked() const noexcept
{
    controller.onUserWantsToToggleUseSelectedPitch();
}

void TestAreaViewImpl::onTestButtonClicked() const noexcept
{
    controller.onUserWantsToPlaySound();
}

void TestAreaViewImpl::onMonoButtonClicked() const noexcept
{
    controller.onUserWantsToToggleMonophony();
}

void TestAreaViewImpl::onEditButtonClicked() const noexcept
{
    controller.onUserWantsToEditChannels();
}

void TestAreaViewImpl::onKeyboardKeyPressed(const int noteNumber) const noexcept
{
    controller.onUserWantsToPlayNote(noteNumber);
}

void TestAreaViewImpl::setExpressionIndex(const bool isArpeggio, const int index, const bool isExpressionUsed) noexcept
{
    auto& label = isArpeggio ? usedArpeggioIndexLabel : usedPitchIndexLabel;
    const auto index0 = (index <= 0);
    label.setText(index0 ? "--" : NumberUtil::toHexByte(index), juce::NotificationType::dontSendNotification);
    label.setAlpha((index0 || !isExpressionUsed) ? 0.4F : 1.0F);
}

}   // namespace arkostracker
