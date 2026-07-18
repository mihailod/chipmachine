#include "ChannelView.h"

#include "../../audio/Volumes.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "controller/MetersController.h"

namespace arkostracker 
{

ChannelView::ChannelView(MetersController& pMetersController, const int pChannelIndex, const PsgType pPsgType, const bool pMuted, const size_t pSignalBufferSize) noexcept :
        metersController(pMetersController),
        channelIndex(pChannelIndex),
        bufferToDisplay(pSignalBufferSize, true),
        signalBufferSize(pSignalBufferSize),

        muted(pMuted),

        labelChannelNumber(juce::String(), juce::String(pChannelIndex + 1)), // Displays the channel number.

        psgType(pPsgType),

        labelMouseListener(*this),

        // Colors are set below.
        colorMetersBackground(),
        colorMetersScaleLines(),
        colorMetersLabelBackground(),
        colorMetersLabel(),
        colorMetersShape(),
        colorMetersMuteFilter()
{
    setOpaque(true);

    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);

    updateInternalColorsAndNativeComponents();

    // Sets to the Label where the channel number is written.
    // The position is not set here, but on resize.

    // Simplistic operation to have the size of the label grows according to the number of letters of the channel number.
    const auto labelWidth = labelChannelNumberBaseSize + (labelChannelNumber.getText().length() * widthAddedPerDigit);
    labelChannelNumber.setSize(labelWidth, labelChannelNumberHeight);
    addAndMakeVisible(labelChannelNumber);

    labelChannelNumber.setInterceptsMouseClicks(true, false);
    labelChannelNumber.addMouseListener(&labelMouseListener, false);
    labelChannelNumber.setMouseClickGrabsKeyboardFocus(false);
}

int ChannelView::getChannelIndex() const noexcept
{
    return channelIndex;
}

juce::HeapBlock<uint16_t>& ChannelView::getDisplayBuffer() noexcept
{
    return bufferToDisplay;
}

void ChannelView::LabelMouseListener::mouseDown(const juce::MouseEvent& /*event*/)
{
    // Asks the MainController about going to the clicked Track number.
    //ChannelView& parent = getParentOfObject();
    //parent.mainController.gotoChannel(parent.channelIndex);
}

void ChannelView::setPsgStyle(const PsgType newPsgType) noexcept
{
    // Calculates the new scale, only if necessary.
    if (psgType != newPsgType) {
        psgType = newPsgType;
    }
}

void ChannelView::resized()
{
    // On resize, relocate the Label where the channel index is written.
    const auto newWidth = getWidth();
    const auto labelChannelNumberWidth = labelChannelNumber.getWidth();
    labelChannelNumber.setBounds(newWidth - labelChannelNumberWidth - labelChannelNumberBorderSize, labelChannelNumberBorderSize, labelChannelNumberWidth, labelChannelNumberHeight);
}

void ChannelView::lookAndFeelChanged()
{
    updateInternalColorsAndNativeComponents();     // No need to make a repaint.
}

void ChannelView::refreshChannelView() noexcept
{
    repaint();
}

void ChannelView::mouseDown(const juce::MouseEvent& mouseEvent)
{
    // Notifies the controller, but do NOT do anything.
    if (mouseEvent.mods.isLeftButtonDown()) {
        // Only changes the mute state of this channel.
        metersController.onWantToChangeMuteState(channelIndex, !muted);
    } else if (mouseEvent.mods.isRightButtonDown()) {
        // All the states must be changed (solo/not solo feature).
        metersController.onWantToAllMuteExcept(channelIndex);
    }
}

void ChannelView::changeMuteState(const bool newMuteState) noexcept
{
    if (muted != newMuteState) {
        muted = newMuteState;
        repaint();
    }
}

void ChannelView::paint(juce::Graphics& g)
{
    const auto localMuted = muted;

    g.fillAll(localMuted ? colorMetersBackground.interpolatedWith(colorMetersMuteFilter, 0.4F) : colorMetersBackground);

    // Sets the default colors.
    auto lineColor = colorMetersShape;
    auto scalesLineColor = colorMetersScaleLines;
    // If muted, changes the colors.
    if (localMuted) {
        lineColor = lineColor.interpolatedWith(colorMetersMuteFilter, 0.5F);
        scalesLineColor = scalesLineColor.interpolatedWith(colorMetersMuteFilter, 0.5F);
    }

    // Draws the horizontal scale lines in the background.
    const auto& volumeTable = (psgType == PsgType::ay) ? Volumes::baseVolumesAy : Volumes::baseVolumesYm;
    const auto height = static_cast<float>(getHeight());
    const auto width = static_cast<float>(getWidth());
    const auto yScale = static_cast<float>(Volumes::maximumValue) / height;
    const auto bottom = height;
    g.setColour(scalesLineColor);
    // Draws the half-steps. On AY, it won't change anything.
    for (auto volumeIndex = 1; volumeIndex < volumeCount; volumeIndex += 2) {        // Skips the "half steps".
        const auto yLine = bottom - (static_cast<float>(volumeTable.at(static_cast<size_t>(volumeIndex))) / yScale);
        g.drawLine(0.0F, yLine, width - 1.0F, yLine, 1.0F);
    }

    // Not muted. Displays the full EQ.
    g.setColour(lineColor);

    const auto stabilizationWindowsLength = static_cast<unsigned int>(signalBufferSize) / 4U;       // Arbitrary. The bigger, the smaller the displayed window!

    // Stabilization: finds the best high-shelf.
    auto slidingPositionInBuffer = 0U;
    auto positionBestCandidate = 0U;
    auto bestCandidateDifference = 0;
    auto continueStabilization = true;

    while ((slidingPositionInBuffer < stabilizationWindowsLength) && continueStabilization) {
        const auto firstValue = static_cast<int>(bufferToDisplay[slidingPositionInBuffer]);
        const auto secondValue = static_cast<int>(bufferToDisplay[slidingPositionInBuffer + 1U]);
        if (secondValue > firstValue) {
            // Found one high-shelf. Is it the best we could find? The best is
            const auto difference = secondValue - firstValue;
            // If the difference was the best we can imagine (0 to max), we can stop here.
            if ((firstValue == 0) && (difference == Volumes::maximumValue)) {
                continueStabilization = false;
                positionBestCandidate = slidingPositionInBuffer;
            } else if (difference > bestCandidateDifference) {
                // If the difference is "only" better than the best candidate, it becomes the best candidate.
                positionBestCandidate = slidingPositionInBuffer + 1U;
                bestCandidateDifference = difference;
            }
        }
        if (continueStabilization) {
            ++slidingPositionInBuffer;
        }
    }
    // Success or not, we use the best we have.
    auto positionInBuffer = static_cast<double>(positionBestCandidate);
    jassert(positionInBuffer <= stabilizationWindowsLength);

    // Calculates the step to map a value in the buffer to a pixel on the screen.
    const auto stepInBuffer = static_cast<double>(signalBufferSize - stabilizationWindowsLength) / width;

    // Draws vertical lines for each X.
    auto previousTop = 0.0F;
    for (auto x = 0; x < static_cast<int>(width); ++x) {
        // No need to check for AY/YM: the signal is generated the right way! Directly 16 bits.
        const auto originalValue = bufferToDisplay[static_cast<size_t>(positionInBuffer)];

        // Draws a vertical line from the bottom (always the same position) to a top.
        const auto top = bottom - (static_cast<float>(originalValue) / yScale);

        // Links the previous point to the new one (so, ignores X == 0).
        if (x > 0) {
            // Draws a line from the previous top to the new top.
            g.setColour(lineColor);
            g.drawLine(static_cast<float>(x) - 1.0F, previousTop, static_cast<float>(x), top);
        }

        previousTop = top;

        // Moves inside the buffer from a possibly very small step.
        positionInBuffer = positionInBuffer + stepInBuffer;
    }

    // If muted, displays "muted".
    if (localMuted) {
        g.setFont(juce::Font(16.0F));
        g.setColour(colorMetersLabel);
        g.drawText(juce::translate("Muted"), getLocalBounds(), juce::Justification::centred, true);
    }
}

void ChannelView::updateInternalColorsAndNativeComponents() noexcept
{
    const auto& defaultLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    colorMetersBackground = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersBackground));
    colorMetersScaleLines = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersScaleLines));
    colorMetersLabelBackground = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersLabelBackground));
    colorMetersLabel = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersLabel));
    colorMetersShape = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersShape));
    //colorMetersVolume = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersVolume));
    //colorMetersVolumeIfHardware = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersVolumeIfHardware));
    //colorMetersNoise = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersNoise));
    colorMetersMuteFilter = defaultLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::metersMuteFilter));

    // Native components must have their color changed now.
    labelChannelNumber.setColour(juce::Label::textColourId, colorMetersLabel);
    labelChannelNumber.setColour(juce::Label::backgroundColourId, colorMetersLabelBackground);
}

}   // namespace arkostracker
