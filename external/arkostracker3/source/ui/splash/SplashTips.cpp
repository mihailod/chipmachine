#include "SplashTips.h"

#include <BinaryData.h>

#include "../../app/preferences/PreferencesManager.h"
#include "../../controllers/MainController.h"
#include "../lookAndFeel/CustomLookAndFeel.h"

namespace arkostracker 
{

SplashTips::SplashTips(const MainController& pMainController, std::function<void()> pInputCallback) noexcept :
        CustomResizableWindow("Splash"),
        callback(std::move(pInputCallback)),

        tipsController(pMainController.getCommandManager()),

        contentComponent(),
        logoImage(),
        tipOfTheDayLabel(juce::String(), juce::translate("Tip of the day:")),
        separatorTop(juce::Label::ColourIds::textColourId),
        separatorBottom(juce::Label::ColourIds::textColourId),
        text(),
        tipImage(),
        tipZippedImages(),
        showSplashToggleButton(juce::translate("Show this on start-up")),
        versionLabel(),
        okButton(juce::translate("Go!")),
        nextTipButton(juce::translate("Next tip"))
{
}


// SplashDialog method implementations.
// ==========================================================

void SplashTips::show() noexcept
{
    constexpr auto desiredWidth = 650;
    constexpr auto desiredHeight = 595;

    setOpaque(true);

    logoImage.setImage(juce::ImageFileFormat::loadFrom(BinaryData::LogoWithTextBelow_png, BinaryData::LogoWithTextBelow_pngSize), juce::RectanglePlacement::centred);
    setUpVersionLabel(versionLabel);
    showSplashToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    okButton.onClick = [&] { exitSplash(); };
    nextTipButton.onClick = [&] { showNextTip(); };

    centreWithSize(desiredWidth, desiredHeight);
    contentComponent.setSize(desiredWidth, desiredHeight);
    setContentNonOwned(&contentComponent, false);

    contentComponent.addAndMakeVisible(&logoImage);
    contentComponent.addAndMakeVisible(&tipOfTheDayLabel);
    contentComponent.addAndMakeVisible(&separatorTop);
    contentComponent.addAndMakeVisible(&separatorBottom);
    contentComponent.addAndMakeVisible(&text);
    contentComponent.addAndMakeVisible(&showSplashToggleButton);
    contentComponent.addAndMakeVisible(&versionLabel);
    contentComponent.addAndMakeVisible(&okButton);
    contentComponent.addAndMakeVisible(&nextTipButton);

    enterModalState(true, nullptr, false);
    setVisible(true);

    // Shows a tip.
    const auto tip = tipsController.getRandomTip();
    displayTip(tip);
}


// Component method implementations.
// ==========================================================

void SplashTips::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    constexpr auto logoHeight = 200;
    const auto logoWidth = width;
    constexpr auto labelHeights = 25;
    constexpr auto labelMargin = margins;
    constexpr auto verticalMargin = margins;
    constexpr auto okButtonWidth = 90;
    constexpr auto nextTipButtonWidth = 110;

    // First, the logo at the top.
    logoImage.setBounds((width - logoWidth) / 2, 20, logoWidth, logoHeight);

    // Then the checkbox/version/OK button at the bottom.
    showSplashToggleButton.setBounds(labelMargin, height - labelHeights - verticalMargin, 200, labelHeights);

    versionLabel.setBounds(width - versionLabelWidth - labelMargin, verticalMargin, versionLabelWidth, labelHeights);

    okButton.setBounds((width - okButtonWidth) / 2, height - labelHeights - verticalMargin, okButtonWidth, labelHeights);
    nextTipButton.setBounds(width - nextTipButtonWidth - margins, okButton.getY(), nextTipButtonWidth, labelHeights);

    ResizableWindow::resized();         // Required by the parent.
}

bool SplashTips::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) {
        exitSplash();
        return true;
    }

    return Component::keyPressed(key);
}


// ==========================================================

void SplashTips::exitSplash() const noexcept
{
    // Updates the "show on start up" preference.
    const auto showOnStartUp = showSplashToggleButton.getToggleState();
    PreferencesManager::getInstance().setShowSplash(showOnStartUp);

    callback();
}

void SplashTips::showNextTip() noexcept
{
    const auto tip = tipsController.getNextTip();
    displayTip(tip);
}

void SplashTips::displayTip(const Tip& tip) noexcept
{
    tipZippedImages.reset();
    tipImage.reset();

    const auto availableWidth = getWidth() - (2 * margins);       // Will take as much as possible.
    constexpr auto left = margins;

    tipOfTheDayLabel.setBounds(left, logoImage.getBottom() + (margins * 2), availableWidth, LookAndFeelConstants::labelsHeight);
    tipOfTheDayLabel.setJustificationType(juce::Justification::centredTop);
    constexpr auto separatorMargins = 100;
    constexpr auto separatorHeight = 1;
    constexpr auto separatorLeft = left + separatorMargins;
    const auto separatorWidth = availableWidth - (2 * separatorMargins);
    separatorTop.setBounds(separatorLeft, tipOfTheDayLabel.getBottom(), separatorWidth, separatorHeight);
    separatorBottom.setBounds(separatorLeft, okButton.getY() - (margins * 2), separatorWidth, separatorHeight);

    const auto tipY = separatorTop.getBottom() + margins;
    const auto tipHeight = separatorBottom.getY() - tipY - margins - separatorHeight;

    const auto isTipImage = tip.getImageName().isNotEmpty();
    const auto isTipVideo = tip.getZippedVideoName().isNotEmpty();
    const auto isTipMedia = isTipImage || isTipVideo;

    // The text.
    const auto& tipText = tip.getText();
    text.setText(tipText, juce::NotificationType::dontSendNotification);
    text.setJustificationType(isTipMedia ? juce::Justification::centredLeft : juce::Justification::centred);
    text.setMinimumHorizontalScale(1.0F);
    constexpr auto textX = left;

    auto tipMediaWidth = availableWidth / 2;    // By default.
    const auto forcedMediaWidth = tip.getImageWidth();
    if (isTipMedia && (forcedMediaWidth > 0)) {
        tipMediaWidth = forcedMediaWidth;
    } else if (!isTipMedia) {
        tipMediaWidth = 0;
    }
    const auto textWidth = availableWidth - tipMediaWidth - (tipMediaWidth > 0 ? margins : 0);
    const auto tipMediaX = availableWidth - tipMediaWidth;
    const auto textHeight = tipHeight;
    const auto textY = tipY;

    text.setBounds(textX, textY, textWidth, textHeight);

    if (isTipImage) {
        // Displays the tip image.
        tipImage = std::make_unique<juce::ImageComponent>();
        tipImage->setBounds(tipMediaX, tipY, tipMediaWidth, tipHeight);
        tipImage->setImagePlacement(juce::RectanglePlacement::Flags::onlyReduceInSize);
        contentComponent.addAndMakeVisible(*tipImage);

        const auto imageToDisplay = juce::ImageFileFormat::loadFrom(tip.getImageName());
        tipImage->setImage(imageToDisplay);
    } else if (isTipVideo) {
        // Creates the tip video.
        const auto& zippedVideoNamePath = tip.getZippedVideoName();
        tipZippedImages = std::make_unique<ZippedImages>(zippedVideoNamePath, videoFrameDurationMs);
        tipZippedImages->setBounds(tipMediaX, tipY, tipMediaWidth, tipHeight);
        contentComponent.addAndMakeVisible(*tipZippedImages);

        // Displays the tip image.
        tipZippedImages->show();
    }
}

}   // namespace arkostracker
