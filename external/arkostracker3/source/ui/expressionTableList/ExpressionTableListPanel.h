#pragma once

#include "../containerArranger/Panel.h"
#include "../lists/controller/ListController.h"

namespace arkostracker 
{

class MainController;

/** A generic Expression List Panel, to be used by Arpeggios and Pitches. */
class ExpressionTableListPanel : public Panel,
                                 public juce::DragAndDropContainer
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    ExpressionTableListPanel(bool isArpeggio, MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~ExpressionTableListPanel() override;

    // Component method implementations.
    // ===================================
    void resized() override;
    void getKeyboardFocus() noexcept override;

private:
    ListController& expressionTableListController;
};

}   // namespace arkostracker

