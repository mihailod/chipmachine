#pragma once

#include "../../expressionTableList/controller/ExpressionTableListControllerImpl.h"

namespace arkostracker 
{

/** The Arpeggio Table List Controller implementation. */
class ArpeggioTableListController : public ExpressionTableListControllerImpl
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit ArpeggioTableListController(MainController& mainController) noexcept;

protected:
    PanelType getRelatedPanelToOpen() const noexcept override;
};

}   // namespace arkostracker

