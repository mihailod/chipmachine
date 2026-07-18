#include "LineHeader.h"

#include <BinaryData.h>

#include "../../../../../utils/NumberUtil.h"
#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

LineHeader::LineHeader(Listener& pListener, const int pHeightToShow, const bool pShowInHex) noexcept :
        listener(pListener),
        shownHeight(pHeightToShow),
        showInHex(pShowInHex),
        font(juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMonoB_ttf, BinaryData::UbuntuMonoB_ttfSize))
{
}

void LineHeader::setDisplayedData(const int newHeight, const bool newShowInHex) noexcept
{
    // Any change?
    if ((shownHeight == newHeight) && (showInHex == newShowInHex)) {
        return;
    }

    shownHeight = newHeight;
    showInHex = newShowInHex;
    repaint();
}


// BaseHeaderView method implementations.
// =======================================

void LineHeader::paintAfter(juce::Graphics& g, const juce::Colour /*backgroundColor*/) noexcept
{
    const auto width = getWidth();
    const auto height = getHeight();

    // Displays the height.
    g.setColour(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId));
    g.setFont(font);
    g.drawFittedText(
        showInHex ? NumberUtil::toUnsignedHex(shownHeight) : juce::String(shownHeight),
        0, 0, width, height, juce::Justification::centred, 1, 1.0F);
}

juce::Colour LineHeader::getBackgroundColor() const noexcept
{
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    return localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerLineTrackHeaderBackground));
}


// Component method implementations.
// =====================================

void LineHeader::mouseDown(const juce::MouseEvent& /*event*/)
{
    listener.onUserClickedOnHeight();
}

void LineHeader::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!juce::exactlyEqual(wheel.deltaY, 0.0F)) {
        listener.onUserUsedMouseWheelOnHeight(wheel.deltaY > 0.0F, event.mods.isShiftDown());
    }
}

}   // namespace arkostracker
