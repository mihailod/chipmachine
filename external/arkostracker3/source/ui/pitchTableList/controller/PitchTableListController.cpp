#include "PitchTableListController.h"

namespace arkostracker 
{

PitchTableListController::PitchTableListController(MainController& pMainController) noexcept :
        ExpressionTableListControllerImpl(pMainController, false)
{
}

PanelType PitchTableListController::getRelatedPanelToOpen() const noexcept
{
    return PanelType::pitchTable;
}

}   // namespace arkostracker
