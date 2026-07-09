#pragma once

#include "ButtonWithImage.h"
#include "EditText.h"

namespace arkostracker 
{

/**
 * Shows a Label (or image), a TextEditor, and two +/- buttons on the same line.
 *
 * The size of this Component is automatically calculated on construction.
 *
 * Warning, the philosophy may be counter-intuitive, but it was done to be consistent with the architecture of other Views.
 * Once the value has changed, the UI is NOT refreshed. Instead, the listener is called with the new value,
 * and it may (or not) update this View or not. This update will refresh the internal value and the UI, but
 * won't notify anything.
 */
class SliderIncDec final : public juce::Component,
                           public juce::TextEditor::Listener
{
public:
    /** Listener to a change of value. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the value has changed on the Slider.
         * @param slider the slider.
         * @param value the value. Always valid.
         */
        virtual void onWantToChangeSliderValue(SliderIncDec& slider, double value) = 0;

        /**
         * Called when the slider is validated with Enter key. The default implementation does nothing.
         * An implementation could close a pop-up for example.
         */
        virtual void onSliderValidated(SliderIncDec& slider, double value);

        /**
         * Called when the slider is canceled with Escape key. The default implementation does nothing.
         * An implementation could close a pop-up for example.
         */
        virtual void onSliderCancelled();
    };

    /** The filter on the TextEditor. */
    enum class Filter : std::uint8_t
    {
        signedHexadecimal,
        hexadecimal,
        integer,
        decimal,
        none,
    };

    /**
     * Constructor with a Label. The shown value is 0.
     * The size of this Component is automatically calculated on construction.
     * @param listener to be aware of the events.
     * @param labelText the text in the Label.
     * @param characterLength how many characters can be entered.
     * @param step the step to increase/decrease when using the mouse wheel or the inc/dec Buttons.
     * @param fastStep the fast step to increase/decrease when using the mouse wheel or the inc/dec Buttons.
     * @param textFilter what characters can be entered.
     * @param labelWidthIfWanted if <=0, the label size is automatically calculated. Use >0 to force the label width.
     * @param heightIfWanted if <=0, the height is forced.
     * @param mustHighlightValue true to highlight the text editor field once the value is set.
     */
    explicit SliderIncDec(Listener& listener, const juce::String& labelText, int characterLength = 2, double step = 1.0,
                          double fastStep = 2.0, Filter textFilter = Filter::hexadecimal,
                          int labelWidthIfWanted = 0, int heightIfWanted = 0, bool mustHighlightValue = false) noexcept;

    /**
     * Constructor with an Image. The shown value is 0.
     * The size of this Component is automatically calculated on construction.
     * @param listener to be aware of the events.
     * @param imageData the data of the image to show.
     * @param imageDataSize the size of the image data.
     * @param characterLength how many characters can be entered.
     * @param step the step to increase/decrease when using the mouse wheel or the inc/dec Buttons.
     * @param fastStep the fast step to increase/decrease when using the mouse wheel or the inc/dec Buttons.
     * @param textFilter what characters can be entered.
     * @param heightIfWanted if <=0, the height is forced.
     * @param mustHighlightValue true to highlight the text editor field once the value is set.
     */
    explicit SliderIncDec(Listener& listener, const void* imageData, size_t imageDataSize, int characterLength = 2, double step = 1.0,
                          double fastStep = 2.0, Filter textFilter = Filter::hexadecimal,
                          int heightIfWanted = 0, bool mustHighlightValue = false) noexcept;

    /** Returns the value of this Slider. */
    double getValue() const noexcept;

    /**
     * Sets the value to be seen. This refreshes the UI and the internal value, if different. This does NOT notify anything.
     * This should be used on init, or when the client has validated the value it receives via callback.
     * @param value the new value.
     */
    void setShownValue(double value) noexcept;

    /** @return the width of the label. */
    int getLabelWidth() const noexcept;


    // Component method implementations.
    // ==================================================
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void lookAndFeelChanged() override;

    // TextEditor::Listener method implementations.
    // ==================================================
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

private:
    static const int imageColorId;

    static const int buttonsWidth;                                              // How large are each Buttons.
    static const int spacingLabelAndTextEditor;                                 // Spacing between the Label and the TextEditor.
    static const int spacingTextEditorAndButtons;                               // Spacing between TextEditor and the Buttons.
    static const int magicMargin;

    static const juce::String textEditorInputFilterStringSignedHexadecimal;           // The signed hexadecimal allowed characters for the TextEditor.
    static const juce::String textEditorInputFilterStringHexadecimal;                 // The hexadecimal allowed characters for the TextEditor.
    static const juce::String textEditorInputFilterStringInteger;                     // The integer allowed characters for the TextEditor.
    static const juce::String textEditorInputFilterStringDecimal;                     // The decimal allowed characters for the TextEditor.

    /**
     * Initializes the Components. This must be called from the constructors.
     * @param characterLength how many characters can be entered.
     * @param height the height to use.
     */
    void initialize(int characterLength, int height) noexcept;

    /** Called when the Plus button is clicked. */
    void onPlusButtonClicked();
    /** Called when the Minus button is clicked. */
    void onMinusButtonClicked();

    /** Updates the displayed in the TextEditor, according to the inner value. */
    void updateUiFromInternalValue() noexcept;

    /** Notifies an addition in the value. This does not change the internal value and the UI, we rely on the client to notify us. */
    void notifyWantToAddValue(double valueToAdd) noexcept;
    /** Notifies a change of value. This does not change the internal value, we rely on the client to notify us. */
    void notifyWantToChangeValue(double value) noexcept;

    /**
     * Parses the TextEditor.
     * @return the read value.
     */
    double parseTextEditor() const noexcept;

    Listener& listener;

    Filter textFilter;
    double value;                                       // The value to display.
    double step;                                        // How much can be increased when increasing/decreasing.
    double fastStep;                                    // How much can be increased when increasing/decreasing with a fast step.

    std::unique_ptr<juce::Label> label;                 // The Label, if shown.
    std::unique_ptr<ColoredImage> image;                // The Image, if shown.
    EditText textEditor;
    ButtonWithImage minusButton;
    ButtonWithImage plusButton;

    juce::TextEditor::LengthAndCharacterRestriction textEditorInputFilterSignedHexadecimal; // Input filter for the TextEditor, signed hexadecimal.
    juce::TextEditor::LengthAndCharacterRestriction textEditorInputFilterHexadecimal; // Input filter for the TextEditor, hexadecimal.
    juce::TextEditor::LengthAndCharacterRestriction textEditorInputFilterDecimal;     // Input filter for the TextEditor, decimal.
    juce::TextEditor::LengthAndCharacterRestriction textEditorInputFilterInteger;     // Input filter for the TextEditor, integer.

    bool mustHighlightValue;                                    // Simple flag to highlight on arrival.
};

}   // namespace arkostracker
