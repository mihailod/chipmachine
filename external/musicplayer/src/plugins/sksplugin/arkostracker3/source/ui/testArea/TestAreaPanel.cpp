#include "TestAreaPanel.h"

#include "../../controllers/MainController.h"
#include "controller/TestAreaController.h"

namespace arkostracker 
{

TestAreaPanel::TestAreaPanel(MainController& mainController, Listener& pListener) noexcept :
        Panel(pListener),
        controller(mainController.getTestAreaControllerInstance())
{
    controller.onParentViewCreated(*this);
}

TestAreaPanel::~TestAreaPanel() noexcept
{
    controller.onParentViewDeleted();
}

PanelType TestAreaPanel::getType() const noexcept
{
    return PanelType::testArea;
}

void TestAreaPanel::resized()
{
    controller.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

void TestAreaPanel::getKeyboardFocus() noexcept
{
    controller.getKeyboardFocus();
}

}   // namespace arkostracker
