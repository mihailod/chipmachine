#include "PitchTableListPanel.h"

#include "../../controllers/MainController.h"
#include "../containerArranger/PanelSizes.h"

namespace arkostracker 
{

PitchTableListPanel::PitchTableListPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        ExpressionTableListPanel(false, mainController, pListener)
{
}


// Panel method implementations.
// ================================

PanelType PitchTableListPanel::getType() const noexcept
{
    return PanelType::pitchList;
}


}   // namespace arkostracker

