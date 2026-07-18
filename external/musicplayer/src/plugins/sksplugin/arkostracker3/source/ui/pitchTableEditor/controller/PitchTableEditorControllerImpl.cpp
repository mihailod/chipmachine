#include "PitchTableEditorControllerImpl.h"

#include "PitchTableEditorControllerSpecificImpl.h"

namespace arkostracker 
{

PitchTableEditorControllerImpl::PitchTableEditorControllerImpl(MainController& mainController) noexcept :
        EditorWithBarsControllerImpl(mainController, *std::make_unique<Factory>(), AreaType::primaryPitch)
{
}


// EditorWithBarsControllerImpl method implementations.
// =======================================================

bool PitchTableEditorControllerImpl::canHideRows() const noexcept
{
    return false;
}

bool PitchTableEditorControllerImpl::showShift() const noexcept
{
    return true;
}

bool PitchTableEditorControllerImpl::showIsLooping() const noexcept
{
    return false;
}

bool PitchTableEditorControllerImpl::showIsRetrig() const noexcept
{
    return false;
}

bool PitchTableEditorControllerImpl::canHideRow(AreaType /*areaType*/) const noexcept
{
    return false;
}


// Factory method implementations.
// ==================================

std::unique_ptr<EditorWithBarsControllerSpecific> PitchTableEditorControllerImpl::Factory::buildSpecificController(
    MainController& mainController, EditorWithBarsController& editorController) noexcept
{
    return std::make_unique<PitchTableEditorControllerSpecificImpl>(mainController, editorController);
}

}   // namespace arkostracker
