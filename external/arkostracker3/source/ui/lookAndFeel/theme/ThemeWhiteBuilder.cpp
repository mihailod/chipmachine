#include "ThemeWhiteBuilder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include "../LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String ThemeWhiteBuilder::themeName = juce::translate("White bicycle");          // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

std::unique_ptr<StoredLookAndFeel> ThemeWhiteBuilder::build() noexcept
{
    constexpr auto text = 0xff000000;
    constexpr auto textSecondary = 0xff404040;          // Background highlight, or not selected text.

    constexpr auto primaryMain = 0xffc0c0c0;            // For backgrounds (dialog, panel unfocused).
    constexpr auto primaryMainLighter = 0xffe0e0e0;     // Panel focused.

    constexpr auto secondaryMain = 0xffffffff;
    const auto patternViewerChannelOnOff = juce::Colour(secondaryMain).darker(0.2F).getARGB();

    constexpr auto interspace = 0xffe0e0e0;

    constexpr auto dialogBackground = primaryMain;
    constexpr auto panelBackgroundUnfocused = primaryMain;
    constexpr auto panelBackgroundFocused = primaryMainLighter;

    constexpr auto panelBorderFocused = 0xff000000;
    constexpr auto panelBorderUnfocused = 0xff808080;

    constexpr auto scrollBar = 0xffa0a0a0;

    constexpr auto panelHandleMouseOver = secondaryMain;

    constexpr auto buttonBackground = 0xffa0a0a0;
    constexpr auto buttonBorder = panelBorderUnfocused;
    const auto buttonBackgroundMouseOver = juce::Colour(buttonBackground).darker(0.3F).getARGB();
    const auto buttonBackgroundClicked = juce::Colour(buttonBackground).darker(0.7F).getARGB();

    constexpr auto loop = 0xfff0f040;
    constexpr auto linkerPosition = 0xffc4c4c4;

    constexpr auto barBackground = 0xfff0f0f0;
    constexpr auto barSoft = 0xffa0a0a0;
    constexpr auto barHard = 0xffa0a0b0;
    constexpr auto barSoundTypeBackground = barBackground;

    constexpr auto barSoundTypeSoft = barSoft;
    constexpr auto barSoundTypeHard = barHard;

    constexpr auto barNoiseBackground = barBackground;
    constexpr auto barNoise = barHard;

    constexpr auto barPrimaryBackground = barBackground;
    constexpr auto barPrimary = barSoft;

    return std::make_unique<StoredLookAndFeel>(
            static_cast<int>(LookAndFeelConstants::ThemeId::white),
            themeName,
            true,

            StoredLookAndFeel::GeneralColors(
                    text,
                    dialogBackground,

                    secondaryMain,
                    interspace,
                    primaryMainLighter,

                    scrollBar,
                    loop,

                    primaryMainLighter,
                    text
            ),

            StoredLookAndFeel::PanelColors(
                    panelBackgroundFocused,
                    panelBackgroundUnfocused,

                    panelBorderFocused,
                    panelBorderUnfocused,

                    text,
                    primaryMain,
                    secondaryMain,

                    panelHandleMouseOver
            ),

            StoredLookAndFeel::ButtonColors(
                    buttonBorder,
                    buttonBackground,
                    buttonBackgroundClicked,
                    buttonBackgroundMouseOver,
                    text,
                    textSecondary
            ),

            StoredLookAndFeel::ToolbarColors(
                    secondaryMain,
                    text,
                    textSecondary,
                    primaryMain,
                    text,
                    textSecondary,
                    primaryMainLighter,
                    primaryMain
            ),

            StoredLookAndFeel::ListColors(
                    0xffcce0c2,
                    0xffdcf0d2,
                    0xffecffe2,
                    0xffbcbcdc,
                    0xffccccec,
                    0xffececff,
                    primaryMain,
                    primaryMainLighter,
                    secondaryMain,
                    textSecondary,
                    text
            ),

            StoredLookAndFeel::MeterColors(
                    0xffffffff,
                    0xffa0a0a0,
                    0x80808080,
                    text,
                    0xff101010,
                    0xffa0a0a0
            ),

            StoredLookAndFeel::LinkerColors(
                    loop,
                    buttonBackground,
                    primaryMain,
                    linkerPosition,
                    linkerPosition
            ),

            StoredLookAndFeel::EditorWithBarsColors(
                    barSoundTypeBackground,
                    barSoundTypeSoft,
                    barSoundTypeSoft,
                    barSoundTypeHard,
                    barSoundTypeHard,
                    barSoundTypeHard,
                    barSoundTypeHard,
                    barBackground,
                    barSoft,
                    barHard,
                    barNoiseBackground,
                    barNoise,
                    barPrimaryBackground,
                    barPrimary,
                    barPrimaryBackground,
                    barPrimary,
                    barPrimaryBackground,
                    barPrimary,
                    barPrimaryBackground,
                    barPrimary,
                    loop
            ),

            StoredLookAndFeel::PatternViewerColors(
                    0xfff0f0f0,
                    0xff303030,
                    0xffff0000,
                    0xff606060,
                    0xff40e0e0,
                    0xff5050ff,
                    0xff008000,
                    0xff303030,
                    0xff303080,
                    0xff000000,
                    0xffc0c0c0,
                    0xff20ff20,
                    0xff808080,
                    0xffd0d0d0,
                    0xffe0e0e0,
                    0xffd0d0d0,
                    0xff202020,
                    0xff000000,
                    secondaryMain,

                    secondaryMain,
                    secondaryMain,
                    patternViewerChannelOnOff,
                    patternViewerChannelOnOff,
                    secondaryMain,
                    text,
                    secondaryMain,
                    0xff300000,
                    secondaryMain,
                    0xff404070,
                    0xff606060,

                    text,
                    0xffff00ff,
                    0xffff0000
            )
    );
}

}   // namespace arkostracker
