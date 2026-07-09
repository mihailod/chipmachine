#include "SampleViewer.h"

#include "../../../../song/cells/CellConstants.h"
#include "../../../../utils/FileExtensions.h"
#include "../../../../utils/NoteUtil.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../components/FileChooserCustom.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "BinaryData.h"

namespace arkostracker
{

const int SampleViewer::lineMarkerWidth = 1;
const int SampleViewer::scrollStepWhenBoundariesReached = 100;
const int SampleViewer::comboBoxIdOffset = 1;

const int SampleViewer::sampleViewerMinimumHeight = 100;
const int SampleViewer::sampleViewerMaximumHeight = 300;

SampleViewer::SampleViewer(Listener& pListener, const double pMultiplierMinimum, const double pMultiplierMaximum, const double pMultiplierStep) noexcept:
        listener(pListener),
        imageStartMarker(*this, BinaryData::SampleLoopStart_png, static_cast<size_t>(BinaryData::SampleLoopStart_pngSize),
                         juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd))),
        imageEndMarker(*this, BinaryData::SampleLoopEnd_png, static_cast<size_t>(BinaryData::SampleLoopEnd_pngSize),
                       juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd))),
        lineStartMarker(juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd))),
        lineEndMarker(juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd))),
        samplePart(),
        waveViewerDisplayedData(),
        waveViewer(),
        loopStartSlider(*this, juce::translate("Loop to"), 4, 1.0, 8.0),
        endSlider(*this, juce::translate("End"), 4, 1.0, 8.0),
        lengthLabel(),
        loopButton(BinaryData::IconLoopOff_png, BinaryData::IconLoopOff_pngSize,
                   BinaryData::IconLoopOn_png, BinaryData::IconLoopOn_pngSize,
                   juce::translate("Toggle the loop"),
                   [&] (int, const bool newState, bool) { onLoopToggleClicked(newState); }),
        multiplierLabel(juce::String(), juce::translate("Gain")),
        multiplierSlider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft),
        sampleFrequencyLabel(),
        digidrumNoteLabel(juce::String(), juce::translate("Diginote")),
        digidrumNoteComboBox(),
        originalFileNameLabel(juce::String(), juce::String()),
        loadSampleButton(
            BinaryData::IconFolder_png, BinaryData::IconFolder_pngSize, juce::translate("Replace sample"),
            [&] (int, bool, bool) { onLoadSampleButtonClicked(); }),
        waveViewerMouseListener(*this),
        horizontalScrollBar(false)
{
    // The WV must be able to get the focus (see also onWantToChangeSliderValue).
    waveViewer.setWantsKeyboardFocus(true);

    multiplierSlider.onValueChange = [&] { onUserMovedMultiplierSlider(); };
    multiplierSlider.setRange(pMultiplierMinimum, pMultiplierMaximum, pMultiplierStep);
    waveViewer.addMouseListener(&waveViewerMouseListener, false);
    horizontalScrollBar.setAutoHide(true);
    horizontalScrollBar.addListener(this);
    digidrumNoteComboBox.onChange = [&] { onDigiNoteChanged(); };
    digidrumNoteComboBox.setTooltip(juce::translate("Note used when the sample is played in the \"events\" track."));

    fillDigiNoteComboBox();

    lengthLabel.setEnabled(false);
    sampleFrequencyLabel.setEnabled(false);
    originalFileNameLabel.setEnabled(false);

    addAndMakeVisible(waveViewer);

    addAndMakeVisible(loopStartSlider);
    addAndMakeVisible(endSlider);
    addAndMakeVisible(lengthLabel);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(multiplierLabel);
    addAndMakeVisible(multiplierSlider);
    addAndMakeVisible(sampleFrequencyLabel);
    addAndMakeVisible(digidrumNoteLabel);
    addAndMakeVisible(digidrumNoteComboBox);
    addAndMakeVisible(loadSampleButton);
    addAndMakeVisible(originalFileNameLabel);

    addAndMakeVisible(horizontalScrollBar);

    addChildComponent(imageStartMarker);        // Invisible if no loop.
    addChildComponent(lineStartMarker);
    addAndMakeVisible(imageEndMarker);
    addAndMakeVisible(lineEndMarker);
}


// Component method implementations.
// ===================================================

void SampleViewer::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::smallMargins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto scrollBarHeight = juce::LookAndFeel::getDefaultLookAndFeel().getDefaultScrollbarWidth();

    const auto top = margins;
    const auto left = smallMargins;
    const auto availableWidth = width - (2 * smallMargins);

    const auto captionY = top;

    loopStartSlider.setBounds(left, captionY, 170, labelsHeight);
    endSlider.setBounds(loopStartSlider.getRight(), captionY, 150, labelsHeight);
    loopButton.setBounds(endSlider.getRight(), captionY, LookAndFeelConstants::iconButtonsWidth, labelsHeight);

    multiplierLabel.setBounds(loopButton.getRight() + margins, captionY, 45, labelsHeight);
    multiplierSlider.setBounds(multiplierLabel.getRight(), captionY, 150, labelsHeight);

    digidrumNoteLabel.setBounds(multiplierSlider.getRight() + smallMargins, captionY, 70, labelsHeight);
    digidrumNoteComboBox.setBounds(digidrumNoteLabel.getRight(), captionY, 70, labelsHeight);

    loadSampleButton.setBounds(digidrumNoteComboBox.getRight() + (margins * 2), captionY, 32, labelsHeight);

    const auto separatorHeightSampleAndBottomFields = margins;
    const auto bottomFieldsHeight = labelsHeight;
    const auto bottomAreaHeight = bottomFieldsHeight + separatorHeightSampleAndBottomFields;

    const auto waveViewerY = multiplierLabel.getBottom() + margins;
    const auto remainingSpaceForWaveViewer = height - waveViewerY - bottomAreaHeight;
    const auto waveViewerMaximumHeight = sampleViewerMaximumHeight - bottomAreaHeight;
    const auto waveViewerMinimumHeight = sampleViewerMinimumHeight - bottomAreaHeight;
    jassert(waveViewerMinimumHeight > 0);

    const auto waveViewerWidth = availableWidth;
    const auto waveViewerHeight = NumberUtil::correctNumber(remainingSpaceForWaveViewer, waveViewerMinimumHeight, waveViewerMaximumHeight);

    waveViewer.setBounds(left, waveViewerY, waveViewerWidth, waveViewerHeight);
    horizontalScrollBar.setBounds(waveViewer.getX(), waveViewer.getBottom() - scrollBarHeight, waveViewerWidth, scrollBarHeight);

    // Corner case, the loop bars are displayed too soon, before the wave viewer has its size set. So we do it now.
    locateLoopBars();

    waveViewer.setExplicitFocusOrder(1);        // Else, the first Slider would get focus.

    // Under the sample, some fields.
    const auto bottomAreaY = waveViewer.getBottom() + separatorHeightSampleAndBottomFields;
    lengthLabel.setBounds(left, bottomAreaY, 110, bottomFieldsHeight);
    sampleFrequencyLabel.setBounds(lengthLabel.getRight() + margins, bottomAreaY, 110, labelsHeight);
    const auto originalFileNameLabelX = sampleFrequencyLabel.getRight() + margins;
    const auto originalFileNameLabelWidth = availableWidth - originalFileNameLabelX;
    originalFileNameLabel.setBounds(originalFileNameLabelX, bottomAreaY, originalFileNameLabelWidth, labelsHeight);
}


// ===================================================

void SampleViewer::setSampleDataAndRefreshUi(SamplePart pSamplePart, const WaveViewerDisplayedData& pDisplayedData) noexcept
{
    // Any change? If not, don't do anything.
    if ((samplePart == pSamplePart) && (waveViewerDisplayedData == pDisplayedData)) {
        return;
    }

    samplePart = std::move(pSamplePart);
    waveViewerDisplayedData = pDisplayedData;

    waveViewer.setSampleAndRefresh(samplePart.getSample(), pDisplayedData);

    const auto loop = samplePart.getLoop();
    const auto loopStartIndex = loop.getStartIndex();
    const auto endIndex = loop.getEndIndex();

    const auto startViewedOffset = pDisplayedData.getStartViewedOffset();
    const auto pastEndViewedOffset = pDisplayedData.getPastEndViewedOffset();
    const auto sampleLength = pDisplayedData.getSampleLength();

    // Updates the captions.
    const auto lengthText = juce::translate("Length: ") + juce::String::toHexString(sampleLength);
    lengthLabel.setText(lengthText, juce::NotificationType::dontSendNotification);
    const auto sampleFrequencyText = juce::translate("Freq: ") + juce::String(pDisplayedData.getSampleFrequencyHz()) + juce::translate("hz");
    sampleFrequencyLabel.setText(sampleFrequencyText, juce::NotificationType::dontSendNotification);
    loopStartSlider.setShownValue(loopStartIndex);
    endSlider.setShownValue(endIndex);
    loopButton.setState(pDisplayedData.isLooping());
    multiplierSlider.setValue(samplePart.getAmplificationRatio(), juce::NotificationType::dontSendNotification);
    const auto& originalFileName = samplePart.getOriginalFileName();
    originalFileNameLabel.setText(originalFileName.isEmpty() ? juce::String() : "Original file name: " + originalFileName, juce::NotificationType::dontSendNotification);

    // Updates the start/end bars.
    locateLoopBars();

    // Updates the scrollbar.
    horizontalScrollBar.setRangeLimits(0.0, static_cast<double>(sampleLength - 1));
    horizontalScrollBar.setCurrentRangeStart(static_cast<double>(startViewedOffset), juce::NotificationType::dontSendNotification);
    horizontalScrollBar.setCurrentRange(static_cast<double>(startViewedOffset), static_cast<double>(pastEndViewedOffset - startViewedOffset),
                                        juce::NotificationType::dontSendNotification);

    // Diginote.
    digidrumNoteComboBox.setSelectedItemIndex(pDisplayedData.getDigidrumNote(), juce::dontSendNotification);
}

void SampleViewer::locateLoopBars() noexcept
{
    const auto waveViewerWidth = waveViewer.getWidth();
    if (waveViewerWidth <= 0) {
        return;
    }

    const auto barTop = waveViewer.getY();
    const auto barBottom = waveViewer.getBottom() - 1;
    const auto barHeight = barBottom - barTop;
    const auto& loop = samplePart.getLoop();

    // The start bar only appears if looping.
    const auto isLooping = waveViewerDisplayedData.isLooping();
    const auto startBarX = waveViewer.getXFromSampleOffset(loop.getStartIndex());
    const auto startBarVisible = isLooping && startBarX.isPresent();
    if (startBarVisible) {
        const auto imageWidth = imageStartMarker.getImageWidth();
        const auto imageHeight = imageStartMarker.getImageHeight();

        const auto x = waveViewer.getX() + startBarX.getValue();
        imageStartMarker.setBounds(x, barTop, imageWidth, imageHeight);
        lineStartMarker.setBounds(x, barTop, lineMarkerWidth, barHeight);
    }
    imageStartMarker.setVisible(startBarVisible);
    lineStartMarker.setVisible(startBarVisible);

    const auto endBarXOptional = waveViewer.getXFromSampleOffset(loop.getEndIndex() + 1, true);   // The bar encompasses the last read byte.
    const auto endBarVisible = endBarXOptional.isPresent();
    if (endBarVisible) {
        const auto endBarX = endBarXOptional.getValue();
        const auto imageWidth = imageEndMarker.getImageWidth();
        const auto imageHeight = imageEndMarker.getImageHeight();

        // A bit of a trick. Makes the X goes left for the bar to be seen if on the full right.
        const auto offsetX = (endBarX == waveViewerWidth) ? -1 : 0;
        const auto x = waveViewer.getX() + endBarX + offsetX;
        imageEndMarker.setBounds(x - imageWidth + 1, barBottom - imageHeight, imageWidth, imageHeight);
        lineEndMarker.setBounds(x, barTop, lineMarkerWidth, barHeight);
    }
    imageEndMarker.setVisible(endBarVisible);
    lineEndMarker.setVisible(endBarVisible);
}

void SampleViewer::onUserMovedMultiplierSlider() const noexcept
{
    listener.onUserWantsToChangeVolumeAmplification(multiplierSlider.getValue());
}

void SampleViewer::onLoopToggleClicked(const bool currentState) const noexcept
{
    listener.onUserWantsToChangeLoopState(!currentState);
}

void SampleViewer::fillDigiNoteComboBox() noexcept
{
    for (auto noteIndex = CellConstants::minimumNote; noteIndex <= CellConstants::maximumNote; ++noteIndex) {
        const auto displayedNote = NoteUtil::getStringFromNote(noteIndex);
        digidrumNoteComboBox.addItem(displayedNote, noteIndex + comboBoxIdOffset);
    }
}

void SampleViewer::onDigiNoteChanged() const noexcept
{
    const auto digiNote = digidrumNoteComboBox.getSelectedItemIndex();
    listener.onUserWantsToChangeDigiNote(digiNote);
}

void SampleViewer::onLoadSampleButtonClicked() const noexcept
{
    const auto extension = FileExtensions::wavExtensionWithWildcard;
    FileChooserCustom fileChooser(juce::String("Load a WAV file"), FolderContext::other, extension);
    if (const auto success = fileChooser.browseForFileToOpen(nullptr); !success) {
        return;
    }

    const auto fileToLoad = fileChooser.getResultWithExtensionIfNone(extension);
    listener.onUserWantsToChangeSampleFile(fileToLoad);
}


// ScrollBar::Listener method implementations.
// ===================================================

void SampleViewer::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, const double newRangeStart)
{
    jassert(scrollBarThatHasMoved == &horizontalScrollBar); (void)scrollBarThatHasMoved;
    listener.onUserWantsToScrollTo(static_cast<int>(newRangeStart));
}


// DraggableImage::Listener method implementations.
// ===================================================

void SampleViewer::onDraggableImageDragged(DraggableImage& draggedImage, const juce::MouseEvent& mouseEvent)
{
    // First, test if there be a scrolling? The mouseEvent is relative to the Flag!
    const auto mouseEventInWaveViewerWrapper = mouseEvent.getEventRelativeTo(this);
    const auto mouseX = mouseEventInWaveViewerWrapper.x;
    auto stepToScroll = 0;
    auto isLeftBoundaryReached = false;
    if (mouseX < 0) {
        stepToScroll = -scrollStepWhenBoundariesReached;
        isLeftBoundaryReached = true;
    } else if (mouseX >= (waveViewer.getWidth() - 1)) {
        stepToScroll = scrollStepWhenBoundariesReached;
    }

    // Scrolling?
    if (stepToScroll != 0) {
        // Makes the flag appear at the boundary.
        const auto flagOffsetInSample = isLeftBoundaryReached ?
                                        waveViewerDisplayedData.getStartViewedOffset() :       // The offset is the offset at the left corner.
                                        // The offset is the offset at the right corner.
                                        waveViewerDisplayedData.getPastEndViewedOffset() - 1;

        // Sets the modified loop.
        if (&draggedImage == &imageStartMarker) {
            listener.onUserWantsToChangeLoopStart(flagOffsetInSample);
        } else if (&draggedImage == &imageEndMarker) {
            listener.onUserWantsToChangeEnd(flagOffsetInSample);
        } else {
            jassertfalse;       // Image not managed?
        }

        horizontalScrollBar.moveScrollbarInSteps(stepToScroll, juce::NotificationType::sendNotificationAsync);
        return;
    }

    // No scrolling. We're still inside the WaveViewer.
    const auto& loop = samplePart.getLoop();
    const auto dragX = mouseEvent.x;
    // Applies the zoom factor on the drag to move it at the right "speed".
    const auto dragSampleValue = static_cast<int>(waveViewer.getZoomFactor() * static_cast<double>(dragX));

    if (&draggedImage == &imageStartMarker) {
        listener.onUserWantsToChangeLoopStart(loop.getStartIndex() + dragSampleValue);
    } else if (&draggedImage == &imageEndMarker) {
        listener.onUserWantsToChangeEnd(loop.getEndIndex() + dragSampleValue);
    } else {
        jassertfalse;       // Image not managed?
    }
}


// SliderIncDec::Listener method implementations.
// ===================================================

void SampleViewer::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    const auto value = static_cast<int>(valueDouble);
    if (&slider == &loopStartSlider) {
        listener.onUserWantsToChangeLoopStart(value);
    } else if (&slider == &endSlider) {
        listener.onUserWantsToChangeEnd(value);
    } else {
        jassertfalse;
    }

    waveViewer.grabKeyboardFocus();     // Hack to give the focus to the WaveViewer.
}


// WaveViewerMouseListener method implementations.
// ===================================================

void SampleViewer::WaveViewerMouseListener::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (juce::exactlyEqual(wheel.deltaY, 0.0F)) {
        return;
    }

    if (event.mods.isAnyModifierKeyDown()) {
        // Finds the pointed offset from the clicked position.
        const auto mouseX = event.getPosition().getX();
        const auto width = parentObject.waveViewer.getWidth();
        const auto startOffset = parentObject.waveViewerDisplayedData.getStartViewedOffset();
        const auto endOffset = parentObject.waveViewerDisplayedData.getPastEndViewedOffset() - 1;

        const auto ratio = static_cast<double>(endOffset - startOffset) / static_cast<double>(width);
        const auto clickedOffset = (static_cast<double>(mouseX) * ratio) + static_cast<double>(startOffset);

        parentObject.listener.onUserWantsToZoom(wheel.deltaY > 0.0, static_cast<int>(clickedOffset));
    } else {
        parentObject.listener.onUserWantsToScroll(wheel.deltaY < 0.0);
    }
}


// ===================================================

}   // namespace arkostracker
