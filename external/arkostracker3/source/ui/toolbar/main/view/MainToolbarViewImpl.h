#pragma once

#include <memory>
#include <vector>

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../../../utils/WithParent.h"
#include "../../../components/ButtonWithImage.h"
#include "../../../components/SliderIncDec.h"
#include "MainToolbarView.h"

namespace arkostracker 
{

class MainToolbarController;

/** Implementation of the View of the Main toolbar. Shows the Play/Stop buttons, the layout buttons, and the timer. */
class MainToolbarViewImpl final : public MainToolbarView,
                                  SliderIncDec::Listener
{
public:
    /**
     * Constructor.
     * @param controller the Controller.
     * @param selectedLayoutNumber the index of the selected layout.
     * @param octaveToShow the octave to show.
     * @param speedToShow the speed to show.
     * @param bpmToShow the bpm to show.
     */
    explicit MainToolbarViewImpl(MainToolbarController& controller, int selectedLayoutNumber, int octaveToShow, int speedToShow, int bpmToShow) noexcept;

    // MainToolbarView method implementations.
    // ==========================================
    void setTimerToDisplay(juce::String timer) noexcept override;
    void setCurrentArrangementNumber(int number) noexcept override;
    void setOctave(int octave) noexcept override;
    void setSpeed(int speed, int bpm) noexcept override;
    void setSerialIconState(bool connected, bool enabled) noexcept override;
    void showSerialBubble(const juce::String& message) noexcept override;
    void setHighlightedPlayIcon(MainToolbarView::PlayButton playButtonToHighlight) noexcept override;

    // Component method implementations.
    // ====================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    static const int itemsHeight;

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

    /** A Layout Button. */
    class LayoutButton final : public juce::TextButton,
                               public WithParent<MainToolbarViewImpl>
    {
    public:
        explicit LayoutButton(MainToolbarViewImpl& parent, const juce::String& buttonName, const juce::String& toolTip, int layoutIndex);
        void mouseDown(const juce::MouseEvent& event) override;
        void lookAndFeelChanged() override;

    private:
        void setButtonColors() noexcept;
        const int layoutIndex;
    };

    /**
     * Called when a Layout Button is clicked.
     * @param layoutIndex the index of the Button.
     * @param isLeftButton true if left Button is clicked, false if right (or other).
     */
    void onLayoutButtonClicked(int layoutIndex, bool isLeftButton) const noexcept;

    /**
     * Asks to restore an arrangement. Also selects the related button.
     * @param arrangementIndex the arrangement index.
     */
    void restoreArrangement(int arrangementIndex) const noexcept;

    /**
     * Highlights the button from the given arrangement index.
     * @param arrangementIndex the arrangement index.
     */
    void highlightButton(int arrangementIndex) const noexcept;

    /** Called when the Duration label is clicked. */
    void onClickOnDurationLabel() const noexcept;

    /** Called when one of the Play Song From Start Button is clicked. */
    void onPlaySongFromStartButtonClicked() const noexcept;
    /** Called when one of the Play Song Button is clicked. */
    void onPlaySongButtonClicked() const noexcept;
    /** Called when one of the Play Pattern From Start Button is clicked. */
    void onPlayPatternFromStartButtonClicked() const noexcept;
    /** Called when one of the Play Pattern Button is clicked. */
    void onPlayPatternButtonClicked() const noexcept;
    /** Called when one of the Stop Button is clicked. */
    void onStopButtonClicked() const noexcept;

    /** Called when the Serial Button is clicked. */
    void onSerialButtonClicked() const noexcept;

    /**
     * Shows the given speed.
     * @param speed the speed to show.
     * @param bpm the bpm to show, or 0.
     */
    void showSpeed(int speed, int bpm) noexcept;

    /** Custom Mouse Listener on the Label. */
    class LabelMouseListener final : public juce::MouseListener,
                                     WithParent<MainToolbarViewImpl>
    {
    public:
        explicit LabelMouseListener(MainToolbarViewImpl& parent) :
                WithParent(parent)
        {
        }
        void mouseDown(const juce::MouseEvent& /*event*/) override
        {
            parentObject.onClickOnDurationLabel();
        }
    };

    MainToolbarController& controller;

    ButtonWithImage playSongFromStartButton;
    ButtonWithImage playSongButton;
    ButtonWithImage playPatternFromStartButton;
    ButtonWithImage playPatternButton;
    ButtonWithImage stopButton;
    std::vector<std::unique_ptr<LayoutButton>> layoutButtons;   // The Layout buttons.
    SliderIncDec octaveSlider;
    juce::Label speedLabel;
    juce::Label speedNumberLabel;
    ButtonWithImage serialButton;
    juce::Label durationLabel;
    LabelMouseListener durationLabelMouseListener;

    std::unique_ptr<juce::BubbleMessageComponent> serialBubble;                         // Shows serial errors.

    int cachedSpeed;
    int cachedBpm;
};

}   // namespace arkostracker
