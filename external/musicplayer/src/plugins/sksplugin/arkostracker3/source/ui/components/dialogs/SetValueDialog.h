#pragma once

#include "ModalDialog.h"
#include "../SliderIncDec.h"

namespace arkostracker 
{

/** Generic dialog to edit a value. */
class SetValueDialog final : public ModalDialog,
                             public SliderIncDec::Listener
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** The user cancelled the Position editing. */
        virtual void onWantToCancelSetValue() = 0;
        /** The user wants to set new data. */
        virtual void onWantToSetValue(int newValue) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the user's intent.
     * @param initialValue the initial value to show.
     * @param showInHex true to show in hex, false in decimal.
     * @param minimumValue the minimum value possible.
     * @param maximumValue the maximum value possible.
     * @param title the title.
     * @param label the label.
     * @param topText a possible text at the top. If empty, not shown.
     */
    SetValueDialog(Listener& listener, int initialValue, bool showInHex, int minimumValue, int maximumValue, const juce::String& title, const juce::String& label,
        const juce::String& topText = juce::String()) noexcept;

    // SliderIncDec::Listener method implementations.
    // ============================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;
    void onSliderValidated(SliderIncDec& slider, double value) override;
    void onSliderCancelled() override;

private:
    /** Called when the Ok button is clicked. */
    void onDialogOk() const noexcept;
    /** Called when the Cancel button is clicked. */
    void onDialogCancelled() const noexcept;

    Listener& listener;

    juce::Label text;

    SliderIncDec valueSlider;
    int minimumValue;
    int maximumValue;
};

}   // namespace arkostracker
