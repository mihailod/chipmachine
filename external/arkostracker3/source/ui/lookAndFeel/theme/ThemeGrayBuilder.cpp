#include "ThemeGrayBuilder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include "../LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String ThemeGrayBuilder::themeName = juce::translate("Gray");          // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

std::unique_ptr<StoredLookAndFeel> ThemeGrayBuilder::build() noexcept
{
    constexpr auto text = 0xffffffff;
    constexpr auto textSecondary = 0xffc0c0c0;          // Background highlight, or not selected text.

    constexpr auto primaryMain = 0xff404040;            // For backgrounds (dialog, panel unfocused).
    constexpr auto primaryMainLighter = 0xff606060;     // Panel focused.

    constexpr auto secondaryMain = 0xff808080;
    const auto patternViewerChannelOnOff = juce::Colour(secondaryMain).darker(0.15F).getARGB();

    const auto interspace = juce::Colour(0xff000000).getARGB();

    constexpr auto dialogBackground = primaryMain;
    constexpr auto panelBackgroundUnfocused = primaryMain;
    constexpr auto panelBackgroundFocused = primaryMainLighter;

    constexpr auto panelBorderFocused = 0xffc0c0c0;
    constexpr auto panelBorderUnfocused = secondaryMain;

    constexpr auto panelHandleMouseOver = secondaryMain;

    constexpr auto loop = 0xffc0c0c0;
    constexpr auto position = 0xffa0a0a0;

    constexpr auto buttonBackground = 0xff505050;
    const auto buttonBorder = interspace;
    const auto buttonBackgroundMouseOver = juce::Colour(buttonBackground).brighter(0.3F).getARGB();
    const auto buttonBackgroundClicked = juce::Colour(buttonBackground).brighter(0.7F).getARGB();

    constexpr auto barBackground = 0xff303030;
    constexpr auto barSoft = 0xff707870;
    constexpr auto barHard = 0xff606068;
    constexpr auto barSoundTypeBackground = barBackground;

    constexpr auto barSoundTypeSoft = barSoft;
    constexpr auto barSoundTypeHard = barHard;

    constexpr auto barNoiseBackground = 0xff202020;
    constexpr auto barNoise = 0xff606860;

    constexpr auto barPrimaryBackground = 0xff101010;
    constexpr auto barPrimary = 0xff505040;

    return std::make_unique<StoredLookAndFeel>(
            static_cast<int>(LookAndFeelConstants::ThemeId::crimson),
            themeName,
            true,

            StoredLookAndFeel::GeneralColors(
                    text,
                    dialogBackground,

                    secondaryMain,
                    interspace,
                    primaryMainLighter,

                    secondaryMain,
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
                    0xff303030,
                    0xff404040,
                    0xff505050,
                    0xff1c1c1c,
                    0xff2c2c2c,
                    0xff4c4c4c,
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
                    loop,
                    buttonBackground,
                    primaryMain,
                    position,
                    position
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
                    0xff000000,
                    0xff808080,
                    0xffff0000,
                    0xff505050,
                    0xff40e0e0,
                    0xff5050ff,
                    0xff40ff40,
                    0xffa0a0a0,
                    0xff707070,
                    0xffffffff,
                    0xff404040,
                    0xffd0d0d0,
                    0xff202020,
                    0xffd0d0d0,
                    0xff202020,
                    0xff303030,
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
                    0xff909090,
                    secondaryMain,
                    0xff808080,
                    0xff404040,

                    0xffffffff,
                    0xffff00ff,
                    0xffff0000
            )
    );
}

}   // namespace arkostracker
