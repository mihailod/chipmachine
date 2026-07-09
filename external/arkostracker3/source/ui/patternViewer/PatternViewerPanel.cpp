#include "PatternViewerPanel.h"

#include "../../controllers/MainController.h"
#include "controller/PatternViewerController.h"

namespace arkostracker 
{

PatternViewerPanel::PatternViewerPanel(MainController& pMainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        mainController(pMainController),
        patternViewController(pMainController.getPatternViewerControllerInstance())
{
    patternViewController.onParentViewCreated(*this);
    mainController.observers().getPatternViewerMetadataObservers().addObserver(this);
}

PatternViewerPanel::~PatternViewerPanel() noexcept
{
    mainController.observers().getPatternViewerMetadataObservers().removeObserver(this);
    patternViewController.onParentViewDeleted();
}

PanelType PatternViewerPanel::getType() const noexcept
{
    return PanelType::patternViewer;
}

void PatternViewerPanel::getKeyboardFocus() noexcept
{
    patternViewController.getKeyboardFocus();
}

void PatternViewerPanel::resized()
{
    patternViewController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}


// PatternViewerMetadataObserver method implementations.
// ========================================================

void PatternViewerPanel::onPatternViewerMetadataChanged()
{
    // Nothing to do.
}

void PatternViewerPanel::onRecordStateChanged(bool /*newIsRecordingState*/)
{
    repaint();
}

void PatternViewerPanel::drawBorder(juce::Graphics& g, const bool isFocused) noexcept
{
    // Special border when recording.
    const auto isRecording = patternViewController.isRecording();

    juce::Colour borderColor;
    if (isRecording) {
        borderColor = juce::Colour(isFocused ? 0xffff0000 : 0xff800000);
    } else {
        borderColor = findColour(static_cast<int>(isFocused ? LookAndFeelConstants::Colors::panelBorderFocused : LookAndFeelConstants::Colors::panelBorderUnfocused));
    }

    g.setColour(borderColor);
    g.drawRect(0, 0, getWidth(), getHeight());
}

}   // namespace arkostracker
