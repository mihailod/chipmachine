#pragma once

#include "../expressionTableList/ExpressionTableListPanel.h"
#include "../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker 
{

/** A panel to show the Arpeggio table list Panel. */
class ArpeggioTableListPanel : public ExpressionTableListPanel
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    ArpeggioTableListPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
};

}   // namespace arkostracker

