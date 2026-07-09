#pragma once

#include "../../editorWithBars/controller/EditorWithBarsControllerImpl.h"

namespace arkostracker 
{

/**
 * Implementation of the controller for Arpeggio Table Editor. Doesn't do much, the behavior is done by the Specific Controller,
 * but it is handy as it provides the Factory to create the latter.
 */
class ArpeggioTableEditorControllerImpl final : public EditorWithBarsControllerImpl
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
    explicit ArpeggioTableEditorControllerImpl(MainController& mainController) noexcept;

    // EditorWithBarsControllerImpl method implementations.
    // =======================================================
    bool showShift() const noexcept override;
    bool showIsLooping() const noexcept override;
    bool showIsRetrig() const noexcept override;
    bool canHideRows() const noexcept override;
    bool canHideRow(AreaType areaType) const noexcept override;
};

}   // namespace arkostracker
