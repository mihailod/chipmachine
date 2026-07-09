#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace arkostracker
{
/**
 * An image which is colored by a color of the look'n'feel. So if the theme changes, so does the image.
 * It can also be clicked via the onClick member.
 */
class ThemedColoredImage final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param imageBytes the bytes of the image (BinaryData::xxx).
     * @param imageSize the size of the image (BinaryData::xxxSize).
     * @param lookAndFeelColorId the ID of the color of the theme (see LookAndFeelConstants or any juce ColorId class).
     * @param opacityWhenNotHovered opacity when the mouse is not over.
     * @param opacityWhenHovered opacity when the mouse is over.
     */
    ThemedColoredImage(const void* imageBytes, size_t imageSize, int lookAndFeelColorId, float opacityWhenNotHovered = 0.4F, float opacityWhenHovered = 1.0F) noexcept;

    /** @return the image width. */
    int getImageWidth() const noexcept;
    /** @return the image height. */
    int getImageHeight() const noexcept;

    // Component method implementations.
    // ======================================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void lookAndFeelChanged() override;

    std::function<void(bool isLeftButtonClicked)> onClick;                  // NOLINT(*-non-private-member-variables-in-classes)

private:
    static constexpr auto opacityWhenDisabled = 0.4F;

    /** @return the color from the current color ID. */
    juce::Colour determineImageColor() const noexcept;

    juce::Image innerImage;
    int lookAndFeelColorId;
    juce::Colour cachedColor;

    float opacityWhenNotHovered;
    float opacityWhenHovered;

    bool isHovered;
};

} // namespace arkostracker
