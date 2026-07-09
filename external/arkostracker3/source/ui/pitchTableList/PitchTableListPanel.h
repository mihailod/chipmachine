#pragma once

#include "../containerArranger/Panel.h"

#include "../expressionTableList/ExpressionTableListPanel.h"
#include "../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker 
{

/** A panel to show the Pitch table list Panel. */
class PitchTableListPanel : public ExpressionTableListPanel
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    PitchTableListPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
};


}   // namespace arkostracker

