#include "LinkerPanel.h"

#include "controller/LinkerController.h"
#include "../../controllers/MainController.h"

namespace arkostracker 
{

LinkerPanel::LinkerPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        linkerController(mainController.getLinkerControllerInstance()),
        hasBeenSized(false)
{
    linkerController.onParentViewCreated(*this);
}

LinkerPanel::~LinkerPanel()
{
    // Tells the LinkerController that its UI does not exist anymore.
    linkerController.onParentViewDeleted();
}

PanelType LinkerPanel::getType() const noexcept
{
    return PanelType::linker;
}

void LinkerPanel::resized()
{
    linkerController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());

    if (!hasBeenSized) {
        linkerController.onParentViewFirstResize(*this);
        hasBeenSized = true;
    }
}

void LinkerPanel::getKeyboardFocus() noexcept
{
    linkerController.getKeyboardFocus();
}


}   // namespace arkostracker

