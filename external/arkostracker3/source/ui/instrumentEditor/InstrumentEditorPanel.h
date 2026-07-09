#pragma once

#include "../containerArranger/Panel.h"

namespace arkostracker
{

class MainController;
class InstrumentEditorController;

/** A panel to show the Instrument Editor. */
class InstrumentEditorPanel final : public Panel,
                                    public juce::DragAndDropContainer
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    InstrumentEditorPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~InstrumentEditorPanel() override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ================================
    void resized() override;

private:
    InstrumentEditorController& instrumentEditorController;
};

}   // namespace arkostracker
