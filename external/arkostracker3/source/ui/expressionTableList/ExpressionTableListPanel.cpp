#include "ExpressionTableListPanel.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

ExpressionTableListPanel::ExpressionTableListPanel(const bool isArpeggio, MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        expressionTableListController(mainController.getExpressionTableListController(isArpeggio))
{
    expressionTableListController.onParentViewCreated(*this);
}

ExpressionTableListPanel::~ExpressionTableListPanel()
{
    // Tells the Controller that its UI does not exist anymore.
    expressionTableListController.onParentViewDeleted();
}

void ExpressionTableListPanel::resized()
{
    expressionTableListController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

void ExpressionTableListPanel::getKeyboardFocus() noexcept
{
    expressionTableListController.getKeyboardFocus();
}

}   // namespace arkostracker
