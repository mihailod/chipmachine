#include "ButtonWithImage.h"

#include "../lookAndFeel/CustomLookAndFeel.h"

namespace arkostracker 
{

ButtonWithImage::ButtonWithImage(const void* pImageBytes, const size_t pImageSize, const juce::String& pTooltip, std::function<void(int, bool, bool)> pOnClick,
    const int pId) noexcept :
        id(pId),
        onClickBool(std::move(pOnClick)),
        onClickMultiple(nullptr),
        textButton(),
        colorForImage(getImageColorFromTheme()),
        coloredImages(),
        tooltips({ pTooltip }),
        state(),
        animationTimer(),
        animationCounter(),
        showAnimationBorder()
{
    coloredImages.push_back(std::make_unique<ColoredImage>(pImageBytes, pImageSize, colorForImage));

    initialize();
}

ButtonWithImage::ButtonWithImage(const void* pImageOffBytes, size_t pImageOffSize, const void* pImageOnBytes, size_t pImageOnSize, const juce::String& pTooltip,
                                 std::function<void(int, bool, bool)> pOnClick, const int pId, bool pColorizeOffImage, bool pColorizeOnImage) noexcept :
        id(pId),
        onClickBool(std::move(pOnClick)),
        onClickMultiple(nullptr),
        textButton(),
        colorForImage(getImageColorFromTheme()),
        coloredImages(),
        tooltips({ pTooltip }),
        state(),
        animationTimer(),
        animationCounter(),
        showAnimationBorder()
{
    coloredImages.push_back(std::make_unique<ColoredImage>(pImageOffBytes, pImageOffSize, colorForImage, pColorizeOffImage));
    coloredImages.push_back(std::make_unique<ColoredImage>(pImageOnBytes, pImageOnSize, colorForImage, pColorizeOnImage));

    initialize();
}

ButtonWithImage::ButtonWithImage(const void* pImageOffBytes, size_t pImageOffSize, const void* pImageOnBytes, size_t pImageOnSize, std::vector<juce::String> pTooltips,
                                 std::function<void(int, bool, bool)> pOnClick, const int pId, bool pColorizeOffImage, bool pColorizeOnImage) noexcept :
        id(pId),
        onClickBool(std::move(pOnClick)),
        onClickMultiple(nullptr),
        textButton(),
        colorForImage(getImageColorFromTheme()),
        coloredImages(),
        tooltips(std::move(pTooltips)),
        state(),
        animationTimer(),
        animationCounter(),
        showAnimationBorder()
{
    coloredImages.push_back(std::make_unique<ColoredImage>(pImageOffBytes, pImageOffSize, colorForImage, pColorizeOffImage));
    coloredImages.push_back(std::make_unique<ColoredImage>(pImageOnBytes, pImageOnSize, colorForImage, pColorizeOnImage));

    initialize();
}

ButtonWithImage::ButtonWithImage(std::function<void(int)> pOnClick) noexcept :
        id(),
        onClickBool(nullptr),
        onClickMultiple(std::move(pOnClick)),
        textButton(),
        colorForImage(getImageColorFromTheme()),
        coloredImages(),
        tooltips(),
        state(-1),
        animationTimer(),
        animationCounter(),
        showAnimationBorder()
{
    initialize();
}

ButtonWithImage::~ButtonWithImage()
{
    if (animationTimer != nullptr) {
        animationTimer->stopTimer();
    }
}

void ButtonWithImage::addImage(std::unique_ptr<ColoredImage> image, juce::String tooltip) noexcept
{
    setUpImage(*image);

    coloredImages.push_back(std::move(image));
    tooltips.push_back(std::move(tooltip));
}

void ButtonWithImage::initialize() noexcept
{
    // Selects the first tooltip, if any.
    if (!tooltips.empty()) {
        textButton.setTooltip(tooltips.front());
    }

    addAndMakeVisible(textButton);
    for (auto& coloredImage : coloredImages) {
        setUpImage(*coloredImage);
    }

    textButton.addMouseListener(this, true);

    // Removes the focus on click (much better for the focus).
    textButton.setMouseClickGrabsKeyboardFocus(false);

    showImagesFromState();
}

void ButtonWithImage::setUpImage(ColoredImage& image) noexcept
{
    addChildComponent(image);
    image.setInterceptsMouseClicks(false, false);       // So that the Button behind can get the mouse event (mouseOver), etc.
    image.setMouseClickGrabsKeyboardFocus(false);       // Removes the focus on click (must better for the focus).
}

void ButtonWithImage::resized()
{
    const auto bounds = getLocalBounds();
    textButton.setBounds(bounds);
    for (const auto& coloredImage : coloredImages) {
        coloredImage->setBounds(bounds);
    }
}

void ButtonWithImage::mouseDown(const juce::MouseEvent& event)
{
    // A click is performed on the TextButton. Transmits to the client.
    // What callback?
    if (onClickBool != nullptr) {
        onClickBool(id, (state != 0), event.mods.isLeftButtonDown());
    } else if (onClickMultiple != nullptr) {
        onClickMultiple(state);
    }
}

void ButtonWithImage::paintOverChildren(juce::Graphics& g)
{
    if (showAnimationBorder) {
        const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
        const auto color = currentLookAndFeel.getColor(static_cast<int>(LookAndFeelConstants::Colors::patternViewerError), false);

        g.setColour(color);
        g.drawRect(getLocalBounds(), 2);
    }
}

juce::Colour ButtonWithImage::getImageColorFromTheme(const bool contrasted) noexcept
{
    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
    return currentLookAndFeel.getColor(juce::TextButton::ColourIds::textColourOnId, contrasted);
}

void ButtonWithImage::lookAndFeelChanged()
{
    // Did the Button image color changed from the theme? If no, don't do anything.
    const auto newColor = getImageColorFromTheme(false);
    if (colorForImage != newColor) {
        colorForImage = newColor;
        for (const auto& coloredImage : coloredImages) {
            setImageColor(*coloredImage);
        }
    }
}

void ButtonWithImage::setState(const bool newState) noexcept
{
    setStateInt(newState ? 1 : 0);
}

void ButtonWithImage::setStateInt(const int newState) noexcept
{
    if (state == newState) {
        return;
    }
    state = newState;

    showImagesFromState();

    // Selects the related tooltip, else uses the first one.
    if (!tooltips.empty()) {
        const auto tooltipIndex = (newState < static_cast<int>(tooltips.size())) ? newState : 0;
        textButton.setTooltip(tooltips.at(static_cast<size_t>(tooltipIndex)));
    }
}

void ButtonWithImage::showImagesFromState() const noexcept
{
    jassert(state < static_cast<int>(coloredImages.size()));

    auto imageIndex = 0;
    for (const auto& coloredImage : coloredImages) {
        coloredImage->setVisible(state == imageIndex);
        ++imageIndex;
    }
}

void ButtonWithImage::setToggleState(const bool toggled) noexcept
{
    if (toggled != textButton.getToggleStateValue()) {
        textButton.setToggleState(toggled, juce::NotificationType::dontSendNotification);

        for (const auto& coloredImage : coloredImages) {
            setImageColor(*coloredImage);
        }
    }
}

void ButtonWithImage::setImageColor(ColoredImage& image) const noexcept
{
    const auto toggled = textButton.getToggleState();
    image.setImageColor(getImageColorFromTheme(toggled));
}

void ButtonWithImage::animateForError() noexcept
{
    // Don't restart if already started.
    if ((animationTimer != nullptr) && animationTimer->isTimerRunning()) {
        return;
    }

    animationCounter = 0;
    showAnimationBorder = true;

    animationTimer.reset();
    animationTimer = std::make_unique<LocalTimer>(*this);
    animationTimer->startTimer(animationMs);

    repaint();
}

void ButtonWithImage::onAnimationTimerFinished() noexcept
{
    // Makes the border flicker, as long as we want it.
    if (++animationCounter < animationCount) {
        showAnimationBorder = !showAnimationBorder;
        animationTimer->startTimer(animationMs);
    } else {
        showAnimationBorder = false;
        animationTimer.reset();
    }

    repaint();
}

}   // namespace arkostracker
