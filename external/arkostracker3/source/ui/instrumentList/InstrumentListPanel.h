#pragma once

#include "../containerArranger/Panel.h"
#include "../lists/controller/ListController.h"

namespace arkostracker 
{

class MainController;

/** A panel to show the Instrument List. */
class InstrumentListPanel : public Panel,
                            public juce::DragAndDropContainer
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    InstrumentListPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~InstrumentListPanel() override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ================================
    void resized() override;

private:
    ListController& instrumentTableListController;
};

}   // namespace arkostracker

