#include "ArpeggioTableListController.h"

namespace arkostracker 
{

ArpeggioTableListController::ArpeggioTableListController(MainController& pMainController) noexcept :
        ExpressionTableListControllerImpl(pMainController, true)
{
}

PanelType ArpeggioTableListController::getRelatedPanelToOpen() const noexcept
{
    return PanelType::arpeggioTable;
}


}   // namespace arkostracker

