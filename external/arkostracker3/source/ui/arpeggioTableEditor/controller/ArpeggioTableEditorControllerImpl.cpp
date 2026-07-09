#include "ArpeggioTableEditorControllerImpl.h"

#include "ArpeggioTableEditorControllerSpecificImpl.h"

namespace arkostracker 
{

ArpeggioTableEditorControllerImpl::ArpeggioTableEditorControllerImpl(MainController& mainController) noexcept :
        EditorWithBarsControllerImpl(mainController, *std::make_unique<Factory>(), AreaType::primaryArpeggioOctave)
{
}


// EditorWithBarsControllerImpl method implementations.
// =======================================================

bool ArpeggioTableEditorControllerImpl::canHideRows() const noexcept
{
    return false;
}

bool ArpeggioTableEditorControllerImpl::showShift() const noexcept
{
    return true;
}

bool ArpeggioTableEditorControllerImpl::showIsLooping() const noexcept
{
    return false;
}

bool ArpeggioTableEditorControllerImpl::showIsRetrig() const noexcept
{
    return false;
}

bool ArpeggioTableEditorControllerImpl::canHideRow(AreaType /*areaType*/) const noexcept
{
    return false;
}


// Factory method implementations.
// ==================================

std::unique_ptr<EditorWithBarsControllerSpecific> ArpeggioTableEditorControllerImpl::Factory::buildSpecificController(
    MainController& mainController, EditorWithBarsController& editorController) noexcept
{
    return std::make_unique<ArpeggioTableEditorControllerSpecificImpl>(mainController, editorController);
}

}   // namespace arkostracker
