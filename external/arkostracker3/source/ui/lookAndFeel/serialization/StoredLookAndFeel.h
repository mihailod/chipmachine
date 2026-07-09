#pragma once

#include "../../components/StoredProfile.h"

namespace arkostracker 
{

/** All the variables of a LookAndFeel. Mostly immutable, except the non-colors, as a convenience. */
class StoredLookAndFeel final : public StoredProfile
{
public:
    class PanelColors
    {
    public:
        PanelColors(
                unsigned int panelBackgroundFocusedArgb,
                unsigned int panelBackgroundUnfocusedArgb,
                unsigned int panelBorderFocusedArgb,
                unsigned int panelBorderUnfocusedArgb,
                unsigned int panelTabIconArgb,
                unsigned int panelTabBackgroundUnselectedArgb,
                unsigned int panelTabBackgroundSelectedArgb,

                unsigned int panelHandleMouseOverArgb
        ) noexcept;

        const unsigned int panelBackgroundFocusedArgb;
        const unsigned int panelBackgroundUnfocusedArgb;
        const unsigned int panelBorderFocusedArgb;
        const unsigned int panelBorderUnfocusedArgb;
        const unsigned int panelTabIconArgb;
        const unsigned int panelTabBackgroundUnselectedArgb;
        const unsigned int panelTabBackgroundSelectedArgb;

        const unsigned int panelHandleMouseOverArgb;
    };

    class MeterColors
    {
    public:
        MeterColors(
                unsigned int metersBackgroundArgb,
                unsigned int metersScaleLinesArgb,
                unsigned int metersLabelBackgroundArgb,
                unsigned int metersLabelArgb,
                unsigned int metersShapeArgb,
                unsigned int metersMuteFilterArgb
        ) noexcept;

        const unsigned int metersBackgroundArgb;
        const unsigned int metersScaleLinesArgb;
        const unsigned int metersLabelBackgroundArgb;
        const unsigned int metersLabelArgb;
        const unsigned int metersShapeArgb;
        const unsigned int metersMuteFilterArgb;
    };

    class GeneralColors
    {
    public:
        GeneralColors(
                unsigned int textArgb,
                unsigned int dialogBackgroundArgb,
                unsigned int dialogBorderArgb,
                unsigned int interspace,
                unsigned int titleBarBackgroundArgb,
                unsigned int scrollBarArgb,
                unsigned int loopStartEndArgb,
                unsigned int toolTipBackgroundArgb,
                unsigned int toolTipTextArgb
        ) noexcept;

        const unsigned int textArgb;
        const unsigned int dialogBackgroundArgb;
        const unsigned int dialogBorderArgb;
        const unsigned int interspace;
        const unsigned int titleBarBackgroundArgb;
        const unsigned int scrollBarArgb;
        const unsigned int loopStartEndArgb;
        const unsigned int toolTipBackgroundArgb;
        const unsigned int toolTipTextArgb;
    };

    class ButtonColors
    {
    public:
        ButtonColors(
                unsigned int buttonBorderArgb,
                unsigned int buttonArgb,
                unsigned int buttonClickedArgb,
                unsigned int buttonMouseOverArgb,
                unsigned int buttonTextArgb,
                unsigned int buttonTextOffArgb
        ) noexcept;

        const unsigned int buttonBorderArgb;
        const unsigned int buttonArgb;
        const unsigned int buttonTopClickedArgb;
        const unsigned int buttonTopMouseOverArgb;
        const unsigned int buttonTextArgb;
        const unsigned int buttonTextOffArgb;
    };

    class ToolbarColors
    {
    public:
        ToolbarColors(
                unsigned int menuBarArgb,
                unsigned int menuBarTextArgb,
                unsigned int menuBarTextHighlightArgb,
                unsigned int menuBarTextBackgroundHighlightArgb,
                unsigned int popupMenuTextArgb,
                unsigned int popupMenuTextHighlightArgb,
                unsigned int popupMenuBackgroundArgb,
                unsigned int popupMenuBackgroundHighlightArgb
        ) noexcept;

        const unsigned int menuBarArgb;
        const unsigned int menuBarTextArgb;
        const unsigned int menuBarTextHighlightArgb;
        const unsigned int menuBarTextBackgroundHighlightArgb;
        const unsigned int popupMenuTextArgb;
        const unsigned int popupMenuTextHighlightArgb;
        const unsigned int popupMenuBackgroundArgb;
        const unsigned int popupMenuBackgroundHighlightArgb;
    };

    class ListColors
    {
    public:
        ListColors(
                unsigned int arpeggioListItemBackgroundEvenArgb,
                unsigned int arpeggioListItemBackgroundOddArgb,
                unsigned int arpeggioListItemBackgroundSelectedArgb,
                unsigned int pitchListItemBackgroundEvenArgb,
                unsigned int pitchListItemBackgroundOddArgb,
                unsigned int pitchListItemBackgroundSelectedArgb,
                unsigned int instrumentListItemBackgroundEvenArgb,
                unsigned int instrumentListItemBackgroundOddArgb,
                unsigned int instrumentListItemBackgroundSelectedArgb,
                unsigned int listItemTextArgb,
                unsigned int listItemTextSelectedArgb
        ) noexcept;

        const unsigned int arpeggioListItemBackgroundEvenArgb;
        const unsigned int arpeggioListItemBackgroundOddArgb;
        const unsigned int arpeggioListItemBackgroundSelectedArgb;
        const unsigned int pitchListItemBackgroundEvenArgb;
        const unsigned int pitchListItemBackgroundOddArgb;
        const unsigned int pitchListItemBackgroundSelectedArgb;
        const unsigned int instrumentListItemBackgroundEvenArgb;
        const unsigned int instrumentListItemBackgroundOddArgb;
        const unsigned int instrumentListItemBackgroundSelectedArgb;
        const unsigned int listItemTextArgb;
        const unsigned int listItemTextSelectedArgb;
    };

    class LinkerColors
    {
    public:
        LinkerColors(
                unsigned int linkerPlayCursorTimelineArgb,
                unsigned int linkerPositionWithinSongRangeBackgroundArgb,
                unsigned int linkerPositionOutsideSongRangeBackgroundArgb,
                unsigned int linkerPositionBorderArgb,
                unsigned int linkerPositionCurrentlyPlayedBorderArgb
        ) noexcept;

        const unsigned int linkerPlayCursorTimelineArgb;
        const unsigned int linkerPositionWithinSongRangeBackgroundArgb;
        const unsigned int linkerPositionOutsideSongRangeBackgroundArgb;
        const unsigned int linkerPositionBorderArgb;
        const unsigned int linkerPositionCurrentlyPlayedBorderArgb;
    };

    class PatternViewerColors
    {
    public:
        PatternViewerColors(
                unsigned int patternViewerBackgroundArgb,
                unsigned int patternViewerNoNoteArgb,
                unsigned int patternViewerErrorArgb,
                unsigned int patternViewerNoEffectArgb,
                unsigned int patternViewerArpeggioEffectArgb,
                unsigned int patternViewerPitchEffectArgb,
                unsigned int patternViewerVolumeEffectArgb,
                unsigned int patternViewerResetEffectArgb,
                unsigned int patternViewerInstrumentSpeedEffectArgb,
                unsigned int patternViewerCursorTextHighlightArgb,
                unsigned int patternViewerCursorBackgroundArgb,
                unsigned int patternViewerSelectionArgb,
                unsigned int patternViewerCursorLineBackgroundArgb,
                unsigned int patternViewerCursorLineHighlightTextArgb,
                unsigned int patternViewerHighlightedLineBackgroundPrimaryArgb,
                unsigned int patternViewerHighlightedLineBackgroundSecondaryArgb,
                unsigned int patternViewerHighlightTextPrimaryArgb,
                unsigned int patternViewerHighlightTextSecondaryArgb,
                unsigned int patternViewerTrackBorderArgb,

                unsigned int patternViewerTrackHeaderBackgroundOnArgb,
                unsigned int patternViewerTrackHeaderBackgroundOffArgb,
                unsigned int patternViewerTrackHeaderBackgroundChannelOnArgb,
                unsigned int patternViewerTrackHeaderBackgroundChannelOffArgb,
                unsigned int patternViewerLineTrackHeaderBackgroundArgb,
                unsigned int patternViewerLineTrackTextArgb,
                unsigned int patternViewerSpeedTrackHeaderBackgroundArgb,
                unsigned int patternViewerSpeedTrackTextArgb,
                unsigned int patternViewerEventTrackHeaderBackgroundArgb,
                unsigned int patternViewerEventTrackTextArgb,
                unsigned int patternViewerPlayedLineBackgroundArgb,
                unsigned int patternViewerMetersSoftwareBarArgb,
                unsigned int patternViewerMetersHardwareBarArgb,
                unsigned int patternViewerMetersNoiseBarArgb) noexcept;


        const unsigned int patternViewerBackgroundArgb;
        const unsigned int patternViewerNoNoteArgb;
        const unsigned int patternViewerErrorArgb;
        const unsigned int patternViewerNoEffectArgb;
        const unsigned int patternViewerArpeggioEffectArgb;
        const unsigned int patternViewerPitchEffectArgb;
        const unsigned int patternViewerVolumeEffectArgb;
        const unsigned int patternViewerResetEffectArgb;
        const unsigned int patternViewerInstrumentSpeedEffectArgb;
        const unsigned int patternViewerCursorTextHighlightArgb;
        const unsigned int patternViewerCursorBackgroundArgb;
        const unsigned int patternViewerSelectionArgb;
        const unsigned int patternViewerCursorLineBackgroundArgb;
        const unsigned int patternViewerCursorLineHighlightTextArgb;
        const unsigned int patternViewerHighlightedLineBackgroundPrimaryArgb;
        const unsigned int patternViewerHighlightedLineBackgroundSecondaryArgb;
        const unsigned int patternViewerHighlightTextPrimaryArgb;
        const unsigned int patternViewerHighlightTextSecondaryArgb;
        const unsigned int patternViewerTrackBorderArgb;

        const unsigned int patternViewerTrackHeaderBackgroundOnArgb;
        const unsigned int patternViewerTrackHeaderBackgroundOffArgb;
        const unsigned int patternViewerTrackHeaderBackgroundChannelOnArgb;
        const unsigned int patternViewerTrackHeaderBackgroundChannelOffArgb;
        const unsigned int patternViewerLineTrackHeaderBackgroundArgb;
        const unsigned int patternViewerLineTrackTextArgb;
        const unsigned int patternViewerSpeedTrackHeaderBackgroundArgb;
        const unsigned int patternViewerSpeedTrackTextArgb;
        const unsigned int patternViewerEventTrackHeaderBackgroundArgb;
        const unsigned int patternViewerEventTrackTextArgb;
        const unsigned int patternViewerPlayedLineBackgroundArgb;

        const unsigned int patternViewerMetersSoftwareBarArgb;
        const unsigned int patternViewerMetersHardwareBarArgb;
        const unsigned int patternViewerMetersNoiseBarArgb;
    };

    class EditorWithBarsColors
    {
    public:
        EditorWithBarsColors(
                unsigned int barSoundTypeBackgroundArgb,
                unsigned int barSoundTypeNoSoftNoHardArgb,
                unsigned int barSoundTypeSoftArgb,
                unsigned int barSoundTypeHardArgb,
                unsigned int barSoundTypeSoftToHardArgb,
                unsigned int barSoundTypeHardToSoftArgb,
                unsigned int barSoundTypeSoftAndHardArgb,
                unsigned int barEnvelopeBackgroundArgb,
                unsigned int barEnvelopeSoftArgb,
                unsigned int barEnvelopeHardArgb,
                unsigned int barNoiseBackgroundArgb,
                unsigned int barNoiseArgb,
                unsigned int barPeriodBackgroundArgb,
                unsigned int barPeriodArgb,
                unsigned int barArpeggioNoteBackgroundArgb,
                unsigned int barArpeggioNoteArgb,
                unsigned int barArpeggioOctaveBackgroundArgb,
                unsigned int barArpeggioOctaveArgb,
                unsigned int barPitchBackgroundArgb,
                unsigned int barPitchArgb,
                unsigned int autoSpreadLoopArgb
        ) noexcept;

        const unsigned int barSoundTypeBackgroundArgb;
        const unsigned int barSoundTypeNoSoftNoHardArgb;
        const unsigned int barSoundTypeSoftArgb;
        const unsigned int barSoundTypeHardArgb;
        const unsigned int barSoundTypeSoftToHardArgb;
        const unsigned int barSoundTypeHardToSoftArgb;
        const unsigned int barSoundTypeSoftAndHardArgb;
        const unsigned int barEnvelopeBackgroundArgb;
        const unsigned int barEnvelopeSoftArgb;
        const unsigned int barEnvelopeHardArgb;
        const unsigned int barNoiseBackgroundArgb;
        const unsigned int barNoiseArgb;
        const unsigned int barPeriodBackgroundArgb;
        const unsigned int barPeriodArgb;
        const unsigned int barArpeggioNoteBackgroundArgb;
        const unsigned int barArpeggioNoteArgb;
        const unsigned int barArpeggioOctaveBackgroundArgb;
        const unsigned int barArpeggioOctaveArgb;
        const unsigned int barPitchBackgroundArgb;
        const unsigned int barPitchArgb;
        const unsigned int autoSpreadLoopArgb;
    };


    // =================================================================

    StoredLookAndFeel(int id, juce::String displayedName, bool readOnly,
                      const GeneralColors& generalColors,
                      const PanelColors& panelColors,
                      const ButtonColors& buttonColors,
                      const ToolbarColors& toolbarColors,
                      const ListColors& listColors,
                      const MeterColors& meterColors,
                      const LinkerColors& linkerColors,
                      const EditorWithBarsColors& editorWithBarsColors,
                      const PatternViewerColors& patternViewerColors
    );

    /** The ID. As a convenience, >0 to be put in juce::ComboBox (JUCE reserves 0). Custom ones are using high numbers. */
    int id;
    /** A name, as shown in a drop-down for example. */
    juce::String displayedName;
    /** User-created themes are not read only. */
    bool readOnly;

    GeneralColors generalColors;
    PanelColors panelColors;
    ButtonColors buttonColors;
    ToolbarColors toolbarColors;
    ListColors listColors;
    MeterColors meterColors;
    LinkerColors linkerColors;
    EditorWithBarsColors editorWithBarsColors;
    PatternViewerColors patternViewerColors;


    // StoredProfile method implementations.
    // ===========================================
    int getId() const override;
    void setId(int id) override;
    bool isReadOnly() const override;
    juce::String getDisplayedName() const override;
    void setDisplayedName(const juce::String& name) override;
    void setReadOnly(bool readOnly) override;
};

}   // namespace arkostracker
