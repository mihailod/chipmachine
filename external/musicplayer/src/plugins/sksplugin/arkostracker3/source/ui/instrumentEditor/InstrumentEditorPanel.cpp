#include "InstrumentEditorPanel.h"

#include "../../controllers/MainController.h"
#include "controller/InstrumentEditorController.h"

namespace arkostracker 
{

InstrumentEditorPanel::InstrumentEditorPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        instrumentEditorController(mainController.getInstrumentEditorController())
{
    instrumentEditorController.onParentViewCreated(*this);
}

InstrumentEditorPanel::~InstrumentEditorPanel()
{
    // Tells the Controller that its UI does not exist anymore.
    instrumentEditorController.onParentViewDeleted();
}

PanelType InstrumentEditorPanel::getType() const noexcept
{
    return PanelType::instrumentEditor;
}

void InstrumentEditorPanel::getKeyboardFocus() noexcept
{
    instrumentEditorController.getKeyboardFocus();
}

void InstrumentEditorPanel::resized()
{
    instrumentEditorController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

}   // namespace arkostracker
