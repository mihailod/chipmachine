#pragma once

#include <unordered_map>

#include "../../../song/cells/Effect.h"
#include "../../components/GroupWithViewport.h"
#include "../../components/SliderIncDec.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class PreferencesManager;
class MainController;

/** Dialog to set up of the Pattern Viewer. */
class PatternViewerSetupDialog final : public ModalDialog,
                                       public SliderIncDec::Listener
{
public:
    /**
     * Constructor.
     * @param mainController to sent event.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
     */
    PatternViewerSetupDialog(MainController& mainController, std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    static const int minimumFontSize;
    static const int maximumFontSize;
    static const int minimumMouseWheelSteps;
    static const int maximumMouseWheelSteps;

    static const juce::String propertyEffect;

    /** Called when the "show hex" toggle has been switched. */
    void onToggledChanged() noexcept;
    /**
     * The user entered a new font size. It may be invalid! This must update the UI.
     * @param desiredFontSize the typed font size.
     */
    void onUserWantsNewFontSize(int desiredFontSize) noexcept;
    /**
     * The user entered new mouse wheel steps. It may be invalid! This must update the UI.
     * @param desiredSteps the desired steps.
     */
    void onUserWantsNewMouseWheelSteps(int desiredSteps) noexcept;

    /** Notifies the change of data to the listeners (typically, the Pattern Viewer). */
    void notifyChange() const noexcept;

    /**
     * Displays the effects in the group.
     * @param effectToChar map linking effects to their char.
     */
    void displayEffects(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept;

    /**
     * Fills the given ComboBox with letters and selects the right one from the effect.
     * @param comboBox the ComboBox linked to this effect.
     * @param label the label linked to this effect.
     * @param effect the effect the ComboBox is related to.
     * @param effectToChar the effect to char map.
     */
    static void fillComboBox(juce::ComboBox& comboBox, juce::Label& label, Effect effect, const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept;

    /** Called when the OK button is clicked. Saves the data and notifies the UI. */
    void onOkButtonClicked() const noexcept;

    /** Called when an effect ComboBox has changed. Duplicates may be found. */
    void onComboBoxChanged() noexcept;

    /** Checks if there are errors in the selected comboBoxes. This uses their data. Updates the UI (colors, OK button). */
    void checkErrorsAndUpdateColors() noexcept;

    /** Called when the Reset To Default button is clicked. */
    void onResetToDefaultClicked() noexcept;

    MainController& mainController;

    std::function<void()> okCallback;

    juce::ToggleButton toggleShowHexadecimal;
    juce::ToggleButton toggleZoomMiddleLine;

    SliderIncDec textSizeSlider;
    SliderIncDec mouseWheelStepsSlider;

    GroupWithViewport group;
    std::vector<std::unique_ptr<juce::Label>> effectNameLabels;
    std::vector<std::unique_ptr<juce::ComboBox>> effectComboBoxes;
    juce::Colour originalLabelsColor;
    juce::TextButton resetToDefaultButton;

    PreferencesManager& preferences;

    bool changed;
};

}   // namespace arkostracker
