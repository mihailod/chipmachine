#pragma once

#include "../../components/ButtonWithImage.h"
#include "TestAreaView.h"
#include "keyboard/KeyboardView.h"

namespace arkostracker 
{

class TestAreaController;

/** Implementation of the Test Area View. */
class TestAreaViewImpl final : public TestAreaView
{
public:
    /**
     * Constructor.
     * @param controller the Controller.
     * @param displayedData the data to display.
     */
    TestAreaViewImpl(TestAreaController& controller, const DisplayedData& displayedData) noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

    // TestAreaView method implementations.
    // =======================================
    void setDisplayedData(const DisplayedData& displayedData) noexcept override;

private:
    static const int activityIndicatorFadeOutSpeedMs;
    static const float activityIndicatorFadeInAlpha;

    /**
     * Updates the UI from the given data.
     * @param displayedData the data to display.
     */
    void updateUi(const DisplayedData& displayedData) noexcept;

    /** Called when the Arpeggio button is clicked. */
    void onArpeggioButtonClicked() const noexcept;
    /** Called when the Pitch button is clicked. */
    void onPitchButtonClicked() const noexcept;

    /** Called when the Test button is clicked. */
    void onTestButtonClicked() const noexcept;
    /** Called when the mono/poly button is clicked. */
    void onMonoButtonClicked() const noexcept;

    /** Called when the Edit button is clicked. */
    void onEditButtonClicked() const noexcept;

    /**
     * Called when a keyboard key is pressed. The UI has not been refreshed yet.
     * @param noteNumber the note number.
     */
    void onKeyboardKeyPressed(int noteNumber) const noexcept;

    /**
     * Sets the index in one of the expression.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param index the index.
     * @param isExpressionUsed true if the expression is used in the replay of the note.
     */
    void setExpressionIndex(bool isArpeggio, int index, bool isExpressionUsed) noexcept;

    TestAreaController& controller;

    ButtonWithImage useSelectedArpButton;
    ButtonWithImage useSelectedPitchButton;
    juce::Label usedArpeggioIndexLabel;
    juce::Label usedPitchIndexLabel;
    juce::TextButton testButton;
    juce::TextButton monophonicButton;                  // Displays "mono" or "poly".
    ButtonWithImage editButton;                         // To edit the mono/poly settings.
    KeyboardView keyboardView;

    juce::ImageComponent activityIndicatorImage;
    juce::ComponentAnimator& animator;

};

}   // namespace arkostracker
