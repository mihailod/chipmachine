#include "SampleInstrumentEditorControllerImpl.h"

#include "../../../../controllers/SongController.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "../../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../../import/sample/LoadSampleTask.h"
#include "../../view/sample/SampleInstrumentEditorViewImpl.h"
#include "../../view/sample/SampleInstrumentEditorViewNoOp.h"

namespace arkostracker 
{

const double SampleInstrumentEditorControllerImpl::zoomRatio = 0.1;
const double SampleInstrumentEditorControllerImpl::minZoomStep = 20.0;
const double SampleInstrumentEditorControllerImpl::scrollSpeedRatio = 0.09;
const double SampleInstrumentEditorControllerImpl::sampleAmplificationStep = 0.1;

SampleInstrumentEditorControllerImpl::SampleInstrumentEditorControllerImpl(SongController& pSongController) noexcept :
        songController(pSongController),
        shownInstrumentId(),
        instrumentEditorView(std::make_unique<SampleInstrumentEditorViewNoOp>()),
        waveViewerDisplayedData(),
        loadSampleBackgroundTask(),
        dialog()
{
    songController.getInstrumentObservers().addObserver(this);
}

SampleInstrumentEditorControllerImpl::~SampleInstrumentEditorControllerImpl()
{
    songController.getInstrumentObservers().removeObserver(this);
}


// InstrumentChangeObserver method implementations.
// ===================================================

void SampleInstrumentEditorControllerImpl::onInstrumentChanged(const Id& id, unsigned int /*whatChanged*/)
{
    if (shownInstrumentId == id) {
        // Raw, but no need to do more, equally tests are performed.
        refreshAllUi();
    }
}

void SampleInstrumentEditorControllerImpl::onPsgInstrumentCellChanged(const Id& /*id*/, int /*cellIndex*/, bool /*mustRefreshAllAfterToo*/)
{
    // Nothing to do.
}

void SampleInstrumentEditorControllerImpl::onInstrumentsInvalidated()
{
    // Nothing to do.
}


// TypedInstrumentEditorController method implementations.
// ==========================================================

void SampleInstrumentEditorControllerImpl::onNewItemSelected(const Id& newInstrumentId, const bool force) noexcept
{
    if (!force && (OptionalId(newInstrumentId) == shownInstrumentId)) {
        return;
    }

    onSelectedItemChangedMustRefreshUi(newInstrumentId);
}

void SampleInstrumentEditorControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    instrumentEditorView = std::make_unique<SampleInstrumentEditorViewImpl>(*this, static_cast<double>(PsgValues::minimumSampleAmplificationRatio),
                                                                            static_cast<double >(PsgValues::maximumSampleAmplificationRatio),
                                                                            sampleAmplificationStep);

    parentView.addAndMakeVisible(*instrumentEditorView);

    refreshAllUi();
}

void SampleInstrumentEditorControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    instrumentEditorView->setBounds(startX, startY, newWidth, newHeight);
}

void SampleInstrumentEditorControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
    // Nothing to do.
}

void SampleInstrumentEditorControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to the no-op view.
    instrumentEditorView = std::make_unique<SampleInstrumentEditorViewNoOp>();
}

void SampleInstrumentEditorControllerImpl::getKeyboardFocus() noexcept
{
    instrumentEditorView->grabKeyboardFocus();
}


// ==========================================================

void SampleInstrumentEditorControllerImpl::onSelectedItemChangedMustRefreshUi(const Id& newInstrumentId) noexcept
{
    shownInstrumentId = newInstrumentId;

    waveViewerDisplayedData = WaveViewerDisplayedData();    // Invalidates the view to force a de-zoom.
    refreshAllUi();
}

void SampleInstrumentEditorControllerImpl::refreshAllUi() noexcept
{
    if (shownInstrumentId.isAbsent()) {
        jassertfalse;       // Nothing to show?
        return;
    }

    SamplePart samplePart;
    songController.performOnConstInstrument(shownInstrumentId.getValue(), [&](const Instrument& instrument) {
        samplePart = instrument.getConstSamplePart();
    });

    // Shows everything at first, else uses the same viewport.
    // However, corrects the view, needed in case the sample changes (undo/redo is performed).
    const auto sampleLength = samplePart.getSample()->getLength();
    const auto isViewPortValid = (waveViewerDisplayedData.getStartViewedOffset() < sampleLength) && (waveViewerDisplayedData.getPastEndViewedOffset() <= sampleLength);
    if (isViewPortValid && waveViewerDisplayedData.isDisplayValid()) {
        // Shows the same viewport.
        waveViewerDisplayedData = WaveViewerDisplayedData(samplePart.getAmplificationRatio(),
                                                          waveViewerDisplayedData.getStartViewedOffset(),
                                                          waveViewerDisplayedData.getPastEndViewedOffset(),
                                                          sampleLength,
                                                          samplePart.getFrequencyHz(), samplePart.getLoop().isLooping(),
                                                          samplePart.getDigidrumNote());
    } else {
        // First time, shows everything.
        waveViewerDisplayedData = WaveViewerDisplayedData(samplePart.getAmplificationRatio(), 0, sampleLength, sampleLength,
                                                          samplePart.getFrequencyHz(), samplePart.getLoop().isLooping(),
                                                          samplePart.getDigidrumNote());
    }

    instrumentEditorView->refreshAllUi(samplePart, waveViewerDisplayedData);
}


// SampleInstrumentEditorController method implementations.
// ==========================================================

void SampleInstrumentEditorControllerImpl::onUserantsToChangedMultiplier(const double newMultiplier) noexcept
{
    const auto instrumentId = shownInstrumentId;
    if (instrumentId.isAbsent()) {
        jassertfalse;       // Abnormal.
        return;
    }

    songController.setSampleInstrumentMetadata(instrumentId.getValueRef(), {}, {}, {}, static_cast<float>(newMultiplier), {});
}

void SampleInstrumentEditorControllerImpl::onUserWantsToSetNewLoop(const OptionalInt newLoopStart, const OptionalInt newEnd, const OptionalBool newIsLooping) noexcept
{
    const auto instrumentId = shownInstrumentId;
    if (instrumentId.isAbsent()) {
        jassertfalse;       // Abnormal.
        return;
    }

    songController.setSampleInstrumentMetadata(instrumentId.getValueRef(), newLoopStart, newEnd, newIsLooping, {}, {});
}

void SampleInstrumentEditorControllerImpl::onUserWantsToZoom(const bool zoomIn, const int clickedOffset) noexcept
{
    auto startOffset = waveViewerDisplayedData.getStartViewedOffset();
    auto pastEndOffset = waveViewerDisplayedData.getPastEndViewedOffset();
    const auto sampleLength = waveViewerDisplayedData.getSampleLength();
    const auto isLooping = waveViewerDisplayedData.isLooping();
    auto success = true;

    const auto shownLength = pastEndOffset - startOffset;
    const auto zoomStep = static_cast<int>(std::max(minZoomStep, static_cast<double>(shownLength) * zoomRatio));

    const auto shiftRatio = static_cast<double>(clickedOffset - startOffset) / static_cast<double>(pastEndOffset - startOffset);

    if (zoomIn) {
        // Zooms as long as the start hasn't reached the past end.
        success = ((startOffset + zoomStep) < (pastEndOffset - zoomStep));
        if (success) {
            // Zooms where the cursor is. We do that by calculating a ratio between how far we are from the left/right limit.
            startOffset += static_cast<int>(static_cast<double>(zoomStep) * shiftRatio);
            pastEndOffset -= static_cast<int>(static_cast<double>(zoomStep) * (1.0 - shiftRatio));
        }
    } else {
        // Zoom out, if possible, where the cursor is.
        startOffset -= static_cast<int>(static_cast<double>(zoomStep) * shiftRatio);
        pastEndOffset += static_cast<int>(static_cast<double>(zoomStep) * (1.0 - shiftRatio));

        startOffset = NumberUtil::correctNumber(startOffset, 0, sampleLength - 1);
        pastEndOffset = NumberUtil::correctNumber(pastEndOffset, 0, sampleLength);
    }

    if (!success) {
        return;
    }

    waveViewerDisplayedData = WaveViewerDisplayedData(
            waveViewerDisplayedData.getMultiplier(),
            startOffset,
            pastEndOffset,
            sampleLength,
            waveViewerDisplayedData.getSampleFrequencyHz(),
            isLooping,
            waveViewerDisplayedData.getDigidrumNote()
    );

    instrumentEditorView->refreshUi(waveViewerDisplayedData);
}

void SampleInstrumentEditorControllerImpl::onUserWantsToScroll(const bool right) noexcept
{
    auto sampleLength = 0;
    songController.performOnConstInstrument(shownInstrumentId.getValue(), [&](const Instrument& instrument) {
        sampleLength = instrument.getConstSamplePart().getSample()->getLength();
    });

    const auto isLooping = waveViewerDisplayedData.isLooping();
    auto startOffset = waveViewerDisplayedData.getStartViewedOffset();
    auto pastEndOffset = waveViewerDisplayedData.getPastEndViewedOffset();
    // Uses the seen part to determine how fast to scroll. A minimum is also needed, as the division can return 0.
    const auto scrollSpeed = std::max(1, static_cast<int>(static_cast<double>(pastEndOffset - startOffset) * scrollSpeedRatio));

    if (!right) {
        // Scroll left, if possible.
        const auto newStartOffset = std::max(0, startOffset - scrollSpeed);

        pastEndOffset -= startOffset - newStartOffset;
        startOffset = newStartOffset;
    } else {
        // Scroll right, if possible.
        const auto newPastEndOffset = std::min(sampleLength, pastEndOffset + scrollSpeed);

        startOffset += newPastEndOffset - pastEndOffset;
        pastEndOffset = newPastEndOffset;
    }

    waveViewerDisplayedData = WaveViewerDisplayedData(
            waveViewerDisplayedData.getMultiplier(),
            startOffset,
            pastEndOffset,
            sampleLength,
            waveViewerDisplayedData.getSampleFrequencyHz(),
            isLooping,
            waveViewerDisplayedData.getDigidrumNote()
    );

    instrumentEditorView->refreshUi(waveViewerDisplayedData);
}

void SampleInstrumentEditorControllerImpl::onUserWantsToScrollTo(const int newStartOffset) noexcept
{
    const auto viewedLength = waveViewerDisplayedData.getPastEndViewedOffset() - waveViewerDisplayedData.getStartViewedOffset();

    waveViewerDisplayedData = WaveViewerDisplayedData(
            waveViewerDisplayedData.getMultiplier(),
            newStartOffset,
            newStartOffset + viewedLength,
            waveViewerDisplayedData.getSampleLength(),
            waveViewerDisplayedData.getSampleFrequencyHz(),
            waveViewerDisplayedData.isLooping(),
            waveViewerDisplayedData.getDigidrumNote()
    );

    instrumentEditorView->refreshUi(waveViewerDisplayedData);
}

void SampleInstrumentEditorControllerImpl::onUserWantsToChangeDigiNote(const int newDigiNote) noexcept
{
    const auto instrumentId = shownInstrumentId;
    if (instrumentId.isAbsent()) {
        jassertfalse;       // Abnormal.
        return;
    }

    waveViewerDisplayedData = WaveViewerDisplayedData(
            waveViewerDisplayedData.getMultiplier(),
            waveViewerDisplayedData.getStartViewedOffset(),
            waveViewerDisplayedData.getPastEndViewedOffset(),
            waveViewerDisplayedData.getSampleLength(),
            waveViewerDisplayedData.getSampleFrequencyHz(),
            waveViewerDisplayedData.isLooping(),
            newDigiNote
    );

    instrumentEditorView->refreshUi(waveViewerDisplayedData);

    songController.setSampleInstrumentMetadata(instrumentId.getValueRef(), {}, {}, {}, {}, newDigiNote);
}

void SampleInstrumentEditorControllerImpl::onUserWantsToChangeSampleFile(const juce::File& fileToLoad) noexcept
{
    if (shownInstrumentId.isAbsent()) {
        jassertfalse;       // Abnormal.
        return;
    }

    // Loads the sample on a background thread.
    auto loadSampleTask = std::make_unique<LoadSampleTask>(fileToLoad.getFullPathName());
    loadSampleBackgroundTask = std::make_unique<BackgroundTask<std::unique_ptr<SampleLoader::Result>>>(*this, std::move(loadSampleTask));
    loadSampleBackgroundTask->performTask();
}

void SampleInstrumentEditorControllerImpl::onCloseDialog() noexcept
{
    dialog.reset();
}


// BackgroundTaskListener method implementations.
// ============================================================

void SampleInstrumentEditorControllerImpl::onBackgroundTaskFinished(TaskOutputState /*taskOutputState*/, const std::unique_ptr<SampleLoader::Result> result) noexcept
{
    // UI thread.

    // Shows a dialog in case of error.
    if (const auto outcome = result->getOutcome(); outcome != SampleLoader::Outcome::success) {
        const auto errorMessage = LoadSampleTask::getErrorMessage(outcome);
        dialog = SuccessOrErrorDialog::buildForError(errorMessage, [&] { onCloseDialog(); });
        return;
    }

    // Sample loaded OK. Modifies the Instrument.
    const auto sampleMemoryBlock = result->getData();
    auto sample = std::make_unique<Sample>(*sampleMemoryBlock);
    songController.setSample(shownInstrumentId.getValueRef(), std::move(sample), result->getSampleFrequencyHz(), result->getOriginalFileName());
}

}   // namespace arkostracker
