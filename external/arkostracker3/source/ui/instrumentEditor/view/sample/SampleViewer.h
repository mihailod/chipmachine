#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../song/instrument/sample/SamplePart.h"
#include "../../../../utils/WithParent.h"
#include "../../../components/BlankComponent.h"
#include "../../../components/DraggableImage.h"
#include "../../../components/SliderIncDec.h"
#include "WaveViewer.h"

namespace arkostracker
{

/** Shows a sample (including the scrollbar and header to change the zoom, start/end, etc.). */
class SampleViewer final : public juce::Component,
                           public juce::ScrollBar::Listener,
                           public DraggableImage::Listener,
                           public SliderIncDec::Listener
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user wants to zoom in or out.
         * @param zoomIn true to zoom in, false to zoom out.
         * @param clickedOffset the clicked offset in the sample.
         */
        virtual void onUserWantsToZoom(bool zoomIn, int clickedOffset) noexcept = 0;

        /**
         * Called when the user wants to scroll.
         * @param right true to go right, false to go left.
         */
        virtual void onUserWantsToScroll(bool right) noexcept = 0;

        /**
         * Called when the user wants to scroll to a specific position.
         * @param newStartOffset the new offset.
         */
        virtual void onUserWantsToScrollTo(int newStartOffset) noexcept = 0;

        /**
         * Called when the user wants to change the sample end.
         * @param newEnd the new end.
         */
        virtual void onUserWantsToChangeEnd(int newEnd) noexcept = 0;

        /**
         * Called when the user wants to change the sample end.
         * @param newLoopStart the new start.
         */
        virtual void onUserWantsToChangeLoopStart(int newLoopStart) noexcept = 0;

        /**
         * Called when the user wants to change the loop state.
         * @param newIsLooping the new loop state.
         */
        virtual void onUserWantsToChangeLoopState(bool newIsLooping) noexcept = 0;

        /**
         * Called when the user wants to change the amplification.
         * @param newAmplification the new amplification.
         */
        virtual void onUserWantsToChangeVolumeAmplification(double newAmplification) noexcept = 0;

        /**
         * Called when the user wants to change the diginote.
         * @param newDigiNote the new diginote.
         */
        virtual void onUserWantsToChangeDigiNote(int newDigiNote) noexcept = 0;

        /**
         * Called when the user wants to change the sample itself.
         * @param fileToLoad the WAV file to load. May be absent.
         */
        virtual void onUserWantsToChangeSampleFile(const juce::File& fileToLoad) noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param multiplierMinimum the minimum value of the multiplier.
     * @param multiplierMaximum the maximum value of the multiplier.
     * @param multiplierStep the step the multiplier.
     */
    SampleViewer(Listener& listener, double multiplierMinimum, double multiplierMaximum, double multiplierStep) noexcept;

    /**
     * Sets the sample data to display, refreshes the UI. Nothing happens if there is no change.
     * @param samplePart the sample part.
     * @param displayedData how to display the data.
     */
    void setSampleDataAndRefreshUi(SamplePart samplePart, const WaveViewerDisplayedData& displayedData) noexcept;

    // Component method implementations.
    // ===================================================
    void resized() override;

    // ScrollBar::Listener method implementations.
    // ===================================================
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    // DraggableImage::Listener method implementations.
    // ===================================================
    void onDraggableImageDragged(DraggableImage& draggedImage, const juce::MouseEvent& mouseEvent) override;

    // SliderIncDec::Listener method implementations.
    // ===================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    static const int lineMarkerWidth;
    static const int scrollStepWhenBoundariesReached;   // How fast the scrolling moves when dragging a flag and reaching the boundaries.
    static const int comboBoxIdOffset;                  // Because ID 0 is not possible in Juce.
    static const int sampleViewerMinimumHeight;
    static const int sampleViewerMaximumHeight;

    class WaveViewerMouseListener final : public juce::MouseListener,
                                          public WithParent<SampleViewer>
    {
    public:
        explicit WaveViewerMouseListener(SampleViewer& parent) :
                WithParent(parent)
        {
        }

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    };

    /** Called when the multiplier slider value has changed. */
    void onUserMovedMultiplierSlider() const noexcept;

    /** Sets the bars at their locations. The width of the WaveViewer should be set, else nothing will happen. */
    void locateLoopBars() noexcept;

    /** Called when the user changed the loop toggle state. */
    void onLoopToggleClicked(bool currentState) const noexcept;

    /** Fills the diginote ComboBox with notes. */
    void fillDigiNoteComboBox() noexcept;

    /** Called when the diginote changed. Notifies the Controller. */
    void onDigiNoteChanged() const noexcept;

    /** Opens a browser to load a new sample file. When done, notifies the Controller. */
    void onLoadSampleButtonClicked() const noexcept;

    Listener& listener;

    DraggableImage imageStartMarker;
    DraggableImage imageEndMarker;
    BlankComponent lineStartMarker;
    BlankComponent lineEndMarker;

    SamplePart samplePart;
    WaveViewerDisplayedData waveViewerDisplayedData;

    WaveViewer waveViewer;

    SliderIncDec loopStartSlider;
    SliderIncDec endSlider;
    juce::Label lengthLabel;
    ButtonWithImage loopButton;

    juce::Label multiplierLabel;
    juce::Slider multiplierSlider;
    juce::Label sampleFrequencyLabel;

    juce::Label digidrumNoteLabel;
    juce::ComboBox digidrumNoteComboBox;

    juce::Label originalFileNameLabel;

    ButtonWithImage loadSampleButton;

    WaveViewerMouseListener waveViewerMouseListener;
    juce::ScrollBar horizontalScrollBar;
};

}   // namespace arkostracker
