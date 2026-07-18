#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../components/GroupWithToggles.h"
#include "../../../components/SliderIncDec.h"
#include "../../controller/toolbox/ToolboxController.h"

namespace arkostracker
{

class SongController;

/** The PatternViewer Toolbox. */
class Toolbox final : public juce::Component,
                      public SliderIncDec::Listener
{
public:
    static constexpr auto desiredWidth = 200;
    static constexpr auto desiredHeight = 400;

    /**
     * Constructor.
     * @param toolboxController the controller.
     */
    explicit Toolbox(ToolboxController& toolboxController) noexcept;

    /**
     * Updates the UI. Nothing happens if the same.
     * @param displayedData the data to display.
     * @param forceViewCreation true to force the view creation. This is useful as a security when creating the UI the first time.
     */
    void updateUi(const ToolboxController::DisplayedData& displayedData, bool forceViewCreation = false) noexcept;

    // Component method implementations.
    // ==================================
    void paint(juce::Graphics& g) override;
    void focusOfChildComponentChanged(FocusChangeType cause) override;
    void resized() override;

    // SliderIncDec::Listener method implementations.
    // ===============================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    static constexpr auto offsetComboBoxId = 1;

    /** Called when the tool type Combobox selected item changed. */
    void onToolTypeComboBoxChanged() const noexcept;

    /**
     * Creates the views from the tool type, deletes the possible previous ones. This may resize the toolbox.
     * @param toolType the tool type.
     */
    void createViewsFromToolType(ToolboxController::ToolType toolType) noexcept;

    /** Creates the Views for Transpose. */
    void createViewsForTranspose() noexcept;

    /** Creates the Views for a simple action (with no special buttons). */
    void createViewsForSimpleAction() noexcept;

    /** Creates the Views for a Swap Cells. */
    void createViewsForSwapCells() noexcept;

    /** Performs the action. */
    void onPerformClicked() const noexcept;

    /** @return the toggle name for the selection, from its index. */
    static juce::String getSelectionTargetToggleName(int index) noexcept;

    /** @return true if the toggle name for the selection is selected, from its index. */
    bool isSelectionTargetSelected(int index) const noexcept;

    /** Called when the selection target changes. Notifies the controller. The UI has already been changed. */
    void onSelectionTargetChanged() const noexcept;
    /** Called when the first channel changes. Notifies the controller. The UI has already been changed. */
    void onFirstChannelToSwapChanged() const noexcept;
    /** Called when the second channel changes. Notifies the controller. The UI has already been changed. */
    void onSecondChannelToSwapChanged() const noexcept;

    /** Sets the height of the toolbox. */
    void setHeight(int height) noexcept;

    /** @return the available width, without margins. */
    int getAvailableWidth() const noexcept;
    /** Locates and resizes the Selection group and Perform Button after the given Component. */
    void locateSelectionAndPerformButton(const Component& afterComponent) noexcept;

    /**
     * Shows or hides some views. This only changes the visibility, not the location.
     * @param showTranspositionsViews true if visible.
     * @param showInstrumentSelectedLabel true if visible.
     * @param showSwapChannelsComboBoxes true if both the swap channels combo boxes are visible.
     */
    void showViews(bool showTranspositionsViews, bool showInstrumentSelectedLabel = true, bool showSwapChannelsComboBoxes = false) noexcept;

    /**
     * Rebuilds, if needed, the combo boxes, and selects the new channels.
     * @param channelCount the channel count.
     * @param firstChannelIndex the index of the first channel.
     * @param secondChannelIndex the index of the second channel.
     */
    void buildSwapChannelComboBoxIfNeeded(int channelCount, int firstChannelIndex, int secondChannelIndex) noexcept;

    ToolboxController::DisplayedData displayedData;

    const int margins;
    const int left;
    ToolboxController& toolboxController;
    bool hasFocus;

    juce::ComboBox toolTypeComboBox;
    juce::Label selectedInstrumentsLabel;
    GroupWithToggles selectionGroup;

    juce::TextButton performButton;

    SliderIncDec transposePlusMinus;
    juce::TextButton transposePlus1;
    juce::TextButton transposeMinus1;
    juce::TextButton transposePlus12;
    juce::TextButton transposeMinus12;

    juce::ComboBox firstChannelToSwapComboBox;
    juce::ComboBox secondChannelToSwapComboBox;
};

}    // namespace arkostracker
