#include "ThemeColors.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

ThemeColors& ThemeColors::getInstance()
{
    static ThemeColors instance;         // Uses "magic static". Lazy and thread-safe.
    return instance;
}

ThemeColors::ThemeColors() noexcept :
        colorIdToThemeColor()
{
    auto category = ColorCategory::general;
    addColor(juce::Label::ColourIds::textColourId, category, juce::translate("Text"));
    addColor(LookAndFeelConstants::Colors::dialogBackground, category, juce::translate("Windows background"));
    addColor(LookAndFeelConstants::Colors::dialogBorder, category, juce::translate("Dialogs border"));
    addColor(LookAndFeelConstants::Colors::titleBarBackground, category, juce::translate("Windows header"));
    addColor(LookAndFeelConstants::Colors::interspace, category, juce::translate("Interspace"));
    addColor(juce::ScrollBar::ColourIds::thumbColourId, category, juce::translate("Scroll bar"));
    addColor(LookAndFeelConstants::Colors::loopStartEnd, category, juce::translate("Loop bar"));
    addColor(juce::TooltipWindow::ColourIds::backgroundColourId, category, juce::translate("Tooltip background"));
    addColor(juce::TooltipWindow::ColourIds::textColourId, category, juce::translate("Tooltip text"));

    category = ColorCategory::panels;
    addColor(LookAndFeelConstants::Colors::panelBackgroundFocused, category, juce::translate("Background, focused"));
    addColor(LookAndFeelConstants::Colors::panelBackgroundUnfocused, category, juce::translate("Background, unfocused"));
    addColor(LookAndFeelConstants::Colors::panelBorderFocused, category, juce::translate("Border, focused"));
    addColor(LookAndFeelConstants::Colors::panelBorderUnfocused, category, juce::translate("Border, unfocused"));
    addColor(LookAndFeelConstants::Colors::panelTabIcon, category, juce::translate("Tab icon"));
    addColor(LookAndFeelConstants::Colors::panelTabBackgroundUnselected, category, juce::translate("Tab background, unselected"));
    addColor(LookAndFeelConstants::Colors::panelTabBackgroundSelected, category, juce::translate("Tab background, selected"));
    addColor(LookAndFeelConstants::Colors::panelHandleMouseOver, category, juce::translate("Resize handle with mouse over"));

    category = ColorCategory::buttons;
    addColor(juce::TextButton::ColourIds::textColourOnId, category, juce::translate("Text, highlighted"));
    addColor(juce::TextButton::ColourIds::textColourOffId, category, juce::translate("Text"));
    addColor(LookAndFeelConstants::Colors::buttonBorder, category, juce::translate("Border around"));
    addColor(LookAndFeelConstants::Colors::button, category, juce::translate("Background"));
    addColor(LookAndFeelConstants::Colors::buttonMouseOver, category, juce::translate("Background, when mouse over"));
    addColor(LookAndFeelConstants::Colors::buttonClicked, category, juce::translate("Background, when clicked"));

    category = ColorCategory::toolbars;
    addColor(LookAndFeelConstants::Colors::menuBarText, category, juce::translate("Text"));
    addColor(LookAndFeelConstants::Colors::menuBarTextHighlight, category, juce::translate("Text, if selected"));
    addColor(LookAndFeelConstants::Colors::menuBar, category, juce::translate("Background"));
    addColor(LookAndFeelConstants::Colors::menuBarTextBackgroundHighlight, category, juce::translate("Background, if selected"));
    addColor(juce::PopupMenu::ColourIds::textColourId, category, juce::translate("Popup menu text"));
    addColor(juce::PopupMenu::ColourIds::highlightedTextColourId, category, juce::translate("Popup menu text, if selected"));
    addColor(juce::PopupMenu::ColourIds::backgroundColourId, category, juce::translate("Popup menu, background"));
    addColor(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, category, juce::translate("Popup menu, background, if selected"));

    category = ColorCategory::lists;
    addColor(LookAndFeelConstants::Colors::arpeggioListItemBackgroundEven, category, juce::translate("Arpeggio background, even"));
    addColor(LookAndFeelConstants::Colors::arpeggioListItemBackgroundOdd, category, juce::translate("Arpeggio background, odd"));
    addColor(LookAndFeelConstants::Colors::arpeggioListItemBackgroundSelected, category, juce::translate("Arpeggio background, selected"));
    addColor(LookAndFeelConstants::Colors::pitchListItemBackgroundEven, category, juce::translate("Pitch background, even"));
    addColor(LookAndFeelConstants::Colors::pitchListItemBackgroundOdd, category, juce::translate("Pitch background, odd"));
    addColor(LookAndFeelConstants::Colors::pitchListItemBackgroundSelected, category, juce::translate("Pitch background, selected"));
    addColor(LookAndFeelConstants::Colors::instrumentListItemBackgroundEven, category, juce::translate("Instrument background, even"));
    addColor(LookAndFeelConstants::Colors::instrumentListItemBackgroundOdd, category, juce::translate("Instrument background, odd"));
    addColor(LookAndFeelConstants::Colors::instrumentListItemBackgroundSelected, category, juce::translate("Instrument background, selected"));
    addColor(LookAndFeelConstants::Colors::listItemText, category, juce::translate("Text"));
    addColor(LookAndFeelConstants::Colors::listItemTextSelected, category, juce::translate("Text, selected item"));

    category = ColorCategory::meters;
    addColor(LookAndFeelConstants::Colors::metersBackground, category, juce::translate("Background"));
    addColor(LookAndFeelConstants::Colors::metersLabel, category, juce::translate("Texts"));
    addColor(LookAndFeelConstants::Colors::metersLabelBackground, category, juce::translate("Text background"));
    addColor(LookAndFeelConstants::Colors::metersShape, category, juce::translate("Drawings"));
    addColor(LookAndFeelConstants::Colors::metersScaleLines, category, juce::translate("Scale lines"));
    addColor(LookAndFeelConstants::Colors::metersMuteFilter, category, juce::translate("Filter when muted"));       // No alpha! Else "behind" is shown.

    category = ColorCategory::linker;
    addColor(LookAndFeelConstants::Colors::linkerPlayCursorTimeline, category, juce::translate("Timeline and play cursor"));
    addColor(LookAndFeelConstants::Colors::linkerPositionWithinSongRangeBackground, category, juce::translate("Background within song limits"));
    addColor(LookAndFeelConstants::Colors::linkerPositionOutsideSongRangeBackground, category, juce::translate("Background outside song limits"));
    addColor(LookAndFeelConstants::Colors::linkerPositionBorder, category, juce::translate("Border"));
    addColor(LookAndFeelConstants::Colors::linkerPositionCurrentlyPlayedBorder, category, juce::translate("Border if position is played"));

    category = ColorCategory::editorWithBars;
    addColor(LookAndFeelConstants::Colors::barSoundTypeBackground, category, juce::translate("Sound bar, background"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeNoSoftNoHard, category, juce::translate("Sound bar, no soft no hard"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeSoft, category, juce::translate("Sound bar, software only"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeHard, category, juce::translate("Sound bar, hardware only"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeSoftToHard, category, juce::translate("Sound bar, soft to hard"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeHardToSoft, category, juce::translate("Sound bar, hard to soft"));
    addColor(LookAndFeelConstants::Colors::barSoundTypeSoftAndHard, category, juce::translate("Sound bar, soft and hard"));
    addColor(LookAndFeelConstants::Colors::barEnvelopeBackground, category, juce::translate("Envelope bar, background"));
    addColor(LookAndFeelConstants::Colors::barEnvelopeSoft, category, juce::translate("Envelope bar, software envelope"));
    addColor(LookAndFeelConstants::Colors::barEnvelopeHard, category, juce::translate("Envelope bar, hardware envelope"));
    addColor(LookAndFeelConstants::Colors::barNoiseBackground, category, juce::translate("Noise bar, background"));
    addColor(LookAndFeelConstants::Colors::barNoise, category, juce::translate("Noise bar"));
    addColor(LookAndFeelConstants::Colors::barPeriodBackground, category, juce::translate("Period bar, background"));
    addColor(LookAndFeelConstants::Colors::barPeriod, category, juce::translate("Period bar"));
    addColor(LookAndFeelConstants::Colors::barArpeggioNoteBackground, category, juce::translate("Arpeggio note bar, background"));
    addColor(LookAndFeelConstants::Colors::barArpeggioNote, category, juce::translate("Arpeggio note bar"));
    addColor(LookAndFeelConstants::Colors::barArpeggioOctaveBackground, category, juce::translate("Arpeggio octave bar, background"));
    addColor(LookAndFeelConstants::Colors::barArpeggioOctave, category, juce::translate("Arpeggio octave bar"));
    addColor(LookAndFeelConstants::Colors::barPitchBackground, category, juce::translate("Pitch bar, background"));
    addColor(LookAndFeelConstants::Colors::barPitch, category, juce::translate("Pitch bar"));
    addColor(LookAndFeelConstants::Colors::autoSpreadLoop, category, juce::translate("Auto-spread loop bar"));

    category = ColorCategory::patternViewer;
    addColor(LookAndFeelConstants::Colors::patternViewerBackground, category, juce::translate("Background"));
    addColor(LookAndFeelConstants::Colors::patternViewerNoNote, category, juce::translate("Absent note"));
    addColor(LookAndFeelConstants::Colors::patternViewerError, category, juce::translate("Error"));
    addColor(LookAndFeelConstants::Colors::patternViewerNoEffect, category, juce::translate("Absent effect"));
    addColor(LookAndFeelConstants::Colors::patternViewerArpeggioEffect, category, juce::translate("Arpeggio effects"));
    addColor(LookAndFeelConstants::Colors::patternViewerPitchEffect, category, juce::translate("Pitch effects"));
    addColor(LookAndFeelConstants::Colors::patternViewerVolumeEffect, category, juce::translate("Volume effects"));
    addColor(LookAndFeelConstants::Colors::patternViewerResetEffect, category, juce::translate("Reset effect"));
    addColor(LookAndFeelConstants::Colors::patternViewerInstrumentSpeedEffect, category, juce::translate("Instrument speed effect"));
    addColor(LookAndFeelConstants::Colors::patternViewerCursorTextHighlight, category, juce::translate("Text if cursor on it"));
    addColor(LookAndFeelConstants::Colors::patternViewerCursorBackground, category, juce::translate("Cursor"));
    addColor(LookAndFeelConstants::Colors::patternViewerSelection, category, juce::translate("Selection"));
    addColor(LookAndFeelConstants::Colors::patternViewerCursorLineBackground, category, juce::translate("Background if cursor on line"));
    addColor(LookAndFeelConstants::Colors::patternViewerCursorLineHighlightText, category, juce::translate("Text if cursor on line"));
    addColor(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundPrimary, category, juce::translate("Background if primary highlight"));
    addColor(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundSecondary, category, juce::translate("Background if secondary highlight"));
    addColor(LookAndFeelConstants::Colors::patternViewerHighlightTextPrimary, category, juce::translate("Text if primary highlight"));
    addColor(LookAndFeelConstants::Colors::patternViewerHighlightTextSecondary, category, juce::translate("Text if secondary highlight"));
    addColor(LookAndFeelConstants::Colors::patternViewerPlayedLineBackground, category, juce::translate("Background if played line"));
    addColor(LookAndFeelConstants::Colors::patternViewerTrackBorder, category, juce::translate("Border besides tracks"));

    addColor(LookAndFeelConstants::Colors::patternViewerMetersSoftwareBar, category, juce::translate("Vu-meter software bar"));
    addColor(LookAndFeelConstants::Colors::patternViewerMetersHardwareBar, category, juce::translate("Vu-meter hardware bar"));
    addColor(LookAndFeelConstants::Colors::patternViewerNoiseBar, category, juce::translate("Vu-meter noise bar"));

    addColor(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOn, category, juce::translate("Track header background for a On channel"));
    addColor(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOff, category, juce::translate("Track header background for a Off channel"));
    addColor(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOn, category, juce::translate("Track header channel if On"));
    addColor(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOff, category, juce::translate("Track header channel if Off"));
    addColor(LookAndFeelConstants::Colors::patternViewerLineTrackHeaderBackground, category, juce::translate("Line number header background"));
    addColor(LookAndFeelConstants::Colors::patternViewerLineTrackText, category, juce::translate("Line number text"));
    addColor(LookAndFeelConstants::Colors::patternViewerSpeedTrackHeaderBackground, category, juce::translate("Speed track header background"));
    addColor(LookAndFeelConstants::Colors::patternViewerSpeedTrackText, category, juce::translate("Speed track text"));
    addColor(LookAndFeelConstants::Colors::patternViewerEventTrackHeaderBackground, category, juce::translate("Event track header background"));
    addColor(LookAndFeelConstants::Colors::patternViewerEventTrackText, category, juce::translate("Event track text"));

    // Missing any color from the LookAndFeel? We subtract the colors that are JUCE native!
    jassert(colorIdToThemeColor.size() - 10 == static_cast<size_t>(LookAndFeelConstants::Colors::count));
}

const std::map<int, ThemeColor>& ThemeColors::getColorIdToThemeColor() const noexcept
{
    return colorIdToThemeColor;
}

void ThemeColors::addColor(int colorId, const ColorCategory colorCategory, juce::String displayedName, bool allowTransparency) noexcept
{
    colorIdToThemeColor.insert(std::make_pair(colorId, ThemeColor(allowTransparency, colorId, colorCategory, std::move(displayedName))));
}

void ThemeColors::addColor(const LookAndFeelConstants::Colors colorIdEnum, const ColorCategory colorCategory, juce::String displayedName, bool allowTransparency) noexcept
{
    const auto colorId = static_cast<int>(colorIdEnum);
    addColor(colorId, colorCategory, std::move(displayedName), allowTransparency);
}


// ====================================================================================

ThemeColor::ThemeColor(const bool pAllowTransparency, const int pColorId, const ColorCategory pCategory,
        juce::String pDisplayedName) :
        allowTransparency(pAllowTransparency),
        colorId(pColorId),
        category(pCategory),
        displayedName(std::move(pDisplayedName))
{
}

}   // namespace arkostracker
