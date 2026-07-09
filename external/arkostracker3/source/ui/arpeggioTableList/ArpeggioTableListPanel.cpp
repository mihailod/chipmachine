#include "ArpeggioTableListPanel.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

ArpeggioTableListPanel::ArpeggioTableListPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        ExpressionTableListPanel(true, mainController, pListener)
{
}


// Panel method implementations.
// ================================

PanelType ArpeggioTableListPanel::getType() const noexcept
{
    return PanelType::arpeggioList;
}


}   // namespace arkostracker

