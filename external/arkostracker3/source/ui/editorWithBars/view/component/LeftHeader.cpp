#include "LeftHeader.h"

#include <BinaryData.h>

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

LeftHeader::LeftHeader(Listener& pListener, const AreaType pNewAreaType, const bool pCanHideRow, const juce::String& pNewAreaName) noexcept : listener(pListener),
    areaType(pNewAreaType),
    canHideRow(pCanHideRow),
    areaNameLabel(juce::String(), pNewAreaName),
    hideButton(),
    plusButton(BinaryData::IconPlus_png, BinaryData::IconPlus_pngSize,
               juce::translate("Increase height"),
               [&](int, bool, bool) { listener.onLeftHeaderPlusClicked(areaType); }),
    minusButton(BinaryData::IconMinus_png, BinaryData::IconMinus_pngSize,
                juce::translate("Decrease height"),
                [&](int, bool, bool) { listener.onLeftHeaderMinusClicked(areaType); }),
    autoSpreadButton(BinaryData::IconLoopOff_png, BinaryData::IconLoopOff_pngSize,
                     BinaryData::IconLoopOn_png, BinaryData::IconLoopOn_pngSize,
                     juce::translate("Toggle the auto-spread"),
                     [&] (int, bool, bool) { onAutoSpreadButtonClicked(); }),
    displayData(false, false, OptionalBool(), false),
    labelMouseListener(*this)
{
    setHiddenButton(false);
    hideButton.setTooltip(juce::translate("Hide/show"));
    hideButton.onClick = [&] {
        onUserWantsToHide();
    };

    addAndMakeVisible(areaNameLabel);
    addChildComponent(hideButton);
    addAndMakeVisible(plusButton);
    addAndMakeVisible(minusButton);
    addAndMakeVisible(autoSpreadButton);

    // By declaring them as listeners, the enter/exit interactions work.
    areaNameLabel.addMouseListener(&labelMouseListener, true);
    plusButton.addMouseListener(this, true);
    minusButton.addMouseListener(this, true);
    hideButton.addMouseListener(this, true);
    autoSpreadButton.addMouseListener(this, true);

    // Makes sure only the bars can get focus, else the Insert/... keys requires a click on the bar to work.
    plusButton.setMouseClickGrabsKeyboardFocus(false);
    minusButton.setMouseClickGrabsKeyboardFocus(false);
    hideButton.setMouseClickGrabsKeyboardFocus(false);
    autoSpreadButton.setMouseClickGrabsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);
}

void LeftHeader::setDisplayedData(const LeftHeaderDisplayData& newDisplayData, const bool forceRefresh) noexcept
{
    // Any change needed?
    if (!forceRefresh && (displayData == newDisplayData)) {
        return;
    }
    displayData = newDisplayData;

    // Refreshes the UI.
    const auto isHidden = newDisplayData.isHidden();
    plusButton.setEnabled(displayData.isPlusEnabled());
    minusButton.setEnabled(displayData.isMinusEnabled());
    plusButton.setVisible(!isHidden);
    minusButton.setVisible(!isHidden);

    const auto spreadState = displayData.getAutoSpreadOn();
    const auto spreadPresent = spreadState.isPresent();
    autoSpreadButton.setVisible(!isHidden && spreadPresent);
    autoSpreadButton.setState(!isHidden && spreadPresent && spreadState.getValue());

    areaNameLabel.setAlpha(isHidden ? 0.4F : 1.0F);

    setHiddenButton(isHidden);
}

void LeftHeader::onAutoSpreadButtonClicked() const noexcept
{
    listener.onLeftHeaderAutoSpreadClicked(areaType);
}

void LeftHeader::setHiddenButton(const bool hidden) noexcept
{
    const auto isVisible = canHideRow;
    hideButton.setVisible(false);       // On init, always hidden. Only mouse over will show it if possible.
    if (!isVisible) {
        return;
    }

    constexpr auto hideButtonNormalOpacity = 1.0F;      // Strangely enough, JUCE seems to ignore this.
    const auto overlayColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);

    hideButton.setImages(false, false, true,
    hidden ?
        juce::ImageFileFormat::loadFrom(BinaryData::ArrowUp_png, BinaryData::ArrowUp_pngSize) :
        juce::ImageFileFormat::loadFrom(BinaryData::ArrowDown_png, BinaryData::ArrowDown_pngSize),
        hideButtonNormalOpacity, overlayColor,
        juce::Image(), hideButtonNormalOpacity, overlayColor,
        juce::Image(), hideButtonNormalOpacity, overlayColor);
}


// Component method implementations.
// =====================================

void LeftHeader::resized()
{
    const auto width = getWidth();

    const auto iconsSize = LookAndFeelConstants::labelsHeight;
    constexpr auto iconLeftMargin = 4;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    const auto hideButtonX = width - hideButtonWidth;
    hideButton.setBounds(hideButtonX, 0, hideButtonWidth, labelsHeight);
    areaNameLabel.setTopLeftPosition(0, 0);
    setAreaNameSize();

    const auto bigButtonsY = areaNameLabel.getBottom();
    plusButton.setBounds(iconLeftMargin, bigButtonsY, iconsSize, iconsSize);
    minusButton.setBounds(plusButton.getRight() - 2, bigButtonsY, iconsSize, iconsSize);

    autoSpreadButton.setBounds(width - iconsSize, bigButtonsY, iconsSize, iconsSize);
}

void LeftHeader::setAreaNameSize() noexcept
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto width = getWidth();

    // If the hide button is present, shrinks the label.
    const auto labelWidth = hideButton.isVisible() ?
        hideButton.getX() - areaNameLabel.getX():
        width;
    areaNameLabel.setSize(labelWidth, labelsHeight);
}

void LeftHeader::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (canHideRow) {
        hideButton.setVisible(true);
    }
    setAreaNameSize();
}

void LeftHeader::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (canHideRow) {
        hideButton.setVisible(false);
    }
    setAreaNameSize();
}

void LeftHeader::lookAndFeelChanged()
{
    Component::lookAndFeelChanged();

    // The ImageButtons are not automatically color-refreshed.
    setHiddenButton(displayData.isHidden());
}

void LeftHeader::onUserWantsToHide() const noexcept
{
    if (canHideRow) {
        listener.onLeftHeaderHideClicked(areaType);
    }
}


// CustomMouseListener method implementations.
// ===============================================

void LeftHeader::CustomMouseListener::mouseDown(const juce::MouseEvent& /*event*/)
{
    parentObject.onUserWantsToHide();
}

void LeftHeader::CustomMouseListener::mouseEnter(const juce::MouseEvent& event)
{
    parentObject.mouseEnter(event);
}

void LeftHeader::CustomMouseListener::mouseExit(const juce::MouseEvent& event)
{
    parentObject.mouseExit(event);
}

}   // namespace arkostracker
