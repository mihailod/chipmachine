#include "Header.h"

#include <BinaryData.h>

#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../controller/HeaderController.h"

namespace arkostracker 
{

Header::Header(HeaderController& pHeaderController) noexcept :
        headerController(pHeaderController),

        loopStartSlider(*this, juce::translate("Loop start")),
        endSlider(*this, juce::translate("End")),
        speedSlider(*this, juce::translate("Speed")),
        shiftSlider(*this, juce::translate("Shift")),
        xZoomButton(BinaryData::IconZoomX_png, BinaryData::IconZoomX_pngSize, juce::translate("Zoom X"),
            [&](int, bool, bool) { onZoomButtonClicked(); }),
        shrinkRowsButton(BinaryData::IconShrink_png, BinaryData::IconShrink_pngSize, juce::translate(
            "Shrink all rows (right-click to try shrinking to a minimum height)"),
            [&](int, bool, const bool isLeftClicked) { onShrinkRowsButtonClicked(isLeftClicked); }),
        loopButton(BinaryData::IconLoopOff_png, BinaryData::IconLoopOff_pngSize,
                   BinaryData::IconLoopOn_png, BinaryData::IconLoopOn_pngSize,
                   juce::translate("Toggle the loop"),
                   [&] (int, bool, bool) { onLoopButtonClicked(); }),
        retrigButton(BinaryData::IconRetrigOff_png, BinaryData::IconRetrigOff_pngSize,
                     BinaryData::IconRetrigOn_png, BinaryData::IconRetrigOn_pngSize,
                     juce::translate("Toggle the global retrig of hardware envelope when the sound starts"),
                     [&] (int, bool, bool) { onRetrigButtonClicked(); }),
        separator(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundFocused))

{
    separator.setTransformationOnColour([] (const juce::Colour color){
        return color.contrasting(0.2F);
    });
}

void Header::addViewToParent(juce::Component& parent) noexcept
{
    parent.addAndMakeVisible(loopStartSlider);
    parent.addAndMakeVisible(endSlider);
    parent.addAndMakeVisible(speedSlider);
    if (headerController.showShift()) {
        parent.addAndMakeVisible(shiftSlider);
    }
    parent.addAndMakeVisible(xZoomButton);
    if (headerController.showShrinkRows()) {
        parent.addAndMakeVisible(shrinkRowsButton);
    }

    if (headerController.showIsLooping()) {
        parent.addAndMakeVisible(loopButton);
    }
    if (headerController.showIsRetrig()) {
        parent.addAndMakeVisible(retrigButton);
    }

    parent.addAndMakeVisible(separator);
}

void Header::setBounds(int x, const int y, const int width) noexcept
{
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::smallMargins;
    const auto availableWidth = width - 2 * margins;
    constexpr auto topSeparatorHeight = 1;
    const auto buttonsY = y;
    const auto buttonsWithIconsWidth = LookAndFeelConstants::iconButtonsWidth;
    const auto buttonsWithIconsHeight = LookAndFeelConstants::buttonsHeight;

    // Some components may not be visible.
    loopStartSlider.setTopLeftPosition(x, y);
    endSlider.setTopLeftPosition(loopStartSlider.getRight() + margins, y);

    x = endSlider.getRight() + margins * 2;

    if (headerController.showIsLooping()) {
        loopButton.setBounds(x, buttonsY, buttonsWithIconsWidth, buttonsWithIconsHeight);
        x += buttonsWithIconsWidth + margins;
    }

    if (headerController.showIsRetrig()) {
        retrigButton.setBounds(x, buttonsY, buttonsWithIconsWidth, buttonsWithIconsHeight);
        x += buttonsWithIconsWidth + margins;
    }

    speedSlider.setTopLeftPosition(x, y);
    x += speedSlider.getWidth() + margins * 2;

    if (headerController.showShift()) {
        shiftSlider.setTopLeftPosition(x, y);
        x += shiftSlider.getWidth() + margins * 2;
    }
    x += margins;

    if (headerController.showShrinkRows()) {
        shrinkRowsButton.setBounds(x, y, buttonsWithIconsWidth, buttonsWithIconsHeight);
        x += shrinkRowsButton.getWidth() + smallMargins;
    }
    xZoomButton.setBounds(x, y, buttonsWithIconsWidth, buttonsWithIconsHeight);

    separator.setBounds(margins, loopStartSlider.getBottom() + (margins / 2), availableWidth, topSeparatorHeight);
}

int Header::getHeight() const noexcept
{
    return separator.getBottom() - loopStartSlider.getY();
}

void Header::setDisplayedData(const int loopStart, const int end, const bool isLooping, const bool isRetrig, const int speed, const int shift)
{
    loopStartSlider.setShownValue(loopStart);
    endSlider.setShownValue(end);
    loopButton.setState(isLooping);
    retrigButton.setState(isRetrig);
    speedSlider.setShownValue(speed);
    shiftSlider.setShownValue(shift);
}


// SliderIncDec::Listener method implementations.
// =================================================

void Header::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    const auto valueInt = static_cast<int>(value);

    if (&slider == &loopStartSlider) {
        headerController.onUserWantsToSetStartInHeader(valueInt);
    } else if (&slider == &endSlider) {
        headerController.onUserWantsToSetLoopEndInHeader(valueInt);
    } else if (&slider == &speedSlider) {
        headerController.onUserWantsToSetSpeedInHeader(valueInt);
    } else if (&slider == &shiftSlider) {
        headerController.onUserWantsToSetShiftInHeader(valueInt);
    } else {
        jassertfalse; // Slider not managed?
    }
}


// ====================================================

void Header::onZoomButtonClicked() const noexcept
{
    headerController.onUserWantsToSwitchZoomXInHeader();
}

void Header::onLoopButtonClicked() const noexcept
{
    headerController.onUserWantsToToggleLoopInHeader({ });
}

void Header::onRetrigButtonClicked() const noexcept
{
    headerController.onUserWantsToToggleRetrigInHeader();
}

void Header::onShrinkRowsButtonClicked(const bool isLeftClicked) const noexcept
{
    headerController.onUserWantsToShrinkRows(!isLeftClicked);
}

}   // namespace arkostracker
