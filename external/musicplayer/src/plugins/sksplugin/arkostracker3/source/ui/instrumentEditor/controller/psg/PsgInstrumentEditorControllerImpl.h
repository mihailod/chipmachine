#pragma once

#include "../../../../controllers/observers/SongPlayerObserver.h"
#include "../../../editorWithBars/controller/EditorWithBarsControllerImpl.h"

namespace arkostracker
{

/**
 * Implementation of the controller for PSG Instrument Editor. Doesn't do much, the behavior is done by the Specific Controller,
 * but it is handy as it provides the Factory to create the latter.
 */
class PsgInstrumentEditorControllerImpl final : public EditorWithBarsControllerImpl,
                                                public SongPlayerObserver
{
public:
    /** Factory to create the specific controller. */
    class Factory final : public EditorWithBarsControllerSpecific::Factory
    {
    public:
        std::unique_ptr<EditorWithBarsControllerSpecific> buildSpecificController(MainController& mainController, EditorWithBarsController& editorController) noexcept override;
    };

    /**
     * Constructor.
     * @param mainController the main controller.
     */
    explicit PsgInstrumentEditorControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~PsgInstrumentEditorControllerImpl() override;

    // EditorWithBarsControllerImpl method implementations.
    // =======================================================
    bool showShift() const noexcept override;
    bool showIsLooping() const noexcept override;
    bool showIsRetrig() const noexcept override;
    bool canHideRows() const noexcept override;
    bool canHideRow(AreaType areaType) const noexcept override;

    // SongPlayerObserver method implementations.
    // =======================================================
    void onInstrumentPlayed(const InstrumentPlayedInfo& info) noexcept override;

private:
    MainController& localMainController;
};
} // namespace arkostracker
