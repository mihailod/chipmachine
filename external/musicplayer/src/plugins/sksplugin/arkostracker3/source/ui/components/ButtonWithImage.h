#pragma once

#include "ColoredImage.h"
#include "../../utils/WithParent.h"

namespace arkostracker 
{

/**
 * A Button with an image, with one state, two, or more states. Clicking does NOT change anything, it only notifies the listener (UF compliant).
 * Limitation: the image itself does not change according to the mouse events (the button background does).
 */
class ButtonWithImage final : public juce::Component
{
public:
    /** Destructor. */
    ~ButtonWithImage() override;

    /**
     * Constructor with one image only.
     * @param imageBytes the bytes of the image (BinaryData::xxx).
     * @param imageSize the size of the image (BinaryData::xxxSize).
     * @param tooltip the tooltip to show, or empty.
     * @param onClick lambda called when the button is clicked, with the id, the current state, is left-clicked.
     * @param id an ID of the button, send back when clicked.
     */
    ButtonWithImage(const void* imageBytes, size_t imageSize, const juce::String& tooltip, std::function<void(int, bool, bool)> onClick, int id = 0) noexcept;

    /**
     * Constructor with two images. Do NOT use alpha in the images if using the "colorize" parameters, it will be overridden.
     * @param imageOffBytes the bytes of the image (BinaryData::xxx) when off.
     * @param imageOffSize the size of the image (BinaryData::xxxSize).
     * @param imageOnBytes the bytes of the image (BinaryData::xxx) when on.
     * @param imageOnSize the size of the image (BinaryData::xxxSize).
     * @param tooltip the tooltip to show, or empty.
     * @param onClick lambda called when the button is clicked, with the id, the current state, is left-clicked. The UI does NOT change.
     * @param id an ID of the button, send back when clicked.
     * @param colorizeOffImage true to colorize the off image according to the look'n'feel.
     * @param colorizeOnImage true to colorize the on image according to the look'n'feel.
     */
    ButtonWithImage(const void* imageOffBytes, size_t imageOffSize, const void* imageOnBytes, size_t imageOnSize, const juce::String& tooltip,
                             std::function<void(int, bool, bool)> onClick, int id = 0, bool colorizeOffImage = true, bool colorizeOnImage = true) noexcept;

    /**
     * Constructor with two images. Do NOT use alpha in the images if using the "colorize" parameters, it will be overridden.
     * @param imageOffBytes the bytes of the image (BinaryData::xxx) when off.
     * @param imageOffSize the size of the image (BinaryData::xxxSize).
     * @param imageOnBytes the bytes of the image (BinaryData::xxx) when on.
     * @param imageOnSize the size of the image (BinaryData::xxxSize).
     * @param tooltips the tooltips to show, one per state, or empty.
     * @param onClick lambda called when the button is clicked, with the id and the current state, and true if left-clicked. The UI does NOT change.
     * @param id an ID of the button, send back when clicked.
     * @param colorizeOffImage true to colorize the off image according to the look'n'feel.
     * @param colorizeOnImage true to colorize the on image according to the look'n'feel.
     */
    ButtonWithImage(const void* imageOffBytes, size_t imageOffSize, const void* imageOnBytes, size_t imageOnSize, std::vector<juce::String> tooltips,
                             std::function<void(int, bool, bool)> onClick, int id = 0, bool colorizeOffImage = true, bool colorizeOnImage = true) noexcept;

    /**
     * Constructor to add images and tooltips manually. Don't forget to set the state afterward.
     * @param onClick the callback, with the current state. The client might want to increase (and loop it) and call setStateInt with the next state.
     */
    explicit ButtonWithImage(std::function<void(int)> onClick) noexcept;

    /**
     * Sets the toggle state. Will change the background only.
     * @param toggled true if toggled (highlighted).
     */
    void setToggleState(bool toggled) noexcept;

    /**
     * Updates the UI to show the state (bool), if new. Does not notify anything.
     * @param newState the new state.
     */
    void setState(bool newState) noexcept;

    /**
     * Updates the UI to show the state (int), if new. Does not notify anything.
     * @param newState the new state.
     */
    void setStateInt(int newState) noexcept;

    /**
     * Adds an image and its tooltip. Uses this if using the simplest constructor.
     * @param image the image to add.
     * @param tooltip the tooltip related to this image.
     */
    void addImage(std::unique_ptr<ColoredImage> image, juce::String tooltip) noexcept;

    /** Animates the bounds for a few seconds to indicate an error. */
    void animateForError() noexcept;

    // Component method implementations
    // ===============================================
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void paintOverChildren(juce::Graphics& g) override;

private:
    static constexpr auto animationMs = 100;
    static constexpr auto animationCount = 6;

    class LocalTimer final : public juce::Timer,
                             WithParent<ButtonWithImage>
    {
    public:
        explicit LocalTimer(ButtonWithImage& parent) : WithParent(parent)
        {
        }

        void timerCallback() override
        {
            parentObject.onAnimationTimerFinished();
        }
    };

    /**
     * @return the color to use to color the image, from the current theme.
     * @param contrasted false to get the normal color, true if contrasted.
     */
    static juce::Colour getImageColorFromTheme(bool contrasted = false) noexcept;

    /** Initializes the object. */
    void initialize() noexcept;

    /** Show an image according to the current state. */
    void showImagesFromState() const noexcept;

    /**
     * Must be called when an image is added.
     * @param image the image.
     */
    void setUpImage(ColoredImage& image) noexcept;

    /**
     * Sets the image color according to the color of this class, and the toggle state of the button.
     * @param image the image.
     */
    void setImageColor(ColoredImage& image) const noexcept;

    /** Called when the animation timer is finished. */
    void onAnimationTimerFinished() noexcept;

    const int id;
    const std::function<void(int, bool, bool)> onClickBool;
    const std::function<void(int)> onClickMultiple;                     // Used if multiple images (>2).

    juce::TextButton textButton;                                        // Empty text, but necessary to show the background.

    juce::Colour colorForImage;                                         // The color to use for the image.
    std::vector<std::unique_ptr<ColoredImage>> coloredImages;           // The first is the "off".
    std::vector<juce::String> tooltips;                                 // May be empty.

    int state;                                                          // The current state.

    std::unique_ptr<juce::Timer> animationTimer;
    int animationCounter;
    bool showAnimationBorder;
};

}   // namespace arkostracker
