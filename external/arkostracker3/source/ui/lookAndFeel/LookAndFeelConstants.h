#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Constants for our the LookAndFeel. Here are only the new colors defined by AT3. JUCE colors are already within JUCE classes. */
class LookAndFeelConstants
{
public:
    /** Prevents instantiation. */
    LookAndFeelConstants() = delete;

    static const int margins;                                   // A generic margin.
    static const int smallMargins;
    static const int buttonSeparator;                           // Margin between Buttons.
    static const int iconButtonsWidth;                          // A generic with for Button with one icon.
    static const int buttonsHeight;                             // A generic height for Buttons.
    static const int labelsHeight;                              // A generic height of Labels.
    static const int comboBoxesHeight;                          // A generic height of juce::ComboBoxes.
    static const int borderThickness;                           // The border thickness around the main window, to allow resizing.
    static const int groupMarginsX;
    static const int groupMarginsY;

    static const int labelFontSize;
    static const int buttonTextSize;
    static const int titleBarTextSize;
    static const int menuBarFontSize;
    static const int popupMenuItemsAndComboBoxFontSize;
    static const int listItemFontSize;
    static const int tooltipFontSize;

    static const int buttonImagesWidth;                         // Width on button with tiny images.

    static const int listItemHeights;                           // Generic item height, in lists.

    static const juce::uint32 defaultPatternColor;
    static const juce::uint32 defaultMarkerColor;

    static const juce::String typefaceNameSquare;               // Use this name to set a font typeface for a Label (this will look up the right font in the custom Look&Feel).
    static const juce::String typefaceMonospace;                // Found in jucer_FontPropertyComponent.

    static const juce::String labelPropertyFontSize;
    static const juce::String labelPropertyUseCustomFont;

    enum class ThemeId : uint16_t
    {
        minimumId = 1,                 // A little simplification, as JUCE often use 0 in juce::ComboBox.

        defaultAt3 = minimumId,
        white,
        blackAndWhite,
        crimson,
        green,
        gray,

        pastLast,
        countNative = pastLast - minimumId,

        startIdCustomTheme = 5000       // Far away to make sure new themes won't collapse will custom ones.
    };

    enum class Colors : int
    {
        firstColor = 0x7f000000,                // Useful for counting.
        // -----------------------------------

        panelBackgroundUnfocused = firstColor,
        panelBackgroundFocused,
        panelBorderUnfocused,
        panelBorderFocused,
        panelTabIcon,
        panelTabBackgroundUnselected,
        panelTabBackgroundSelected,

        interspace,
        panelHandleMouseOver,

        metersBackground,
        metersScaleLines,
        metersLabelBackground,
        metersLabel,
        metersShape,
        patternViewerMetersSoftwareBar,
        patternViewerMetersHardwareBar,
        patternViewerNoiseBar,
        metersMuteFilter,

        dialogBackground,

        dialogBorder,
        buttonBorder,
        button,
        buttonClicked,
        buttonMouseOver,
        titleBarBackground,
        menuBar,
        menuBarText,
        menuBarTextHighlight,
        menuBarTextBackgroundHighlight,
        arpeggioListItemBackgroundEven,
        arpeggioListItemBackgroundOdd,
        arpeggioListItemBackgroundSelected,
        pitchListItemBackgroundEven,
        pitchListItemBackgroundOdd,
        pitchListItemBackgroundSelected,
        instrumentListItemBackgroundEven,
        instrumentListItemBackgroundOdd,
        instrumentListItemBackgroundSelected,
        listItemTextSelected,
        listItemText,

        loopStartEnd,

        linkerPlayCursorTimeline,
        linkerPositionWithinSongRangeBackground,
        linkerPositionOutsideSongRangeBackground,
        linkerPositionBorder,
        linkerPositionCurrentlyPlayedBorder,

        patternViewerBackground,
        patternViewerNoNote,
        patternViewerError,
        patternViewerNoEffect,
        patternViewerArpeggioEffect,
        patternViewerPitchEffect,
        patternViewerVolumeEffect,
        patternViewerResetEffect,
        patternViewerInstrumentSpeedEffect,
        patternViewerCursorTextHighlight,
        patternViewerCursorBackground,
        patternViewerSelection,
        patternViewerCursorLineBackground,
        patternViewerCursorLineHighlightText,
        patternViewerHighlightedLineBackgroundPrimary,
        patternViewerHighlightedLineBackgroundSecondary,
        patternViewerHighlightTextPrimary,
        patternViewerHighlightTextSecondary,
        patternViewerTrackBorder,

        patternViewerTrackHeaderBackgroundOn,
        patternViewerTrackHeaderBackgroundOff,
        patternViewerTrackHeaderBackgroundChannelOn,
        patternViewerTrackHeaderBackgroundChannelOff,

        patternViewerLineTrackHeaderBackground,
        patternViewerLineTrackText,
        patternViewerSpeedTrackHeaderBackground,
        patternViewerSpeedTrackText,
        patternViewerEventTrackHeaderBackground,
        patternViewerEventTrackText,
        patternViewerPlayedLineBackground,

        barSoundTypeBackground,
        barSoundTypeNoSoftNoHard,
        barSoundTypeSoft,
        barSoundTypeHard,
        barSoundTypeSoftToHard,
        barSoundTypeHardToSoft,
        barSoundTypeSoftAndHard,

        barEnvelopeBackground,
        barEnvelopeSoft,
        barEnvelopeHard,

        barNoiseBackground,
        barNoise,

        barPeriodBackground,
        barPeriod,

        barArpeggioNoteBackground,
        barArpeggioNote,
        barArpeggioOctaveBackground,
        barArpeggioOctave,
        barPitchBackground,
        barPitch,

        autoSpreadLoop,


        // -----------------------------------
        pastLastColor,                          // Useful for counting.
        count = pastLastColor - firstColor,
    };
};

}   // namespace arkostracker
