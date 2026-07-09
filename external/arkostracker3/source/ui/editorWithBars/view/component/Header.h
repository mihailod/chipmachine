#pragma once

#include "../../../components/BlankComponent.h"
#include "../../../components/SliderIncDec.h"

namespace arkostracker 
{

class HeaderController;

/**
 * Header at the top of the Editor With Bars. It holds the view, but is not a view itself.
 * It holds more Views that implementations can actually contain... as a convenience.
 */
class Header final : public SliderIncDec::Listener
{
public:
    /** Constructor. */
    explicit Header(HeaderController& headerController) noexcept;

    /**
     * Adds the views to the given parent.
     * @param parent the parent.
     */
    void addViewToParent(juce::Component& parent) noexcept;

    /** Sets the location of the whole view. The height is determined by the view itself. */
    void setBounds(int x, int y, int width) noexcept;

    /** @return the height. */
    int getHeight() const noexcept;

    /**
     * Sets the data to display, updates the UI if necessary. No callback is triggered.
     * @param loopStart the loop start.
     * @param end the loop end.
     * @param isLooping true if looping.
     * @param isRetrig true if retrigging (if relevant).
     * @param speed the speed.
     * @param shift the shift (if relevant).
     */
    void setDisplayedData(int loopStart, int end, bool isLooping, bool isRetrig, int speed, int shift);

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    /** Switches between loop on/off. */
    void onLoopButtonClicked() const noexcept;
    /** Switches between instrument retrig on/off. */
    void onRetrigButtonClicked() const noexcept;
    /** Switches between the zoom levels. */
    void onZoomButtonClicked() const noexcept;
    /** Shrinks the rows (visibility and height). */
    void onShrinkRowsButtonClicked(bool isLeftClicked) const noexcept;

    HeaderController& headerController;

    SliderIncDec loopStartSlider;
    SliderIncDec endSlider;
    SliderIncDec speedSlider;
    SliderIncDec shiftSlider;
    ButtonWithImage xZoomButton;
    ButtonWithImage shrinkRowsButton;

    ButtonWithImage loopButton;
    ButtonWithImage retrigButton;

    BlankComponent separator;
};

}   // namespace arkostracker
