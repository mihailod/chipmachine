#include "ThemeBlackAndWhiteBuilder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include "../LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String ThemeBlackAndWhiteBuilder::themeName = juce::translate("Black or white");          // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

std::unique_ptr<StoredLookAndFeel> ThemeBlackAndWhiteBuilder::build() noexcept
{
    constexpr auto loopsAndPosition = 0xffc4c4c4;

    constexpr auto text = 0xffffffff;
    constexpr auto textSecondary = 0xffc0c0c0;      // Background, or not selected text.

    constexpr auto primaryMain = 0xff000000;        // For backgrounds (dialog, panel unfocused).
    constexpr auto primaryMainLighter = 0xff101010; // Panel focused.

    constexpr auto secondaryMain = 0xff303030;
    constexpr auto patternViewerChannelOnOff = primaryMainLighter;

    constexpr auto interspace = 0xff000000;

    constexpr auto dialogBackground = primaryMain;
    constexpr auto panelBackgroundUnfocused = primaryMain;
    constexpr auto panelBackgroundFocused = primaryMainLighter;

    const auto panelBorderFocused = juce::Colour(panelBackgroundFocused).brighter(3.8F).getARGB();
    constexpr auto panelBorderUnfocused = secondaryMain;

    constexpr auto panelHandleMouseOver = secondaryMain;

    constexpr auto buttonBackground = secondaryMain;
    constexpr auto buttonBorder = interspace;
    const auto buttonBackgroundMouseOver = juce::Colour(buttonBackground).brighter(0.3F).getARGB();
    const auto buttonBackgroundClicked = juce::Colour(buttonBackground).brighter(0.7F).getARGB();

    const auto linkerPositionBorder = juce::Colour(secondaryMain).brighter(0.5F).getARGB();

    constexpr auto barBackground = 0xff202020;
    constexpr auto barSoft = 0xff808080;
    constexpr auto barHard = 0xffa0a0a0;
    constexpr auto barSoundTypeBackground = barBackground;

    constexpr auto barSoundTypeSoft = barSoft;
    constexpr auto barSoundTypeHard = barHard;

    constexpr auto barNoiseBackground = 0xff202020;
    constexpr auto barNoise = barHard;

    constexpr auto barPrimaryBackground = 0xff303030;
    constexpr auto barPrimary = 0xff808080;

    return std::make_unique<StoredLookAndFeel>(
            static_cast<int>(LookAndFeelConstants::ThemeId::blackAndWhite),
            themeName,
            true,

            StoredLookAndFeel::GeneralColors(
                    text,
                    dialogBackground,

                    secondaryMain,
                    interspace,
                    primaryMainLighter,

                    secondaryMain,
                    loopsAndPosition,

                    primaryMainLighter,
                    text
            ),

            StoredLookAndFeel::PanelColors(
                    panelBackgroundFocused,
                    panelBackgroundUnfocused,

                    panelBorderFocused,
                    panelBorderUnfocused,

                    text,
                    interspace,
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
                    primaryMain,
                    primaryMainLighter,
                    secondaryMain,
                    primaryMain,
                    primaryMainLighter,
                    secondaryMain,
                    primaryMain,
                    primaryMainLighter,
                    secondaryMain,
                    textSecondary,
                    text
            ),

            StoredLookAndFeel::MeterColors(
                    0xff000000,
                    0xff404040,
                    0x80808080,
                    0xffe0e0e0,
                    0xfff0f0f0,
                    0xff101010
            ),

            StoredLookAndFeel::LinkerColors(
                    loopsAndPosition,
                    primaryMainLighter,
                    primaryMain,
                    linkerPositionBorder,
                    loopsAndPosition
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
                    loopsAndPosition
            ),

            StoredLookAndFeel::PatternViewerColors(
                    0xff000000,
                    0xff808080,
                    0xff808080,
                    0xff505050,
                    0xff808080,
                    0xffa0a0a0,
                    0xffc0c0c0,
                    0xffa0a0a0,
                    0xffa0a0a0,
                    0xffffffff,
                    0xff303030,
                    0xff404040,
                    0xff202020,
                    0xffd0d0d0,
                    0xff202020,
                    0xff282828,
                    0xffc0c0c0,
                    0xffffffff,
                    secondaryMain,

                    secondaryMain,
                    secondaryMain,
                    patternViewerChannelOnOff,
                    patternViewerChannelOnOff,
                    secondaryMain,
                    0xffc0c0c0,
                    secondaryMain,
                    0xffd0d0d0,
                    secondaryMain,
                    0xffd0d0d0,
                    0xff4d4d4d,

                    0xffffffff,
                    0xffa0a0a0,
                    0xffd0d0d0
            )
    );
}

}   // namespace arkostracker
