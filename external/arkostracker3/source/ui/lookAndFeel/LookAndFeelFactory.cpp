#include "LookAndFeelFactory.h"

#include <unordered_map>

#include "../../app/preferences/PreferencesManager.h"
#include "LookAndFeelConstants.h"
#include "theme/ThemeBlackAndWhiteBuilder.h"
#include "theme/ThemeCrimsonBuilder.h"
#include "theme/ThemeDefaultBuilder.h"
#include "theme/ThemeGrayBuilder.h"
#include "theme/ThemeGreenBuilder.h"
#include "theme/ThemeWhiteBuilder.h"

namespace arkostracker 
{

std::vector<int> LookAndFeelFactory::getNativeThemeIds() noexcept
{
    // This is the order shown in the Theme Editor.
    static const std::vector ids = {
            static_cast<int>(LookAndFeelConstants::ThemeId::defaultAt3),
            static_cast<int>(LookAndFeelConstants::ThemeId::gray),
            static_cast<int>(LookAndFeelConstants::ThemeId::crimson),
            static_cast<int>(LookAndFeelConstants::ThemeId::green),
            static_cast<int>(LookAndFeelConstants::ThemeId::blackAndWhite),
            static_cast<int>(LookAndFeelConstants::ThemeId::white),
    };
    static_assert(static_cast<int>(LookAndFeelConstants::ThemeId::countNative) == 6,
                  "Did you forget to add a native theme? Check LookAndFeelConstants::ThemeId.");

    return ids;
}

std::unique_ptr<StoredLookAndFeel> LookAndFeelFactory::retrieveLookAndFeel(const int themeId) noexcept
{
    std::unique_ptr<StoredLookAndFeel> lookAndFeelToUse;

    // Is it a native one?
    if (themeId < static_cast<int>(LookAndFeelConstants::ThemeId::startIdCustomTheme)) {
        // Builds a native theme.
        lookAndFeelToUse = LookAndFeelFactory::buildNativeThemeFromId(themeId);
    } else {
        // Builds a custom theme, from the XML.
        lookAndFeelToUse = PreferencesManager::getInstance().getCustomTheme(themeId);
    }
    // Security.
    if (lookAndFeelToUse == nullptr) {
        jassertfalse;       // No theme could be found, strange.
        lookAndFeelToUse = ThemeDefaultBuilder::build();
    }

    return lookAndFeelToUse;
}

std::unique_ptr<StoredLookAndFeel> LookAndFeelFactory::buildNativeThemeFromId(const int id) noexcept
{
    static const std::unordered_map<int, std::function<std::unique_ptr<StoredLookAndFeel>()>> idToFunc =
            {
                    { static_cast<int>(LookAndFeelConstants::ThemeId::defaultAt3),    [] { return ThemeDefaultBuilder::build(); }},
                    { static_cast<int>(LookAndFeelConstants::ThemeId::gray),    [] { return ThemeGrayBuilder::build(); }},
                    { static_cast<int>(LookAndFeelConstants::ThemeId::white),         [] { return ThemeWhiteBuilder::build(); }},
                    { static_cast<int>(LookAndFeelConstants::ThemeId::crimson),       [] { return ThemeCrimsonBuilder::build(); }},
                    { static_cast<int>(LookAndFeelConstants::ThemeId::green),         [] { return ThemeGreenBuilder::build(); }},
                    { static_cast<int>(LookAndFeelConstants::ThemeId::blackAndWhite), [] { return ThemeBlackAndWhiteBuilder::build(); }}
            };
    jassert(idToFunc.size() == static_cast<size_t>(LookAndFeelConstants::ThemeId::countNative));       // Forgot to declare a theme above?

    // Returns the method from the ID, if any.
    const auto iterator = idToFunc.find(id);
    if (iterator == idToFunc.cend() || (iterator->second == nullptr)) {
        return ThemeDefaultBuilder::build();
    }

    return iterator->second();
}

std::unique_ptr<StoredLookAndFeel> LookAndFeelFactory::buildStoredLookAndFeelFromGivenLookAndFeel(const juce::LookAndFeel& lookAndFeel,
                                                                                                  int profileId, const juce::String& name, bool readOnly) noexcept
{
    return std::make_unique<StoredLookAndFeel>(
            profileId,
            name,
            readOnly,

            StoredLookAndFeel::GeneralColors(
                    getColourArgb(lookAndFeel, juce::Label::ColourIds::textColourId),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::dialogBackground),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::dialogBorder),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::interspace),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::titleBarBackground),

                    getColourArgb(lookAndFeel, juce::ScrollBar::ColourIds::thumbColourId),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::loopStartEnd),

                    getColourArgb(lookAndFeel, juce::TooltipWindow::ColourIds::backgroundColourId),
                    getColourArgb(lookAndFeel, juce::TooltipWindow::ColourIds::textColourId)
            ),

            StoredLookAndFeel::PanelColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelBackgroundFocused),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelBackgroundUnfocused),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelBorderFocused),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelBorderUnfocused),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelTabIcon),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelTabBackgroundUnselected),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelTabBackgroundSelected),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::panelHandleMouseOver)
            ),

            StoredLookAndFeel::ButtonColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::buttonBorder),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::button),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::buttonClicked),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::buttonMouseOver),
                    getColourArgb(lookAndFeel, juce::TextButton::ColourIds::textColourOnId),
                    getColourArgb(lookAndFeel, juce::TextButton::ColourIds::textColourOffId)
            ),

            StoredLookAndFeel::ToolbarColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::menuBar),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::menuBarText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::menuBarTextHighlight),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::menuBarTextBackgroundHighlight),
                    getColourArgb(lookAndFeel, juce::PopupMenu::ColourIds::textColourId),
                    getColourArgb(lookAndFeel, juce::PopupMenu::ColourIds::highlightedTextColourId),
                    getColourArgb(lookAndFeel, juce::PopupMenu::ColourIds::backgroundColourId),
                    getColourArgb(lookAndFeel, juce::PopupMenu::ColourIds::highlightedBackgroundColourId)
            ),

            StoredLookAndFeel::ListColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::arpeggioListItemBackgroundEven),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::arpeggioListItemBackgroundOdd),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::arpeggioListItemBackgroundSelected),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::pitchListItemBackgroundEven),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::pitchListItemBackgroundOdd),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::pitchListItemBackgroundSelected),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::instrumentListItemBackgroundEven),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::instrumentListItemBackgroundOdd),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::instrumentListItemBackgroundSelected),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::listItemText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::listItemTextSelected)
            ),

            StoredLookAndFeel::MeterColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersScaleLines),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersLabelBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersLabel),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersShape),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::metersMuteFilter)
            ),

            StoredLookAndFeel::LinkerColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::linkerPlayCursorTimeline),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::linkerPositionWithinSongRangeBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::linkerPositionOutsideSongRangeBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::linkerPositionBorder),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::linkerPositionCurrentlyPlayedBorder)
            ),

            StoredLookAndFeel::EditorWithBarsColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeNoSoftNoHard),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeSoft),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeHard),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeSoftToHard),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeHardToSoft),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barSoundTypeSoftAndHard),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barEnvelopeBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barEnvelopeSoft),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barEnvelopeHard),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barNoiseBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barNoise),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barPeriodBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barPeriod),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barArpeggioNoteBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barArpeggioNote),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barArpeggioOctaveBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barArpeggioOctave),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barPitchBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::barPitch),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::autoSpreadLoop)
            ),

            StoredLookAndFeel::PatternViewerColors(
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerNoNote),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerError),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerNoEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerArpeggioEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerPitchEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerVolumeEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerResetEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerInstrumentSpeedEffect),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerCursorTextHighlight),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerCursorBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerSelection),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerCursorLineBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerCursorLineHighlightText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundPrimary),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundSecondary),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerHighlightTextPrimary),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerHighlightTextSecondary),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerTrackBorder),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOn),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOff),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOn),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOff),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerLineTrackHeaderBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerLineTrackText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerSpeedTrackHeaderBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerSpeedTrackText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerEventTrackHeaderBackground),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerEventTrackText),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerPlayedLineBackground),

                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerMetersSoftwareBar),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerMetersHardwareBar),
                    getColourArgb(lookAndFeel, LookAndFeelConstants::Colors::patternViewerNoiseBar)
            )
    );
}

}   // namespace arkostracker
