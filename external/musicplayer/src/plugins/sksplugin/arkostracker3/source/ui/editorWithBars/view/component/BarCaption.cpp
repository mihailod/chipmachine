#include "BarCaption.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../../utils/TextUtil.h"

namespace arkostracker 
{

const int BarCaption::maxHeightImage = 20;
const int BarCaption::maxHeightMainText = 15;
const int BarCaption::fontHeight = 16;
const juce::uint8 BarCaption::backgroundColorAlpha = 0xa0;
const int BarCaption::minimumCaptionWidthForVisibility = 20;

BarCaption::BarCaption() noexcept :
        displayedData(),
        background(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundUnfocused)),
        imageComponent(),
        mainLabel(),
        textFont(juce::Font(LookAndFeelConstants::typefaceNameSquare, static_cast<float>(fontHeight), juce::Font::FontStyleFlags::plain))
{
    background.setTransformationOnColour([] (const juce::Colour color){
        return color.withAlpha(backgroundColorAlpha);
    });

    // The visibility will be set when the data is set.
    addChildComponent(background);
    addChildComponent(imageComponent);
    addChildComponent(mainLabel);

    setInterceptsMouseClicks(false, false);
    mainLabel.setFont(textFont);
}

void BarCaption::setDisplayedData(const BarCaptionDisplayedData& newDisplayedData) noexcept
{
    // Anything new to show?
    if (displayedData == newDisplayedData) {
        return;
    }
    displayedData = newDisplayedData;

    // What to display?
    const auto isIgnored = displayedData.isIgnored();
    // Alpha is lower if the value is out of loop, ignored or generated (because not "set by hand").
    const auto alpha = (isIgnored || !displayedData.isWithinLoop() || displayedData.isGenerated()) ? 0.2F : 1.0F;
    const auto visible = !displayedData.isOutOfBounds();        // Don't display the values that are out of bounds.
    background.setVisible(visible && !isIgnored);
    const auto& image = displayedData.getImage();
    imageComponent.setVisible(visible && !isIgnored && image.isValid());
    imageComponent.setImage(image);
    setImageComponentColor();
    imageComponent.setAlpha(alpha);

    const auto& mainText = displayedData.getMainText();
    mainLabel.setVisible(visible && !isIgnored && mainText.isNotEmpty());
    mainLabel.setText(mainText, juce::NotificationType::dontSendNotification);
    mainLabel.setJustificationType(juce::Justification::left);
    mainLabel.setBorderSize(juce::BorderSize<int>());       // Saves a lot of space!
    mainLabel.setAlpha(alpha);

    // Puts the views in their right location. Can change because the image may be present or hidden now.
    locateItems();
}

void BarCaption::locateItems() noexcept
{
    const auto width = getWidth();
    const auto height = getHeight();

    constexpr auto verticalMargins = 4;
    constexpr auto textLeftMargin = 4;
    constexpr auto backgroundHorizontalMargins = 2;
    const auto& image = displayedData.getImage();
    const auto isImagePresent = image.isValid();
    const auto imageOriginalWidth = image.getWidth();

    const auto availableWidth = width - 2 * textLeftMargin;

    const auto isTextPresent = !displayedData.getMainText().isEmpty();
    const auto bottomViewY = height - maxHeightMainText - verticalMargins;
    const auto imageY = isTextPresent ? 0 : (bottomViewY - 1);

    imageComponent.setBounds(textLeftMargin, imageY, imageOriginalWidth, maxHeightImage);

    // Try to find a compromise for squashing horizontally. This is empirical...
    const auto isLongText = mainLabel.getText().length() > 2;
    mainLabel.setMinimumHorizontalScale(isLongText ? 0.4F : 1.0F);
    const auto longTextWidthRatio = isLongText ? 1.0 : 1.6;

    const auto textWidth = std::min(availableWidth, static_cast<int>(textFont.getStringWidth(mainLabel.getText()) * longTextWidthRatio));
    const auto imageWidth = isImagePresent ? imageOriginalWidth : 0;
    const auto backgroundWidth = std::max(textWidth - (isLongText ? 1 : 4), imageWidth) + 2 * backgroundHorizontalMargins;
    const auto textY = bottomViewY;
    mainLabel.setBounds(textLeftMargin, textY, textWidth, maxHeightMainText);

    // Wraps the background on the two elements, or on one if there is only one.
    const auto backgroundY = isImagePresent ? (imageY + 4) : textY;
    background.setBounds(textLeftMargin - backgroundHorizontalMargins, backgroundY, backgroundWidth, height - backgroundY - verticalMargins);

    // Don't show the caption if there is no data to show, or we don't have room to show it.
    setVisible(isDataPresent() && (getWidth() >= minimumCaptionWidthForVisibility));
}

bool BarCaption::isDataPresent() const noexcept
{
    return displayedData.getMainText().isNotEmpty() || displayedData.getImage().isValid();
}

void BarCaption::setImageComponentColor() noexcept
{
    const auto color = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);
    imageComponent.setImageColor(color);
}


// Component method implementations.
// ====================================

void BarCaption::resized()
{
    locateItems();
}

void BarCaption::lookAndFeelChanged()
{
    Component::lookAndFeelChanged();

    setImageComponentColor();
}

}   // namespace arkostracker
