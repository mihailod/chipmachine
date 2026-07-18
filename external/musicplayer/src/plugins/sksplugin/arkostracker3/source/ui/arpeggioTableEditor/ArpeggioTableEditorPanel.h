#pragma once

#include "../containerArranger/Panel.h"

namespace arkostracker 
{

class MainController;
class EditorWithBarsController;

/** A panel to show the Linker. */
class ArpeggioTableEditorPanel : public Panel,
                                 public juce::DragAndDropContainer
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    ArpeggioTableEditorPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~ArpeggioTableEditorPanel() override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ===================================
    void resized() override;

private:
    EditorWithBarsController& arpeggioTableEditorController;
};

}   // namespace arkostracker

