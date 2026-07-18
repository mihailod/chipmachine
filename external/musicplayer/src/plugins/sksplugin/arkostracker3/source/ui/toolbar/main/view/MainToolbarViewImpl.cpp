#include "MainToolbarViewImpl.h"

#include <BinaryData.h>

#include "../../../../utils/NumberUtil.h"
#include "../../../components/UiUtil.h"
#include "../../../containerArranger/GlobalArrangementController.h"
#include "../../../lookAndFeel/CustomLookAndFeel.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../controller/MainToolbarController.h"

namespace arkostracker 
{

const int MainToolbarViewImpl::itemsHeight = 21;

MainToolbarViewImpl::MainToolbarViewImpl(MainToolbarController& pController, const int pSelectedLayoutNumber, const int pOctaveToShow,
    const int pSpeedToShow, const int pBpmToShow) noexcept :
        controller(pController),
        playSongFromStartButton(BinaryData::IconPlaySongFromStart_png, BinaryData::IconPlaySongFromStart_pngSize,
                   juce::translate("Play the song from start"), [&](int, bool, bool) { onPlaySongFromStartButtonClicked(); } ),
        playSongButton(BinaryData::IconPlaySong_png, BinaryData::IconPlaySong_pngSize,
                   juce::translate("Continue playing the song"), [&](int, bool, bool) { onPlaySongButtonClicked(); } ),
        playPatternFromStartButton(BinaryData::IconPlayPatternFromStart_png, BinaryData::IconPlayPatternFromStart_pngSize,
                                juce::translate("Play the pattern from start"), [&](int, bool, bool) { onPlayPatternFromStartButtonClicked(); } ),
        playPatternButton(BinaryData::IconPlayPattern_png, BinaryData::IconPlayPattern_pngSize,
                                   juce::translate("Play the pattern"), [&](int, bool, bool) { onPlayPatternButtonClicked(); } ),
        stopButton(BinaryData::IconStop_png, BinaryData::IconStop_pngSize,
                          juce::translate("Stop"), [&](int, bool, bool) { onStopButtonClicked(); } ),
        layoutButtons(),
        octaveSlider(*this, juce::translate("Octave"), 1, 1.0, 2.0, SliderIncDec::Filter::hexadecimal, 0,
                     itemsHeight),
        speedLabel(juce::String(), juce::translate("Speed")),
        speedNumberLabel(),
        serialButton(
                BinaryData::SerialIdle_png, BinaryData::SerialIdle_pngSize,
                BinaryData::SerialTransmitting_png, BinaryData::SerialTransmitting_pngSize,
                juce::translate("Start/stop communicating with hardware to transmit sound"),
                [&] (int, bool, bool) { onSerialButtonClicked(); }
        ),
        durationLabel(),
        durationLabelMouseListener(*this),
        serialBubble(),
        cachedSpeed(),
        cachedBpm()
{
    setOpaque(true);

    // Creates the Layout Buttons.
    for (auto buttonIndex = 0; buttonIndex < GlobalArrangementController::layoutCount; ++buttonIndex) {
        const auto buttonNumber = buttonIndex + 1;
        auto button = std::make_unique<LayoutButton>(*this, juce::String(buttonNumber),
                                                     juce::translate("Click to open layout " + juce::String(buttonNumber) + "."),
                                                     buttonIndex);
        button->setRadioGroupId(1239, juce::NotificationType::dontSendNotification);

        addAndMakeVisible(*button);

        layoutButtons.push_back(std::move(button));
    }
    addAndMakeVisible(octaveSlider);
    addAndMakeVisible(speedLabel);
    addAndMakeVisible(speedNumberLabel);
    addAndMakeVisible(durationLabel);
    addAndMakeVisible(serialButton);

    durationLabel.setJustificationType(juce::Justification::centredRight);
    durationLabel.addMouseListener(&durationLabelMouseListener, false);

    highlightButton(pSelectedLayoutNumber);
    octaveSlider.setShownValue(pOctaveToShow);
    showSpeed(pSpeedToShow, pBpmToShow);
    speedNumberLabel.setEnabled(false);

    // The play buttons.
    for (auto* component : { &playSongFromStartButton, &playSongButton, &playPatternFromStartButton, &playPatternButton, &stopButton }) {
        addAndMakeVisible(component);
    }
}


// Component method implementations.
// ====================================

void MainToolbarViewImpl::paint(juce::Graphics& g)
{
    // Fills the background.
    const auto color = findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBar));
    g.fillAll(color);
}

void MainToolbarViewImpl::resized()
{
    const auto totalHeight = getHeight();

    constexpr auto playButtonsWidth = 30;
    constexpr auto layoutButtonsWidth = playButtonsWidth;
    constexpr auto buttonHorizontalSpacing = 3;

    constexpr auto durationLabelWidth = 120;
    constexpr auto serialButtonWidth = 40;

    constexpr auto separator = 30;

    const auto y = (totalHeight - itemsHeight) / 2;

    constexpr auto margins = 2;

    // The Play Buttons.
    auto x = margins;
    for (auto* playButton : { &playSongFromStartButton, &playSongButton, &playPatternFromStartButton, &playPatternButton, &stopButton }) {
        playButton->setBounds(x, y, playButtonsWidth, itemsHeight);
        x += playButtonsWidth + buttonHorizontalSpacing;
    }
    x += separator;

    octaveSlider.setTopLeftPosition(x, y);
    speedLabel.setBounds(octaveSlider.getRight() + separator, y, 56, itemsHeight);
    speedNumberLabel.setBounds(speedLabel.getRight(), y, 110, itemsHeight);
    x = speedNumberLabel.getRight() + (separator / 3);

    // The Layout Buttons.
    for (const auto& layoutButton : layoutButtons) {
        layoutButton->setBounds(x, y, layoutButtonsWidth, itemsHeight);
        x += layoutButtonsWidth + buttonHorizontalSpacing;
    }
    x += separator;

    serialButton.setBounds(x, y, serialButtonWidth, itemsHeight);
    x += serialButtonWidth;

    // The timer.
    durationLabel.setBounds(x, y, durationLabelWidth + margins, itemsHeight);
}

void MainToolbarViewImpl::onLayoutButtonClicked(const int layoutIndex, bool /*isLeftButton*/) const noexcept
{
    restoreArrangement(layoutIndex);
}

void MainToolbarViewImpl::restoreArrangement(const int arrangementIndex) const noexcept
{
    highlightButton(arrangementIndex);
    controller.onUserWantsToRestoreArrangement(arrangementIndex);
}

void MainToolbarViewImpl::highlightButton(const int arrangementIndex) const noexcept
{
    layoutButtons.at(static_cast<size_t>(arrangementIndex))->setToggleState(true, juce::NotificationType::dontSendNotification);
}

void MainToolbarViewImpl::onClickOnDurationLabel() const noexcept
{
    controller.onTimerClicked();
}

void MainToolbarViewImpl::showSpeed(const int speed, const int bpm) noexcept
{
    if ((speed == cachedSpeed) && (bpm == cachedBpm)) {
       return;
    }
    cachedSpeed = speed;
    cachedBpm = bpm;

    auto text = juce::String::toHexString(speed);
    if (bpm > 0) {
        text += " (" + juce::String(bpm) +" bpm)";
    }
    speedNumberLabel.setText(text, juce::NotificationType::dontSendNotification);
}


// MainToolbarView method implementations.
// ==========================================

void MainToolbarViewImpl::setTimerToDisplay(const juce::String timer) noexcept
{
    durationLabel.setText(timer, juce::NotificationType::dontSendNotification);
}

void MainToolbarViewImpl::setCurrentArrangementNumber(const int number) noexcept
{
    highlightButton(number);
}

void MainToolbarViewImpl::setOctave(const int octave) noexcept
{
    octaveSlider.setShownValue(static_cast<double>(octave));
}

void MainToolbarViewImpl::setSpeed(const int speed, const int bpm) noexcept
{
    showSpeed(speed, bpm);
}

void MainToolbarViewImpl::setSerialIconState(const bool connected, const bool enabled) noexcept
{
    serialButton.setState(connected);
    serialButton.setEnabled(enabled);
}

void MainToolbarViewImpl::showSerialBubble(const juce::String& message) noexcept
{
    if (message.isNotEmpty()) {
        serialBubble = UiUtil::createBubble(serialButton, message, false, 7000);
    }
}

void MainToolbarViewImpl::setHighlightedPlayIcon(const PlayButton playButtonToHighlight) noexcept
{
    const decltype(playSongFromStartButton)* selectedButton;        // NOLINT(*-init-variables)
    switch (playButtonToHighlight) {
        case PlayButton::playFromStart:
            selectedButton = &playSongFromStartButton;
            break;
        case PlayButton::play:
            selectedButton = &playSongButton;
            break;
        case PlayButton::playPatternFromStart:
            selectedButton = &playPatternFromStartButton;
            break;
        case PlayButton::playPattern:
            selectedButton = &playPatternButton;
            break;
        case PlayButton::stop:
            selectedButton = &stopButton;
            break;
        default:
            selectedButton = nullptr;
            break;
    }

    for (auto* button : {
        &playSongFromStartButton, &playSongButton,
        &playPatternFromStartButton, &playPatternButton,
        &stopButton,
    } ) {
        const auto highlighted = (button == selectedButton);
        button->setToggleState(highlighted);
    }
}


// LayoutButton method implementations.
// ====================================

MainToolbarViewImpl::LayoutButton::LayoutButton(MainToolbarViewImpl& pParent, const juce::String& pButtonName, const juce::String& pToolTip, const int pLayoutIndex) :
        TextButton(pButtonName, pToolTip),
        WithParent(pParent),
        layoutIndex(pLayoutIndex)
{
    setButtonColors();
}

void MainToolbarViewImpl::LayoutButton::lookAndFeelChanged()
{
    setButtonColors();
}

void MainToolbarViewImpl::LayoutButton::setButtonColors() noexcept
{
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());

    const auto buttonTextColorSelected = currentLookAndFeel.getColor(textColourOnId, true);
    const auto buttonTextColorUnselected = currentLookAndFeel.getColor(textColourOnId, false);

    // Special colors to match the Play buttons.
    setColour(textColourOnId, buttonTextColorSelected);
    setColour(textColourOffId, buttonTextColorUnselected);
}

void MainToolbarViewImpl::LayoutButton::mouseDown(const juce::MouseEvent& event)
{
    // What Layout Button is clicked? Only left/right are authorized.
    const auto isLeftButton = event.mods.isLeftButtonDown();
    // If left and already clicked, don't do anything (no need to recreate the layout).
    if (isLeftButton && getToggleState()) {
        return;
    }

    if (isLeftButton || event.mods.isRightButtonDown()) {
        parentObject.onLayoutButtonClicked(layoutIndex, isLeftButton);
    }
}


// ====================================

void MainToolbarViewImpl::onPlaySongFromStartButtonClicked() const noexcept
{
    controller.onUserWantsToPlaySongFromStart();
}

void MainToolbarViewImpl::onPlaySongButtonClicked() const noexcept
{
    controller.onUserWantsToPlaySong();
}

void MainToolbarViewImpl::onPlayPatternFromStartButtonClicked() const noexcept
{
    controller.onUserWantsToPlayPatternFromStart();
}

void MainToolbarViewImpl::onPlayPatternButtonClicked() const noexcept
{
    controller.onUserWantsToPlayPattern();
}

void MainToolbarViewImpl::onStopButtonClicked() const noexcept
{
    controller.onUserWantsToStopPlay();
}

void MainToolbarViewImpl::onSerialButtonClicked() const noexcept
{
    controller.onUserWantsToStartStopSerial();
}

// SliderIncDec::Listener method implementations.
// =================================================

void MainToolbarViewImpl::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    if (&slider == &octaveSlider) {
        controller.onUserWantsToSetOctave(static_cast<int>(value));
    }
}

}   // namespace arkostracker
