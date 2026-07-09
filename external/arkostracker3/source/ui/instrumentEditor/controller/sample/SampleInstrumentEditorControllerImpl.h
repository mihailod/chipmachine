#pragma once

#include "../../../../business/instrument/SampleLoader.h"
#include "../../../../controllers/observers/InstrumentChangeObserver.h"
#include "../../../components/dialogs/ModalDialog.h"
#include "../../../utils/backgroundTask/BackgroundTask.h"
#include "../../view/sample/SampleInstrumentEditorView.h"
#include "SampleInstrumentEditorController.h"

namespace arkostracker 
{

class SongController;

/** Controller implementation of the Sample Instrument Editor. */
class SampleInstrumentEditorControllerImpl final : public SampleInstrumentEditorController,
                                                   public InstrumentChangeObserver,
                                                   public BackgroundTaskListener<std::unique_ptr<SampleLoader::Result>>
{
public:
    /** Constructor. */
    explicit SampleInstrumentEditorControllerImpl(SongController& songController) noexcept;

    /** Destructor. */
    ~SampleInstrumentEditorControllerImpl() override;

    // InstrumentChangeObserver method implementations.
    // ===================================================
    void onInstrumentChanged(const Id& id, unsigned int whatChanged) override;
    void onPsgInstrumentCellChanged(const Id& id, int cellIndex, bool mustRefreshAllAfterToo) override;
    void onInstrumentsInvalidated() override;

    // TypedInstrumentEditorController method implementations.
    // ==========================================================
    void onNewItemSelected(const Id& instrumentId, bool force) noexcept override;
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void getKeyboardFocus() noexcept override;

    // SampleInstrumentEditorController method implementations.
    // ==========================================================
    void onUserantsToChangedMultiplier(double multiplier) noexcept override;
    void onUserWantsToSetNewLoop(OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLooping) noexcept override;
    void onUserWantsToZoom(bool zoomIn, int clickedOffset) noexcept override;
    void onUserWantsToScroll(bool right) noexcept override;
    void onUserWantsToScrollTo(int newStartOffset) noexcept override;
    void onUserWantsToChangeDigiNote(int newDigiNote) noexcept override;
    void onUserWantsToChangeSampleFile(const juce::File &fileToLoad) noexcept override;

    // BackgroundTaskListener method implementations.
    // ============================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SampleLoader::Result> result) noexcept override;

private:
    static const double zoomRatio;
    static const double minZoomStep;
    static const double scrollSpeedRatio;
    static const double sampleAmplificationStep;

    /**
     * Called when the selected instrument has changed. This will set the internally stored id.
     * @param newInstrumentId the new instrument id.
     */
    void onSelectedItemChangedMustRefreshUi(const Id& newInstrumentId) noexcept;

    /** Refresh all the UI from the current instrument. */
    void refreshAllUi() noexcept;

    /** Closes the possible dialog. */
    void onCloseDialog() noexcept;

    SongController& songController;
    OptionalId shownInstrumentId;
    std::unique_ptr<SampleInstrumentEditorView> instrumentEditorView;

    WaveViewerDisplayedData waveViewerDisplayedData;

    std::unique_ptr<BackgroundTask<std::unique_ptr<SampleLoader::Result>>> loadSampleBackgroundTask;
    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker
