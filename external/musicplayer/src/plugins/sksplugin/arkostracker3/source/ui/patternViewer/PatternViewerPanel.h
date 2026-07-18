#pragma once

#include "../../controllers/observers/PatternViewerMetadataObserver.h"
#include "../containerArranger/Panel.h"

namespace arkostracker 
{

class PatternViewerController;
class MainController;

/** A panel to show the Pattern Viewer. */
class PatternViewerPanel final : public Panel,
                                 public PatternViewerMetadataObserver
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    PatternViewerPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~PatternViewerPanel() noexcept override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ===================================
    void resized() override;

    // PatternViewerMetadataObserver method implementations.
    // ========================================================
    void onPatternViewerMetadataChanged() override;
    void onRecordStateChanged(bool newIsRecordingState) override;

protected:
    void drawBorder(juce::Graphics& g, bool hasFocus) noexcept override;

private:
    MainController& mainController;
    PatternViewerController& patternViewController;
};

}   // namespace arkostracker

