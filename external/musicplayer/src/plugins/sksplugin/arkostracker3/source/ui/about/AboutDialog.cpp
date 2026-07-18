#include "AboutDialog.h"

#include "../../ProjectInfo.h"          // Generated file in the build folder.
#include "../lookAndFeel/CustomLookAndFeel.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "BinaryData.h"

namespace arkostracker
{

AboutDialog::AboutDialog(const std::function<void()>& pExitCallback) noexcept :
        ModalDialog(juce::translate("About"), 560, 650,
                    pExitCallback,
                    pExitCallback),
        logoImage(),
        welcomeLabel(juce::String(), juce::translate("Welcome to")),
        titleLabel(juce::String(), juce::translate("Arkos Tracker")),
        subtitleLabel(juce::String(), juce::translate("The ultimate tracker for CPC, ZX Spectrum, MSX,\n"
                                          "Atari ST/XE/XL, Oric, Apple 2, and Vectrex!")),
        versionLabel(juce::String(), juce::translate("version ") + projectinfo::applicationVersion),
        mainText(juce::String(), juce::CharPointer_UTF8(R"(Code and design:
Julien Nevo a.k.a. Targhan/Arkos

Arkos Tracker is powered by JUCE, the best cross-platform framework ever!

Also makes the use of:
FAP encoder by Hicks/Vanity and Gozeur/Contrast
Rasm by Roudoudou/Flower Corp
Serial library by William Woodall and John Harrison
LZH depack code by Haruhiko Okumura (1991) and Kerwin F. Medina (1996)
C++ wrapper around the LZH depacker by Arnaud Carré (Leonard)
Z80 emulator (SUZUKI PLAN) by Yoji Suzuki

Special thanks to:
Grim for the volume measurements on CPC
Benjamin Gérard for the ones on Atari ST
GGN for the Atari ST 68000 AKY player
Arnaud Cocquière for the Apple 2 / Oric 6502 AKY player
Krzysztof Dudek for the Atari 8 bits 6502 AKY player
Leïla R. for the logo
LyonsType for this font

Something to report? Remarks, bugs (surely), praises (of course)?
Please send an e-mail to contact@julien-nevo.com.

Pay also a visit to arkostracker.cpcscene.net!)"))
{
    auto area = getUsableModalDialogBounds();
    const auto width = area.getWidth();
    const auto height = area.getHeight();

    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto titleHeight = 40;
    constexpr auto titleSize = 50.0F;

    constexpr auto versionSizeDelta = -3.0F;
    constexpr auto subtitleSizeDelta = 5.0F;

    constexpr auto imageSize = 256;

    // The background image.
    logoImage.setImage(juce::ImageFileFormat::loadFrom(BinaryData::Logo512_png, BinaryData::Logo512_pngSize), juce::RectanglePlacement::centred);
    logoImage.setBounds((width - imageSize) / 2, (height - imageSize) / 3, imageSize, imageSize);
    logoImage.setAlpha(0.3F);
    addComponentToModalDialog(logoImage);

    const auto& currentLookAndFeel = dynamic_cast<CustomLookAndFeel&>(juce::LookAndFeel::getDefaultLookAndFeel());
    const auto& baseFont = currentLookAndFeel.getBaseFont();
    const auto baseFontHeight = baseFont.getHeight();

    // Welcome.
    welcomeLabel.setBounds(area.removeFromTop(labelsHeight));

    // Big title.
    titleLabel.setBounds(area.removeFromTop(titleHeight));
    const auto fontTitle = currentLookAndFeel.getBaseFont().withHeight(titleSize).withStyle(juce::Font::bold);
    titleLabel.setFont(fontTitle);

    // Version number.
    versionLabel.setBounds(area.removeFromTop(30));
    const auto fontVersion = currentLookAndFeel.getBaseFont().withHeight(baseFontHeight + versionSizeDelta);
    versionLabel.setFont(fontVersion);

    // Subtitle.
    subtitleLabel.setBounds(area.removeFromTop(static_cast<int>(labelsHeight * 2.5)));
    const auto fontSubtitle = currentLookAndFeel.getBaseFont().withHeight(baseFontHeight + subtitleSizeDelta);
    subtitleLabel.setFont(fontSubtitle);

    // Main text.
    mainText.setBounds(area.removeFromTop((labelsHeight * 17) + 5));

    // Adds the labels, and sets their justification.
    for (auto* label : { &welcomeLabel, &titleLabel, &subtitleLabel, &versionLabel, &mainText }) {
        label->setJustificationType(juce::Justification::horizontallyCentred);
        addComponentToModalDialog(*label);
    }
    versionLabel.setJustificationType(juce::Justification::centredTop);

    // Sets custom fonts.
    //for (auto* label : { &welcomeLabel, &titleLabel, &subtitleLabel, &versionLabel }) {
    for (auto* label : { &titleLabel, &subtitleLabel, &versionLabel }) {
        label->getProperties().set(LookAndFeelConstants::labelPropertyUseCustomFont, true);
    }
}

}   // namespace arkostracker
