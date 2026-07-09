#pragma once

#include <memory>

#include "../containerArranger/Panel.h"
#include "../containerArranger/PanelType.h"

namespace arkostracker 
{

class MainController;

/** Provides the factory to instantiate the Panels. */
class PanelFactory
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller to transmit to all the panels.
     * @param listener the listener to the Panel events.
     */
    PanelFactory(MainController& mainController, Panel::Listener& listener) noexcept;

    /**
     * Creates a Panel instance, of the desired type.
     * @param panelType the type of the Panel.
     * @return a new Panel instance.
     */
    std::unique_ptr<Panel> createPanel(PanelType panelType) const noexcept;

private:
    MainController& mainController;                     // The Main Controller to transmit to all the panels.
    Panel::Listener& panelListener;
};

}   // namespace arkostracker
