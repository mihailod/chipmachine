#pragma once

#include "../containerArranger/Panel.h"

namespace arkostracker 
{

class MainController;
class EditorWithBarsController;

/** A panel to show the Pitch Table Editor. */
class PitchTableEditorPanel : public Panel,
                              public juce::DragAndDropContainer
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    PitchTableEditorPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~PitchTableEditorPanel() override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;

    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ===================================
    void resized() override;

private:
    EditorWithBarsController& pitchTableEditorController;
};

}   // namespace arkostracker

