#include "SetValueDialog.h"

#include "../../../utils/NumberUtil.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

SetValueDialog::SetValueDialog(Listener& pListener, const int pInitialValue, const bool pShowInHex, const int pMinimumValue, const int pMaximumValue,
                               const juce::String& title, const juce::String& label, const juce::String& topText) noexcept :
        ModalDialog(title, 240, topText.isEmpty() ? 110 : 140,
                    [this] { onDialogOk(); },
                    [this] { onDialogCancelled(); },
                    true, true),
        listener(pListener),
        text(juce::String(), topText),
        valueSlider(*this, label, 4, 1.0, 2.0, pShowInHex ? SliderIncDec::Filter::signedHexadecimal : SliderIncDec::Filter::integer,
            0, 0, true),
        minimumValue(pMinimumValue),
        maximumValue(pMaximumValue)
{
    const auto& usableBounds = getUsableModalDialogBounds();
    const auto left = usableBounds.getX();
    const auto top = usableBounds.getY();
    const auto width = usableBounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    const auto showTopText = topText.isNotEmpty();

    text.setBounds(left, top, width, labelsHeight);
    valueSlider.setTopLeftPosition(left + ((width - valueSlider.getWidth()) / 2), showTopText ? (text.getBottom() + margins) : top);

    if (showTopText) {
        addComponentToModalDialog(text);
    }
    addComponentToModalDialog(valueSlider);

    text.setJustificationType(juce::Justification::centredTop);
    valueSlider.setShownValue(pInitialValue);

    giveFocus(valueSlider);
}

void SetValueDialog::onDialogOk() const noexcept
{
    const auto value = static_cast<int>(valueSlider.getValue());
    listener.onWantToSetValue(value);
}

void SetValueDialog::onDialogCancelled() const noexcept
{
    listener.onWantToCancelSetValue();
}


// SliderIncDec::Listener method implementations.
// ==============================================

void SetValueDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    jassert(&slider == &valueSlider); (void)slider;               // Any other undeclared Slider?

    // Corrects the value.
    const auto newValue = NumberUtil::correctNumber(valueDouble, static_cast<double>(minimumValue), static_cast<double>(maximumValue));
    valueSlider.setShownValue(newValue);
}

void SetValueDialog::onSliderValidated(SliderIncDec& /*slider*/, double /*value*/)
{
    onDialogOk();
}

void SetValueDialog::onSliderCancelled()
{
    onDialogCancelled();
}

}   // namespace arkostracker
