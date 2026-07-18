#include "InstrumentListPanel.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

InstrumentListPanel::InstrumentListPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        instrumentTableListController(mainController.getInstrumentTableListController())
{
    instrumentTableListController.onParentViewCreated(*this);
}

InstrumentListPanel::~InstrumentListPanel()
{
    // Tells the Controller that its UI does not exist anymore.
    instrumentTableListController.onParentViewDeleted();
}

void InstrumentListPanel::resized()
{
    instrumentTableListController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

PanelType InstrumentListPanel::getType() const noexcept
{
    return PanelType::instrumentList;
}

void InstrumentListPanel::getKeyboardFocus() noexcept
{
    instrumentTableListController.getKeyboardFocus();
}

}   // namespace arkostracker
