#pragma once

#include "InstrumentEditorController.h"
#include "ItemEditorController.h"
#include "../../../controllers/observers/SelectedInstrumentIndexObserver.h"

namespace arkostracker 
{

class MainController;
class SongController;

/**
 * Implementation of the Instrument Editor Controller.
 * It holds one instance of each Controller, lazily, one per possible type of instrument (Psg, sample, etc.).
 * One of them is used at one time. Their View is destroyed when the controller instance changes, else it is only refreshed.
 */
class InstrumentEditorControllerImpl final : public InstrumentEditorController,
                                             public SelectedInstrumentIndexObserver
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit InstrumentEditorControllerImpl(MainController& mainController) noexcept;

    // SelectedInstrumentIndexObserver method implementations.
    // ==========================================================
    void onSelectedInstrumentChanged(const OptionalId& selectedInstrumentId, bool forceSingleSelection) override;

    // InstrumentEditorController method implementations.
    // ======================================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void getKeyboardFocus() noexcept override;

private:
    MainController& mainController;
    SongController& songController;

    std::unique_ptr<ItemEditorController> noOpInstrumentEditorController;             // Used on startup, nothing is selected.
    std::unique_ptr<ItemEditorController> noInstrumentEditorController;               // Used when instrument 0 is selected.
    std::unique_ptr<ItemEditorController> psgInstrumentEditorController;              // Lazily constructed PSG editor.
    std::unique_ptr<ItemEditorController> sampleInstrumentEditorController;           // Lazily constructed Sample editor.

    ItemEditorController* currentInstrumentEditorController;
    BoundedComponent* parentView;
};

}   // namespace arkostracker
