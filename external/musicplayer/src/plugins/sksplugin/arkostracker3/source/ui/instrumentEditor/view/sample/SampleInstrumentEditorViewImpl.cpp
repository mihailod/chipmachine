#include "SampleInstrumentEditorViewImpl.h"

#include "../../../../utils/NumberUtil.h"
#include "../../controller/sample/SampleInstrumentEditorController.h"

namespace arkostracker 
{

SampleInstrumentEditorViewImpl::SampleInstrumentEditorViewImpl(SampleInstrumentEditorController& pController, const double pMultiplierMinimum,
                                                               const double pMultiplierMaximum, const double pMultiplierStep) noexcept :
        controller(pController),
        sampleViewer(*this, pMultiplierMinimum, pMultiplierMaximum, pMultiplierStep),
        waveViewerDisplayedData()
{
    addAndMakeVisible(sampleViewer);
}

void SampleInstrumentEditorViewImpl::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    sampleViewer.setBounds(0, 0, width, height);
}


// SampleInstrumentEditorView method implementations.
// ===================================================

void SampleInstrumentEditorViewImpl::refreshAllUi(const SamplePart newSamplePart, const WaveViewerDisplayedData newWaveViewerDisplayedData) noexcept
{
    // If no change, don't do anything.
    if ((waveViewerDisplayedData == newWaveViewerDisplayedData) && (samplePart == newSamplePart)) {
        return;
    }
    samplePart = newSamplePart;
    waveViewerDisplayedData = newWaveViewerDisplayedData;

    sampleViewer.setSampleDataAndRefreshUi(samplePart, waveViewerDisplayedData);
}

void SampleInstrumentEditorViewImpl::refreshUi(const WaveViewerDisplayedData newWaveViewerDisplayedData) noexcept
{
    // If no change, don't do anything.
    if (waveViewerDisplayedData == newWaveViewerDisplayedData) {
        return;
    }
    waveViewerDisplayedData = newWaveViewerDisplayedData;

    sampleViewer.setSampleDataAndRefreshUi(samplePart, waveViewerDisplayedData);
}


// SampleViewer::Listener method implementations.
// ===================================================

void SampleInstrumentEditorViewImpl::onUserWantsToZoom(const bool zoomIn, const int clickedOffset) noexcept
{
    controller.onUserWantsToZoom(zoomIn, clickedOffset);
}

void SampleInstrumentEditorViewImpl::onUserWantsToScroll(const bool right) noexcept
{
    controller.onUserWantsToScroll(right);
}

void SampleInstrumentEditorViewImpl::onUserWantsToScrollTo(const int newStartOffset) noexcept
{
    controller.onUserWantsToScrollTo(newStartOffset);
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeEnd(const int newEnd) noexcept
{
    controller.onUserWantsToSetNewLoop({}, newEnd, {});
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeLoopStart(const int newLoopStart) noexcept
{
    controller.onUserWantsToSetNewLoop(newLoopStart, {}, {});
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeLoopState(const bool newIsLooping) noexcept
{
    controller.onUserWantsToSetNewLoop({}, {}, newIsLooping);
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeVolumeAmplification(const double newAmplification) noexcept
{
    controller.onUserantsToChangedMultiplier(newAmplification);
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeDigiNote(const int newDigiNote) noexcept
{
    controller.onUserWantsToChangeDigiNote(newDigiNote);
}

void SampleInstrumentEditorViewImpl::onUserWantsToChangeSampleFile(const juce::File& fileToLoad) noexcept
{
    controller.onUserWantsToChangeSampleFile(fileToLoad);
}

}   // namespace arkostracker

