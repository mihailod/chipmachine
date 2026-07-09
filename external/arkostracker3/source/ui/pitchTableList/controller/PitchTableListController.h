#pragma once

#include "../../expressionTableList/controller/ExpressionTableListControllerImpl.h"

namespace arkostracker 
{

/** The Pitch Table List Controller implementation. */
class PitchTableListController final : public ExpressionTableListControllerImpl
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit PitchTableListController(MainController& mainController) noexcept;

protected:
    PanelType getRelatedPanelToOpen() const noexcept override;
};

}   // namespace arkostracker
