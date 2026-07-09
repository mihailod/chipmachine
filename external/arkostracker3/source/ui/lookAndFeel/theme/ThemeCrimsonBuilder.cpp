#include "ThemeCrimsonBuilder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include "../LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String ThemeCrimsonBuilder::themeName = juce::translate("Crimson king");          // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

std::unique_ptr<StoredLookAndFeel> ThemeCrimsonBuilder::build() noexcept
{
    constexpr auto text = 0xffffffff;
    constexpr auto textSecondary = 0xffc0c0c0;          // Background highlight, or not selected text.

    constexpr auto primaryMain = 0xff2e0600;            // For backgrounds (dialog, panel unfocused).
    constexpr auto primaryMainLighter = 0xff4e260f;     // Panel focused.

    constexpr auto secondaryMain = 0xff6e462f;
    const auto patternViewerChannelOnOff = juce::Colour(secondaryMain).darker(0.15F).getARGB();

    const auto interspace = juce::Colour(primaryMain).darker(0.4F).getARGB();

    constexpr auto dialogBackground = primaryMain;
    constexpr auto panelBackgroundUnfocused = primaryMain;
    constexpr auto panelBackgroundFocused = primaryMainLighter;

    constexpr auto panelBorderFocused = 0xffae864f;
    constexpr auto panelBorderUnfocused = secondaryMain;

    constexpr auto panelHandleMouseOver = secondaryMain;

    constexpr auto loop = 0xffc0c000;
    constexpr auto position = 0xff401010;

    constexpr auto buttonBackground = 0xff401000;
    const auto buttonBorder = interspace;
    const auto buttonBackgroundMouseOver = juce::Colour(buttonBackground).brighter(0.3F).getARGB();
    const auto buttonBackgroundClicked = juce::Colour(buttonBackground).brighter(0.7F).getARGB();

    constexpr auto barBackground = 0xff300000;
    constexpr auto barSoft = 0xff805000;
    constexpr auto barHard = 0xff807000;
    constexpr auto barSoundTypeBackground = barBackground;

    constexpr auto barSoundTypeSoft = barSoft;
    constexpr auto barSoundTypeHard = barHard;

    constexpr auto barNoiseBackground = 0xff200000;
    constexpr auto barNoise = barHard;

    constexpr auto barPrimaryBackground = 0xff301040;
    constexpr auto barPrimary = 0xff503020;

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
                    0xff0c2002,
                    0xff0c3002,
                    0xff2c5022,
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
                    0xff3030ff,
                    0xffffffff,
                    0xff701010,
                    0xffff2020,
                    0xff401010,
                    0xffd0d0d0,
                    0xff280000,
                    0xff381818,
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
                    0xffd08080,
                    secondaryMain,
                    0xff8080d0,
                    0xff4d00af,

                    0xffffffff,
                    0xffff00ff,
                    0xffff0000
            )
    );
}

}   // namespace arkostracker
