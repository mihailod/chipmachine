#include "ArpeggioTableEditorPanel.h"

#include "../../controllers/MainController.h"
#include "controller/ArpeggioTableEditorControllerImpl.h"

namespace arkostracker 
{

ArpeggioTableEditorPanel::ArpeggioTableEditorPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        arpeggioTableEditorController(mainController.getArpeggioTableEditorController())
{
    arpeggioTableEditorController.onParentViewCreated(*this);
}

ArpeggioTableEditorPanel::~ArpeggioTableEditorPanel()
{
    arpeggioTableEditorController.onParentViewDeleted();
}

PanelType ArpeggioTableEditorPanel::getType() const noexcept
{
    return PanelType::arpeggioTable;
}

void ArpeggioTableEditorPanel::resized()
{
    arpeggioTableEditorController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

void ArpeggioTableEditorPanel::getKeyboardFocus() noexcept
{
    arpeggioTableEditorController.getKeyboardFocus();
}


}   // namespace arkostracker

