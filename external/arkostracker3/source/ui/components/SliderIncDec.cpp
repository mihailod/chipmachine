#include "SliderIncDec.h"

#include <BinaryData.h>

#include <cmath>

#include "../../utils/NumberUtil.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int SliderIncDec::buttonsWidth = 20;
const int SliderIncDec::spacingLabelAndTextEditor = 4;
const int SliderIncDec::spacingTextEditorAndButtons = 2;
const int SliderIncDec::magicMargin = 16;

const int SliderIncDec::imageColorId = juce::Label::ColourIds::textColourId;

const juce::String SliderIncDec::textEditorInputFilterStringSignedHexadecimal = "-ABCDEFabcdef0123456789";        // Signed hexadecimal number only.  NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SliderIncDec::textEditorInputFilterStringHexadecimal = "ABCDEFabcdef0123456789";               // Hexadecimal number only.         NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SliderIncDec::textEditorInputFilterStringInteger = "0123456789";                               // Integer number only.             NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SliderIncDec::textEditorInputFilterStringDecimal = textEditorInputFilterStringInteger + ".";   // Decimal number only.           NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

void SliderIncDec::Listener::onSliderValidated(SliderIncDec& /*slider*/, double /*value*/)
{
    // The default implementation does nothing.
}

void SliderIncDec::Listener::onSliderCancelled()
{
    // The default implementation does nothing.
}


// ==============================================

SliderIncDec::SliderIncDec(Listener& pListener, const juce::String& pLabelText, const int pCharacterLength, const double pStep, const double pFastStep, const Filter pTextFilter,
                           const int pLabelWidthIfWanted, const int pHeightIfWanted, const bool pMustHighlightValue) noexcept :
        // WARNING, second constructor below with (almost) the same code.
        listener(pListener),

        textFilter(pTextFilter),
        value(0.0),
        step(pStep),
        fastStep(pFastStep),

        label(std::make_unique<juce::Label>(juce::String(), pLabelText)),
        image(),        // No image.
        textEditor(),
        minusButton(BinaryData::IconDecSmall_png, static_cast<size_t>(BinaryData::IconDecSmall_pngSize), juce::translate("Decrease the value"),
                    [&](int, bool, bool) { onMinusButtonClicked(); } ),
        plusButton(BinaryData::IconIncSmall_png, static_cast<size_t>(BinaryData::IconIncSmall_pngSize), juce::translate("Increase the value"),
                   [&](int, bool, bool) { onPlusButtonClicked(); } ),

        textEditorInputFilterSignedHexadecimal(pCharacterLength, textEditorInputFilterStringSignedHexadecimal),
        textEditorInputFilterHexadecimal(pCharacterLength, textEditorInputFilterStringHexadecimal),
        textEditorInputFilterDecimal(pCharacterLength, textEditorInputFilterStringDecimal),
        textEditorInputFilterInteger(pCharacterLength, textEditorInputFilterStringInteger),
        mustHighlightValue(pMustHighlightValue)
{
    // Sets up the Label.
    const auto height = (pHeightIfWanted <= 0) ? LookAndFeelConstants::buttonsHeight : pHeightIfWanted;
    const auto labelWidth = pLabelWidthIfWanted > 0
                            ? pLabelWidthIfWanted
                            : label->getFont().getStringWidth(pLabelText) + magicMargin;        // Strange, have to add small margin, else the text is crushed.
    label->setBounds(0, 0, labelWidth, height);

    addAndMakeVisible(*label);

    // Finally, initializes the rest.
    initialize(pCharacterLength, height);
}

SliderIncDec::SliderIncDec(Listener& pListener, const void* pImageData, size_t pImageDataSize, const int pCharacterLength, const double pStep, const double pFastStep,
                           const Filter pTextFilter, const int pHeightIfWanted, const bool pMustHighlightValue) noexcept :
        // WARNING, first constructor below with (almost) the same code.
        listener(pListener),

        textFilter(pTextFilter),
        value(0.0),
        step(pStep),
        fastStep(pFastStep),

        label(),        // No Label.
        image(std::make_unique<ColoredImage>(pImageData, pImageDataSize,
                                             findColour(imageColorId),
                                             true)),
        textEditor(),
        minusButton(BinaryData::IconDecSmall_png, static_cast<size_t>(BinaryData::IconDecSmall_pngSize), juce::translate("Decrease the value"),
        [&](int, bool, bool) { onMinusButtonClicked(); } ),
        plusButton(BinaryData::IconIncSmall_png, static_cast<size_t>(BinaryData::IconIncSmall_pngSize), juce::translate("Increase the value"),
        [&](int, bool, bool) { onPlusButtonClicked(); } ),

        textEditorInputFilterSignedHexadecimal(pCharacterLength, textEditorInputFilterStringSignedHexadecimal),
        textEditorInputFilterHexadecimal(pCharacterLength, textEditorInputFilterStringHexadecimal),
        textEditorInputFilterDecimal(pCharacterLength, textEditorInputFilterStringDecimal),
        textEditorInputFilterInteger(pCharacterLength, textEditorInputFilterStringInteger),
        mustHighlightValue(pMustHighlightValue)
{
    // Sets up the image.
    const auto height = (pHeightIfWanted <= 0) ? LookAndFeelConstants::buttonsHeight : pHeightIfWanted;
    image->setBounds(0, 0, image->getImageWidth() + magicMargin, height);        // Strange, have to add small margin, else the text is crushed.
    addAndMakeVisible(*image);

    // Finally, initializes the rest.
    initialize(pCharacterLength, height);
}

void SliderIncDec::initialize(const int characterLength, const int height) noexcept
{
    // Calculates the size, or uses the given one.

    // Far from optimum... Who cares.
    juce::String str;
    for (auto i = 0; i < characterLength; ++i) {
        str += "8";     // Considers this is the largest digit.
    }
    const auto textEditorWidth = textEditor.getFont().getStringWidth(str) + magicMargin;        // Strange, have to add small margin, else the text is crushed.

    constexpr auto y = 0;
    const auto textEditorX = (label != nullptr) ? label->getRight() : image->getRight();
    textEditor.setBounds(textEditorX + spacingLabelAndTextEditor, y, textEditorWidth, height);
    minusButton.setBounds(textEditor.getRight() + spacingTextEditorAndButtons, y, buttonsWidth, height);
    plusButton.setBounds(minusButton.getRight() - 1, y, buttonsWidth, height);      // Overlaps on the last pixel, they are the same.

    setSize(plusButton.getRight(), height);
    addAndMakeVisible(textEditor);
    addAndMakeVisible(minusButton);
    addAndMakeVisible(plusButton);

    // Sets the text filter.
    juce::TextEditor::InputFilter* filter; // NOLINT(*-init-variables)
    switch (textFilter) {
        case Filter::signedHexadecimal :
            filter = &textEditorInputFilterSignedHexadecimal;
            break;
        case Filter::hexadecimal :
            filter = &textEditorInputFilterHexadecimal;
            break;
        case Filter::decimal :
            filter = &textEditorInputFilterDecimal;
            break;
        case Filter::integer :
            filter = &textEditorInputFilterInteger;
            break;
        default:
            jassertfalse;       // Shouldn't happen!
        case Filter::none :
            filter = nullptr;
            break;
    }
    textEditor.setInputFilter(filter, false);
    textEditor.setJustification(juce::Justification::Flags::centredTop);        // Top-aligned looks better.
    //textEditor.setEscapeAndReturnKeysConsumed(false);       // Unable to make this work from within the component, have to use a listener.

    // Sets the listener on the +/- Buttons and TextEditor.
    textEditor.addListener(this);

    // Removes the focus on click (much better for the focus), but not on the TextEditor, else it is not possible to edit it.
    minusButton.setMouseClickGrabsKeyboardFocus(false);
    plusButton.setMouseClickGrabsKeyboardFocus(false);

    // Shows the initial value.
    updateUiFromInternalValue();
}

double SliderIncDec::getValue() const noexcept
{
    return value;
}

int SliderIncDec::getLabelWidth() const noexcept
{
    if (label == nullptr) {
        jassertfalse;           // This method shouldn't be called, there is no Label.
        return 0;
    }
    return label->getWidth();
}

void SliderIncDec::setShownValue(const double newValue) noexcept
{
    if (juce::exactlyEqual(newValue, value)) {
        return;
    }
    value = newValue;
    updateUiFromInternalValue();

    if (mustHighlightValue) {
        textEditor.setHighlightedRegion({ 0, 999999 });
        mustHighlightValue = false;
    }
}

void SliderIncDec::updateUiFromInternalValue() noexcept
{
    juce::String text;
    if (textFilter == Filter::signedHexadecimal) {
        text = NumberUtil::signedHexToStringWithPrefix(static_cast<int>(value), juce::String(), false, false);
    } else if (textFilter == Filter::hexadecimal) {
        text = juce::String::toHexString(static_cast<int>(value));
    } else {
        // Not hexadecimal. Displays the comma, or not.
        if (textFilter == Filter::decimal) {
            // Approximate the result to one decimal when displaying.
            const auto roundedValue = std::roundf(static_cast<float>(value) * 10.0F) / 10.0F;
            text = juce::String(roundedValue);
        } else {
            text = juce::String(static_cast<float>(value));
        }
    }
    textEditor.setText(text, false);
}

void SliderIncDec::notifyWantToAddValue(const double valueToAdd) noexcept
{
    if (isEnabled()) {
        notifyWantToChangeValue(value + valueToAdd);
    }
}

void SliderIncDec::notifyWantToChangeValue(const double newValue) noexcept
{
    listener.onWantToChangeSliderValue(*this, newValue);
}

double SliderIncDec::parseTextEditor() const noexcept
{
    const auto text = textEditor.getText();
    double parsedValue; // NOLINT(*-init-variables)
    if ((textFilter == Filter::signedHexadecimal) || (textFilter == Filter::hexadecimal)) {
        parsedValue = static_cast<double>(NumberUtil::signedHexStringToInt(text));
    } else {
        // Not hexadecimal. Can be read as-is.
        parsedValue = text.getDoubleValue();
    }

    return parsedValue;
}

void SliderIncDec::onPlusButtonClicked()
{
    notifyWantToAddValue(step);
}

void SliderIncDec::onMinusButtonClicked()
{
    notifyWantToAddValue(-step);
}


// TextEditor::Listener method implementations.
// ==================================================

void SliderIncDec::textEditorReturnKeyPressed(juce::TextEditor& /*editor*/)
{
    const auto parsedValue = parseTextEditor();

    updateUiFromInternalValue();            // Puts back and displays the previous value! It has not been validated yet!

    // Shows the change to the listener.
    notifyWantToChangeValue(parsedValue);

    listener.onSliderValidated(*this, parsedValue);
}

void SliderIncDec::textEditorEscapeKeyPressed(juce::TextEditor& /*editor*/)
{
    // Cancels the typed value.
    updateUiFromInternalValue();

    listener.onSliderCancelled();
}

void SliderIncDec::textEditorFocusLost(juce::TextEditor& /*editor*/)
{
    const auto parsedValue = parseTextEditor();

    updateUiFromInternalValue();            // Puts back and displays the previous value! It has not been validated yet!

    // Shows the change to the listener.
    notifyWantToChangeValue(parsedValue);
}


// Component method implementations.
// ==================================================

void SliderIncDec::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // If shift used, uses the fast step.
    auto currentStep = event.mods.isShiftDown() ? fastStep : step;
    if (wheel.deltaY < 0.0F) {
        currentStep = -currentStep;
    }

    notifyWantToAddValue(currentStep);
}

void SliderIncDec::lookAndFeelChanged()
{
    // Have to update the TextEditor... Strange behavior, see:
    // https://forum.juce.com/t/texteditor-text-colour-not-updated-after-lookandfeelchanged-parent-change/24322/3
    textEditor.applyColourToAllText(getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId), true);
    if (image != nullptr) {
        image->setImageColor(findColour(imageColorId));
    }
}

}   // namespace arkostracker
