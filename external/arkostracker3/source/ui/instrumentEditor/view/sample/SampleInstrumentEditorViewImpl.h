#pragma once

#include "../../../../song/instrument/sample/SamplePart.h"
#include "SampleInstrumentEditorView.h"
#include "SampleViewer.h"

namespace arkostracker 
{

class SampleInstrumentEditorController;

/** Implementation of the Sample Instrument Editor. */
class SampleInstrumentEditorViewImpl final : public SampleInstrumentEditorView,
                                             public SampleViewer::Listener
{
public:
    /**
     * Constructor.
     * @param controller the controller.
     * @param multiplierMinimum the minimum value of the multiplier.
     * @param multiplierMaximum the maximum value of the multiplier.
     * @param multiplierStep the step the multiplier.
     */
    explicit SampleInstrumentEditorViewImpl(SampleInstrumentEditorController& controller, double multiplierMinimum,
                                            double multiplierMaximum, double multiplierStep) noexcept;

    void resized() override;

    // SampleInstrumentEditorView method implementations.
    // ===================================================
    void refreshAllUi(SamplePart samplePart, WaveViewerDisplayedData waveViewerDisplayedData) noexcept override;
    void refreshUi(WaveViewerDisplayedData waveViewerDisplayedData) noexcept override;

    // SampleViewer::Listener method implementations.
    // ===================================================
    void onUserWantsToZoom(bool zoomIn, int clickedOffset) noexcept override;
    void onUserWantsToScroll(bool right) noexcept override;
    void onUserWantsToScrollTo(int newStartOffset) noexcept override;
    void onUserWantsToChangeEnd(int newEnd) noexcept override;
    void onUserWantsToChangeLoopStart(int newLoopStart) noexcept override;
    void onUserWantsToChangeLoopState(bool newIsLooping) noexcept override;
    void onUserWantsToChangeVolumeAmplification(double newAmplification) noexcept override;
    void onUserWantsToChangeDigiNote(int newDigiNote) noexcept override;
    void onUserWantsToChangeSampleFile(const juce::File& fileToLoad) noexcept override;

private:
    SampleInstrumentEditorController& controller;

    SampleViewer sampleViewer;
    SamplePart samplePart;
    WaveViewerDisplayedData waveViewerDisplayedData;
};

}   // namespace arkostracker
