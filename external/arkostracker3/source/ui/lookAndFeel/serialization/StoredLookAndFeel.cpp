#include "StoredLookAndFeel.h"

#include "../LookAndFeelConstants.h"

namespace arkostracker 
{

StoredLookAndFeel::PanelColors::PanelColors(
    const unsigned int pPanelBackgroundFocusedArgb,
    const unsigned int pPanelBackgroundUnfocusedArgb,
    const unsigned int pPanelBorderFocusedArgb,
    const unsigned int pPanelBorderUnfocusedArgb,
    const unsigned int pPanelTabIconArgb,
    const unsigned int pPanelTabBackgroundUnselectedArgb,
    const unsigned int pPanelTabBackgroundSelectedArgb,
    const unsigned int pPanelHandleMouseOverArgb) noexcept :

        panelBackgroundFocusedArgb(pPanelBackgroundFocusedArgb),
        panelBackgroundUnfocusedArgb(pPanelBackgroundUnfocusedArgb),
        panelBorderFocusedArgb(pPanelBorderFocusedArgb),
        panelBorderUnfocusedArgb(pPanelBorderUnfocusedArgb),
        panelTabIconArgb(pPanelTabIconArgb),
        panelTabBackgroundUnselectedArgb(pPanelTabBackgroundUnselectedArgb),
        panelTabBackgroundSelectedArgb(pPanelTabBackgroundSelectedArgb),
        panelHandleMouseOverArgb(pPanelHandleMouseOverArgb)
{
}

StoredLookAndFeel::MeterColors::MeterColors(const unsigned int pMetersBackgroundArgb,
                                            const unsigned int pMetersScaleLinesArgb,
                                            const unsigned int pMetersLabelBackgroundArgb,
                                            const unsigned int pMetersLabelArgb,
                                            const unsigned int pMetersShapeArgb,
                                            const unsigned int pMetersMuteFilterArgb) noexcept :
        metersBackgroundArgb(pMetersBackgroundArgb),
        metersScaleLinesArgb(pMetersScaleLinesArgb),
        metersLabelBackgroundArgb(pMetersLabelBackgroundArgb),
        metersLabelArgb(pMetersLabelArgb),
        metersShapeArgb(pMetersShapeArgb),
        metersMuteFilterArgb(pMetersMuteFilterArgb)
{
}

StoredLookAndFeel::GeneralColors::GeneralColors(const unsigned int pTextArgb,
                                                const unsigned int pDialogBackgroundArgb,
                                                const unsigned int pDialogBorderArgb,
                                                const unsigned int pInterspace,
                                                const unsigned int pTitleBarBackgroundArgb,
                                                const unsigned int pScrollBarArgb,
                                                const unsigned int pLoopStartEndArgb,
                                                const unsigned int pToolTipBackgroundArgb,
                                                const unsigned int pToolTipTextArgb) noexcept :
        textArgb(pTextArgb),
        dialogBackgroundArgb(pDialogBackgroundArgb),
        dialogBorderArgb(pDialogBorderArgb),
        interspace(pInterspace),
        titleBarBackgroundArgb(pTitleBarBackgroundArgb),
        scrollBarArgb(pScrollBarArgb),
        loopStartEndArgb(pLoopStartEndArgb),
        toolTipBackgroundArgb(pToolTipBackgroundArgb),
        toolTipTextArgb(pToolTipTextArgb)
{
}

StoredLookAndFeel::ButtonColors::ButtonColors(const unsigned int pButtonBorderArgb,
                                              const unsigned int pButtonArgb,
                                              const unsigned int pButtonClickedArgb,
                                              const unsigned int pButtonMouseOverArgb,
                                              const unsigned int pButtonTextArgb,
                                              const unsigned int pButtonTextOffArgb) noexcept :
        buttonBorderArgb(pButtonBorderArgb),
        buttonArgb(pButtonArgb),
        buttonTopClickedArgb(pButtonClickedArgb),
        buttonTopMouseOverArgb(pButtonMouseOverArgb),
        buttonTextArgb(pButtonTextArgb),
        buttonTextOffArgb(pButtonTextOffArgb)
{
}

StoredLookAndFeel::ToolbarColors::ToolbarColors(const unsigned int pMenuBarArgb,
                                                const unsigned int pMenuBarTextArgb,
                                                const unsigned int pMenuBarTextHighlightArgb,
                                                const unsigned int pMenuBarTextBackgroundHighlightArgb,
                                                const unsigned int pPopupMenuTextArgb,
                                                const unsigned int pPopupMenuTextHighlightArgb,
                                                const unsigned int pPopupMenuBackgroundArgb,
                                                const unsigned int pPopupMenuBackgroundHighlightArgb) noexcept :
        menuBarArgb(pMenuBarArgb),
        menuBarTextArgb(pMenuBarTextArgb),
        menuBarTextHighlightArgb(pMenuBarTextHighlightArgb),
        menuBarTextBackgroundHighlightArgb(pMenuBarTextBackgroundHighlightArgb),
        popupMenuTextArgb(pPopupMenuTextArgb),
        popupMenuTextHighlightArgb(pPopupMenuTextHighlightArgb),
        popupMenuBackgroundArgb(pPopupMenuBackgroundArgb),
        popupMenuBackgroundHighlightArgb(pPopupMenuBackgroundHighlightArgb)
{
}

StoredLookAndFeel::ListColors::ListColors(const unsigned int pArpeggioListItemBackgroundEvenArgb,
                                          const unsigned int pArpeggioListItemBackgroundOddArgb,
                                          const unsigned int pArpeggioListItemBackgroundSelectedArgb,
                                          const unsigned int pPitchListItemBackgroundEvenArgb,
                                          const unsigned int pPitchListItemBackgroundOddArgb,
                                          const unsigned int pPitchListItemBackgroundSelectedArgb,
                                          const unsigned int pInstrumentListItemBackgroundEvenArgb,
                                          const unsigned int pInstrumentListItemBackgroundOddArgb,
                                          const unsigned int pInstrumentListItemBackgroundSelectedArgb,
                                          const unsigned int pListItemTextArgb,
                                          const unsigned int pListItemTextSelectedArgb) noexcept :
        arpeggioListItemBackgroundEvenArgb(pArpeggioListItemBackgroundEvenArgb),
        arpeggioListItemBackgroundOddArgb(pArpeggioListItemBackgroundOddArgb),
        arpeggioListItemBackgroundSelectedArgb(pArpeggioListItemBackgroundSelectedArgb),
        pitchListItemBackgroundEvenArgb(pPitchListItemBackgroundEvenArgb),
        pitchListItemBackgroundOddArgb(pPitchListItemBackgroundOddArgb),
        pitchListItemBackgroundSelectedArgb(pPitchListItemBackgroundSelectedArgb),
        instrumentListItemBackgroundEvenArgb(pInstrumentListItemBackgroundEvenArgb),
        instrumentListItemBackgroundOddArgb(pInstrumentListItemBackgroundOddArgb),
        instrumentListItemBackgroundSelectedArgb(pInstrumentListItemBackgroundSelectedArgb),
        listItemTextArgb(pListItemTextArgb),
        listItemTextSelectedArgb(pListItemTextSelectedArgb)
{
}

StoredLookAndFeel::LinkerColors::LinkerColors(
    const unsigned int pLinkerPlayCursorTimelineArgb,
    const unsigned int pLinkerPositionWithinSongRangeBackgroundArgb,
    const unsigned int pLinkerPositionOutsideSongRangeBackgroundArgb,
    const unsigned int pLinkerPositionBorderArgb,
    const unsigned int pLinkerPositionCurrentlyPlayedBorderArgb) noexcept :

        linkerPlayCursorTimelineArgb(pLinkerPlayCursorTimelineArgb),
        linkerPositionWithinSongRangeBackgroundArgb(pLinkerPositionWithinSongRangeBackgroundArgb),
        linkerPositionOutsideSongRangeBackgroundArgb(pLinkerPositionOutsideSongRangeBackgroundArgb),
        linkerPositionBorderArgb(pLinkerPositionBorderArgb),
        linkerPositionCurrentlyPlayedBorderArgb(pLinkerPositionCurrentlyPlayedBorderArgb)
{
}

StoredLookAndFeel::PatternViewerColors::PatternViewerColors(
        unsigned int pPatternViewerBackgroundArgb,
        unsigned int pPatternViewerNoNoteArgb,
        unsigned int pPatternViewerErrorArgb,
        unsigned int pPatternViewerNoEffectArgb,
        unsigned int pPatternViewerArpeggioEffectArgb,
        unsigned int pPatternViewerPitchEffectArgb,
        unsigned int pPatternViewerVolumeEffectArgb,
        unsigned int pPatternViewerResetEffectArgb,
        unsigned int pPatternViewerInstrumentSpeedEffectArgb,
        unsigned int pPatternViewerCursorTextHighlightArgb,
        unsigned int pPatternViewerCursorBackgroundArgb,
        unsigned int pPatternViewerSelectionArgb,
        unsigned int pPatternViewerCursorLineBackgroundArgb,
        unsigned int pPatternViewerCursorLineHighlightTextArgb,
        unsigned int pPatternViewerHighlightedLineBackgroundPrimaryArgb,
        unsigned int pPatternViewerHighlightedLineBackgroundSecondaryArgb,
        unsigned int pPatternViewerHighlightTextPrimaryArgb,
        unsigned int pPatternViewerHighlightTextSecondaryArgb,
        unsigned int pPatternViewerTrackBorderArgb,

        unsigned int pPatternViewerTrackHeaderBackgroundOnArgb,
        unsigned int pPatternViewerTrackHeaderBackgroundOffArgb,
        unsigned int pPatternViewerTrackHeaderBackgroundChannelOnArgb,
        unsigned int pPatternViewerTrackHeaderBackgroundChannelOffArgb,
        unsigned int pPatternViewerLineTrackHeaderBackgroundArgb,
        unsigned int pPatternViewerLineTrackTextArgb,
        unsigned int pPatternViewerSpeedTrackHeaderBackgroundArgb,
        unsigned int pPatternViewerSpeedTrackTextArgb,
        unsigned int pPatternViewerEventTrackHeaderBackgroundArgb,
        unsigned int pPatternViewerEventTrackTextArgb,
        unsigned int pPatternViewerPlayedLineBackgroundArgb,

        unsigned int pPatternViewerMetersSoftwareBarArgb,
        unsigned int pPatternViewerMetersHardwareBarArgb,
        unsigned int pPatternViewerMetersNoiseBarArgb) noexcept :

        patternViewerBackgroundArgb(pPatternViewerBackgroundArgb),
        patternViewerNoNoteArgb(pPatternViewerNoNoteArgb),
        patternViewerErrorArgb(pPatternViewerErrorArgb),
        patternViewerNoEffectArgb(pPatternViewerNoEffectArgb),
        patternViewerArpeggioEffectArgb(pPatternViewerArpeggioEffectArgb),
        patternViewerPitchEffectArgb(pPatternViewerPitchEffectArgb),
        patternViewerVolumeEffectArgb(pPatternViewerVolumeEffectArgb),
        patternViewerResetEffectArgb(pPatternViewerResetEffectArgb),
        patternViewerInstrumentSpeedEffectArgb(pPatternViewerInstrumentSpeedEffectArgb),
        patternViewerCursorTextHighlightArgb(pPatternViewerCursorTextHighlightArgb),
        patternViewerCursorBackgroundArgb(pPatternViewerCursorBackgroundArgb),
        patternViewerSelectionArgb(pPatternViewerSelectionArgb),
        patternViewerCursorLineBackgroundArgb(pPatternViewerCursorLineBackgroundArgb),
        patternViewerCursorLineHighlightTextArgb(pPatternViewerCursorLineHighlightTextArgb),
        patternViewerHighlightedLineBackgroundPrimaryArgb(pPatternViewerHighlightedLineBackgroundPrimaryArgb),
        patternViewerHighlightedLineBackgroundSecondaryArgb(pPatternViewerHighlightedLineBackgroundSecondaryArgb),
        patternViewerHighlightTextPrimaryArgb(pPatternViewerHighlightTextPrimaryArgb),
        patternViewerHighlightTextSecondaryArgb(pPatternViewerHighlightTextSecondaryArgb),
        patternViewerTrackBorderArgb(pPatternViewerTrackBorderArgb),

        patternViewerTrackHeaderBackgroundOnArgb(pPatternViewerTrackHeaderBackgroundOnArgb),
        patternViewerTrackHeaderBackgroundOffArgb(pPatternViewerTrackHeaderBackgroundOffArgb),
        patternViewerTrackHeaderBackgroundChannelOnArgb(pPatternViewerTrackHeaderBackgroundChannelOnArgb),
        patternViewerTrackHeaderBackgroundChannelOffArgb(pPatternViewerTrackHeaderBackgroundChannelOffArgb),
        patternViewerLineTrackHeaderBackgroundArgb(pPatternViewerLineTrackHeaderBackgroundArgb),
        patternViewerLineTrackTextArgb(pPatternViewerLineTrackTextArgb),
        patternViewerSpeedTrackHeaderBackgroundArgb(pPatternViewerSpeedTrackHeaderBackgroundArgb),
        patternViewerSpeedTrackTextArgb(pPatternViewerSpeedTrackTextArgb),
        patternViewerEventTrackHeaderBackgroundArgb(pPatternViewerEventTrackHeaderBackgroundArgb),
        patternViewerEventTrackTextArgb(pPatternViewerEventTrackTextArgb),
        patternViewerPlayedLineBackgroundArgb(pPatternViewerPlayedLineBackgroundArgb),

        patternViewerMetersSoftwareBarArgb(pPatternViewerMetersSoftwareBarArgb),
        patternViewerMetersHardwareBarArgb(pPatternViewerMetersHardwareBarArgb),
        patternViewerMetersNoiseBarArgb(pPatternViewerMetersNoiseBarArgb)
{
}

StoredLookAndFeel::EditorWithBarsColors::EditorWithBarsColors(
    const unsigned int pBarSoundTypeBackgroundArgb,
    const unsigned int pBarSoundTypeNoSoftNoHardArgb,
    const unsigned int pBarSoundTypeSoftArgb,
    const unsigned int pBarSoundTypeHardArgb,
    const unsigned int pBarSoundTypeSoftToHardArgb,
    const unsigned int pBarSoundTypeHardToSoftArgb,
    const unsigned int pBarSoundTypeSoftAndHardArgb,
    const unsigned int pBarEnvelopeBackgroundArgb,
    const unsigned int pBarEnvelopeSoftArgb,
    const unsigned int pBarEnvelopeHardArgb,
    const unsigned int pBarNoiseBackgroundArgb,
    const unsigned int pBarNoiseArgb,
    const unsigned int pBarPeriodBackgroundArgb,
    const unsigned int pBarPeriodArgb,
    const unsigned int pBarArpeggioNoteBackgroundArgb,
    const unsigned int pBarArpeggioNoteArgb,
    const unsigned int pBarArpeggioOctaveBackgroundArgb,
    const unsigned int pBarArpeggioOctaveArgb,
    const unsigned int pBarPitchBackgroundArgb,
    const unsigned int pBarPitchArgb,
    const unsigned int pAutoSpreadLoopArgb
        ) noexcept :

        barSoundTypeBackgroundArgb(pBarSoundTypeBackgroundArgb),
        barSoundTypeNoSoftNoHardArgb(pBarSoundTypeNoSoftNoHardArgb),
        barSoundTypeSoftArgb(pBarSoundTypeSoftArgb),
        barSoundTypeHardArgb(pBarSoundTypeHardArgb),
        barSoundTypeSoftToHardArgb(pBarSoundTypeSoftToHardArgb),
        barSoundTypeHardToSoftArgb(pBarSoundTypeHardToSoftArgb),
        barSoundTypeSoftAndHardArgb(pBarSoundTypeSoftAndHardArgb),
        barEnvelopeBackgroundArgb(pBarEnvelopeBackgroundArgb),
        barEnvelopeSoftArgb(pBarEnvelopeSoftArgb),
        barEnvelopeHardArgb(pBarEnvelopeHardArgb),
        barNoiseBackgroundArgb(pBarNoiseBackgroundArgb),
        barNoiseArgb(pBarNoiseArgb),
        barPeriodBackgroundArgb(pBarPeriodBackgroundArgb),
        barPeriodArgb(pBarPeriodArgb),
        barArpeggioNoteBackgroundArgb(pBarArpeggioNoteBackgroundArgb),
        barArpeggioNoteArgb(pBarArpeggioNoteArgb),
        barArpeggioOctaveBackgroundArgb(pBarArpeggioOctaveBackgroundArgb),
        barArpeggioOctaveArgb(pBarArpeggioOctaveArgb),
        barPitchBackgroundArgb(pBarPitchBackgroundArgb),
        barPitchArgb(pBarPitchArgb),
        autoSpreadLoopArgb(pAutoSpreadLoopArgb)
{
}

StoredLookAndFeel::StoredLookAndFeel(const int pId, juce::String pDisplayedName, const bool pReadOnly,
                                     const GeneralColors& pGeneralColors,
                                     const PanelColors& pPanelColors,
                                     const ButtonColors& pButtonColors,
                                     const ToolbarColors& pToolbarColors,
                                     const ListColors& pListColors,
                                     const MeterColors& pMeterColors,
                                     const LinkerColors& pLinkerColors,
                                     const EditorWithBarsColors& pEditorWithBarsColors,
                                     const PatternViewerColors& pPatternViewerColors
) :
        id(pId),
        displayedName(std::move(pDisplayedName)),
        readOnly(pReadOnly),

        generalColors(pGeneralColors),
        panelColors(pPanelColors),
        buttonColors(pButtonColors),
        toolbarColors(pToolbarColors),
        listColors(pListColors),
        meterColors(pMeterColors),
        linkerColors(pLinkerColors),
        editorWithBarsColors(pEditorWithBarsColors),
        patternViewerColors(pPatternViewerColors)
{
    if (!readOnly) {
        jassert(id >= static_cast<int>(LookAndFeelConstants::ThemeId::startIdCustomTheme));
    } else {
        jassert(id >= static_cast<int>(LookAndFeelConstants::ThemeId::minimumId));
        jassert(id < static_cast<int>(LookAndFeelConstants::ThemeId::startIdCustomTheme));
    }
}

int StoredLookAndFeel::getId() const
{
    return id;
}

void StoredLookAndFeel::setId(const int newId)
{
    id = newId;
}

bool StoredLookAndFeel::isReadOnly() const
{
    return readOnly;
}

void StoredLookAndFeel::setReadOnly(const bool isReadOnly)
{
    readOnly = isReadOnly;
}

juce::String StoredLookAndFeel::getDisplayedName() const
{
    return displayedName;
}

void StoredLookAndFeel::setDisplayedName(const juce::String& name)
{
    displayedName = name;
}

}   // namespace arkostracker
