#include "XmlSerializer.h"

#include "../../../utils/XmlHelper.h"
#include "../LookAndFeelConstants.h"
#include "StoredLookAndFeel.h"

namespace arkostracker 
{

std::unique_ptr<juce::XmlElement> XmlSerializer::serialize(const StoredLookAndFeel& storedLookAndFeel) const noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodeLookAndFeel);

    XmlHelper::addValueNode(*rootXml, nodeId, storedLookAndFeel.id);
    XmlHelper::addValueNode(*rootXml, nodeDisplayedName, storedLookAndFeel.displayedName);
    XmlHelper::addValueNode(*rootXml, nodeReadOnly, storedLookAndFeel.readOnly);

    // First, all the JUCE colors that are overridden.
    const auto& generalColors = storedLookAndFeel.generalColors;
    XmlHelper::addValueNode(*rootXml, nodeTextArgb, generalColors.textArgb);
    XmlHelper::addValueNode(*rootXml, nodeDialogBackgroundArgb, generalColors.dialogBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeDialogBorderArgb, generalColors.dialogBorderArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelHandleIdleArgb, generalColors.interspace);
    XmlHelper::addValueNode(*rootXml, nodeTitleBarBackgroundArgb, generalColors.titleBarBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeScrollBarArgb, generalColors.scrollBarArgb);
    XmlHelper::addValueNode(*rootXml, nodeLinkerLoopStartEndArgb, generalColors.loopStartEndArgb);
    XmlHelper::addValueNode(*rootXml, nodeToolTipBackgroundArgb, generalColors.toolTipBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeToolTipTextArgb, generalColors.toolTipTextArgb);

    const auto& buttonColors = storedLookAndFeel.buttonColors;
    XmlHelper::addValueNode(*rootXml, nodeButtonTextArgb, buttonColors.buttonTextArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonTextOffArgb, buttonColors.buttonTextOffArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonTextOffArgb, buttonColors.buttonTextOffArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonBorderArgb, buttonColors.buttonBorderArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonArgb, buttonColors.buttonArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonClickedArgb, buttonColors.buttonTopClickedArgb);
    XmlHelper::addValueNode(*rootXml, nodeButtonMouseOverArgb, buttonColors.buttonTopMouseOverArgb);

    // Then, our custom colors.
    // Makes sure we have the right amount of nodes. Only our custom nodes can be tested though.
    static_assert(static_cast<int>(LookAndFeelConstants::Colors::count) == 97, "Forgot to add the nodes below!"); // NOLINT(*-magic-numbers)

    const auto& panelColors = storedLookAndFeel.panelColors;
    XmlHelper::addValueNode(*rootXml, nodePanelBackgroundFocusedArgb, panelColors.panelBackgroundFocusedArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelBackgroundUnfocusedArgb, panelColors.panelBackgroundUnfocusedArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelBorderFocusedArgb, panelColors.panelBorderFocusedArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelBorderUnfocusedArgb, panelColors.panelBorderUnfocusedArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelTabIconArgb, panelColors.panelTabIconArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelTabBackgroundUnselectedArgb, panelColors.panelTabBackgroundUnselectedArgb);
    XmlHelper::addValueNode(*rootXml, nodePanelTabBackgroundSelectedArgb, panelColors.panelTabBackgroundSelectedArgb);

    XmlHelper::addValueNode(*rootXml, nodePanelHandleMouseOverArgb, panelColors.panelHandleMouseOverArgb);

    const auto& meterColors = storedLookAndFeel.meterColors;
    XmlHelper::addValueNode(*rootXml, nodeMetersBackgroundArgb, meterColors.metersBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersScaleLinesArgb, meterColors.metersScaleLinesArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersLabelBackgroundArgb, meterColors.metersLabelBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersLabelArgb, meterColors.metersLabelArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersShapeArgb, meterColors.metersShapeArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersMuteFilterArgb, meterColors.metersMuteFilterArgb);

    const auto& toolbarColors = storedLookAndFeel.toolbarColors;
    XmlHelper::addValueNode(*rootXml, nodeMenuBarArgb, toolbarColors.menuBarArgb);
    XmlHelper::addValueNode(*rootXml, nodeMenuBarTextArgb, toolbarColors.menuBarTextArgb);
    XmlHelper::addValueNode(*rootXml, nodeMenuBarTextHighlightArgb, toolbarColors.menuBarTextHighlightArgb);
    XmlHelper::addValueNode(*rootXml, nodeMenuBarTextBackgroundHighlightArgb, toolbarColors.menuBarTextBackgroundHighlightArgb);
    XmlHelper::addValueNode(*rootXml, nodePopupMenuTextArgb, toolbarColors.popupMenuTextArgb);
    XmlHelper::addValueNode(*rootXml, nodePopupMenuTextHighlightArgb, toolbarColors.popupMenuTextHighlightArgb);
    XmlHelper::addValueNode(*rootXml, nodePopupMenuBackgroundArgb, toolbarColors.popupMenuBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePopupMenuBackgroundHighlightArgb, toolbarColors.popupMenuBackgroundHighlightArgb);

    const auto& listColors = storedLookAndFeel.listColors;
    XmlHelper::addValueNode(*rootXml, nodeArpeggioListItemBackgroundEvenArgb, listColors.arpeggioListItemBackgroundEvenArgb);
    XmlHelper::addValueNode(*rootXml, nodeArpeggioListItemBackgroundOddArgb, listColors.arpeggioListItemBackgroundOddArgb);
    XmlHelper::addValueNode(*rootXml, nodeArpeggioListItemBackgroundSelectedArgb, listColors.arpeggioListItemBackgroundSelectedArgb);
    XmlHelper::addValueNode(*rootXml, nodePitchListItemBackgroundEvenArgb, listColors.pitchListItemBackgroundEvenArgb);
    XmlHelper::addValueNode(*rootXml, nodePitchListItemBackgroundOddArgb, listColors.pitchListItemBackgroundOddArgb);
    XmlHelper::addValueNode(*rootXml, nodePitchListItemBackgroundSelectedArgb, listColors.pitchListItemBackgroundSelectedArgb);
    XmlHelper::addValueNode(*rootXml, nodeInstrumentListItemBackgroundEvenArgb, listColors.instrumentListItemBackgroundEvenArgb);
    XmlHelper::addValueNode(*rootXml, nodeInstrumentListItemBackgroundOddArgb, listColors.instrumentListItemBackgroundOddArgb);
    XmlHelper::addValueNode(*rootXml, nodeInstrumentListItemBackgroundSelectedArgb, listColors.instrumentListItemBackgroundSelectedArgb);
    XmlHelper::addValueNode(*rootXml, nodeListItemTextArgb, listColors.listItemTextArgb);
    XmlHelper::addValueNode(*rootXml, nodeListItemTextSelectedArgb, listColors.listItemTextSelectedArgb);

    const auto& linkerColors = storedLookAndFeel.linkerColors;
    XmlHelper::addValueNode(*rootXml, nodeLinkerPlayCursorTimelineArgb, linkerColors.linkerPlayCursorTimelineArgb);
    XmlHelper::addValueNode(*rootXml, nodeLinkerPositionWithinSongRangeBackgroundArgb, linkerColors.linkerPositionWithinSongRangeBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeLinkerPositionOutsideSongRangeBackgroundArgb, linkerColors.linkerPositionOutsideSongRangeBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeLinkerPositionBorderArgb, linkerColors.linkerPositionBorderArgb);
    XmlHelper::addValueNode(*rootXml, nodeLinkerPositionCurrentlyPlayedBorderArgb, linkerColors.linkerPositionCurrentlyPlayedBorderArgb);

    const auto& patternViewerColors = storedLookAndFeel.patternViewerColors;
    XmlHelper::addValueNode(*rootXml, nodePatternViewerBackgroundArgb, patternViewerColors.patternViewerBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerNoNoteArgb, patternViewerColors.patternViewerNoNoteArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerErrorArgb, patternViewerColors.patternViewerErrorArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerNoEffectArgb, patternViewerColors.patternViewerNoEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerArpeggioEffectArgb, patternViewerColors.patternViewerArpeggioEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerPitchEffectArgb, patternViewerColors.patternViewerPitchEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerVolumeEffectArgb, patternViewerColors.patternViewerVolumeEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerResetEffectArgb, patternViewerColors.patternViewerResetEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerInstrumentSpeedEffectArgb, patternViewerColors.patternViewerInstrumentSpeedEffectArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerCursorTextHighlightArgb, patternViewerColors.patternViewerCursorTextHighlightArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerCursorBackgroundArgb, patternViewerColors.patternViewerCursorBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerSelectionArgb, patternViewerColors.patternViewerSelectionArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerCursorLineBackgroundArgb, patternViewerColors.patternViewerCursorLineBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerCursorLineHighlightTextArgb, patternViewerColors.patternViewerCursorLineHighlightTextArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerHighlightedLineBackgroundPrimaryArgb, patternViewerColors.patternViewerHighlightedLineBackgroundPrimaryArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerHighlightedLineBackgroundSecondaryArgb, patternViewerColors.patternViewerHighlightedLineBackgroundSecondaryArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerHighlightTextPrimaryArgb, patternViewerColors.patternViewerHighlightTextPrimaryArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerHighlightTextSecondaryArgb, patternViewerColors.patternViewerHighlightTextSecondaryArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerTrackBorderArgb, patternViewerColors.patternViewerTrackBorderArgb);

    XmlHelper::addValueNode(*rootXml, nodePatternViewerTrackHeaderBackgroundOnArgb, patternViewerColors.patternViewerTrackHeaderBackgroundOnArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerTrackHeaderBackgroundOffArgb, patternViewerColors.patternViewerTrackHeaderBackgroundOffArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerTrackHeaderBackgroundChannelOnArgb, patternViewerColors.patternViewerTrackHeaderBackgroundChannelOnArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerTrackHeaderBackgroundChannelOffArgb, patternViewerColors.patternViewerTrackHeaderBackgroundChannelOffArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerLineTrackHeaderBackgroundArgb, patternViewerColors.patternViewerLineTrackHeaderBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerLineTrackTextArgb, patternViewerColors.patternViewerLineTrackTextArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerSpeedTrackHeaderBackgroundArgb, patternViewerColors.patternViewerSpeedTrackHeaderBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerSpeedTrackTextArgb, patternViewerColors.patternViewerSpeedTrackTextArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerEventTrackHeaderBackgroundArgb, patternViewerColors.patternViewerEventTrackHeaderBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerEventTrackTextArgb, patternViewerColors.patternViewerEventTrackTextArgb);
    XmlHelper::addValueNode(*rootXml, nodePatternViewerPlayedLineBackgroundArgb, patternViewerColors.patternViewerPlayedLineBackgroundArgb);

    XmlHelper::addValueNode(*rootXml, nodeMetersVolumeArgb, patternViewerColors.patternViewerMetersSoftwareBarArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersVolumeIfHardwareArgb, patternViewerColors.patternViewerMetersHardwareBarArgb);
    XmlHelper::addValueNode(*rootXml, nodeMetersNoiseArgb, patternViewerColors.patternViewerMetersNoiseBarArgb);

    const auto& editorWithBarsColors = storedLookAndFeel.editorWithBarsColors;
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeBackground, editorWithBarsColors.barSoundTypeBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeNoSoftNoHard, editorWithBarsColors.barSoundTypeNoSoftNoHardArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeSoft, editorWithBarsColors.barSoundTypeSoftArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeHard, editorWithBarsColors.barSoundTypeHardArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeSoftToHard, editorWithBarsColors.barSoundTypeSoftToHardArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeHardToSoft, editorWithBarsColors.barSoundTypeHardToSoftArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarSoundTypeSoftAndHard, editorWithBarsColors.barSoundTypeSoftAndHardArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarEnvelopeBackground, editorWithBarsColors.barEnvelopeBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarEnvelopeSoft, editorWithBarsColors.barEnvelopeSoftArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarEnvelopeHard, editorWithBarsColors.barEnvelopeHardArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarNoiseBackground, editorWithBarsColors.barNoiseBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarNoise, editorWithBarsColors.barNoiseArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarPeriodBackground, editorWithBarsColors.barPeriodBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarPeriod, editorWithBarsColors.barPeriodArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarArpeggioNoteBackground, editorWithBarsColors.barArpeggioNoteBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarArpeggioNote, editorWithBarsColors.barArpeggioNoteArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarArpeggioOctaveBackground, editorWithBarsColors.barArpeggioOctaveBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarArpeggioOctave, editorWithBarsColors.barArpeggioOctaveArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarPitchBackground, editorWithBarsColors.barPitchBackgroundArgb);
    XmlHelper::addValueNode(*rootXml, nodeBarPitch, editorWithBarsColors.barPitchArgb);
    XmlHelper::addValueNode(*rootXml, nodeAutoSpreadLoop, editorWithBarsColors.autoSpreadLoopArgb);

    return rootXml;
}

std::unique_ptr<StoredLookAndFeel> XmlSerializer::deserialize(const juce::XmlElement& xmlElement, const StoredLookAndFeel& defaultStoredLookAndFeel) const noexcept // NOLINT(readability-convert-member-functions-to-static)
{
    const auto& storedGeneralColors = defaultStoredLookAndFeel.generalColors; 
    const StoredLookAndFeel::GeneralColors generalColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeTextArgb, storedGeneralColors.textArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeDialogBackgroundArgb, storedGeneralColors.dialogBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeDialogBorderArgb, storedGeneralColors.dialogBorderArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelHandleIdleArgb, storedGeneralColors.interspace),
            XmlHelper::readUnsignedInt(xmlElement, nodeTitleBarBackgroundArgb, storedGeneralColors.titleBarBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeScrollBarArgb, storedGeneralColors.scrollBarArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerLoopStartEndArgb, storedGeneralColors.loopStartEndArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeToolTipBackgroundArgb, storedGeneralColors.toolTipBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeToolTipTextArgb, storedGeneralColors.toolTipTextArgb)
    );

    const auto& storedPanelColors = defaultStoredLookAndFeel.panelColors;
    const StoredLookAndFeel::PanelColors panelColors(
            XmlHelper::readUnsignedInt(xmlElement, nodePanelBackgroundFocusedArgb, storedPanelColors.panelBackgroundFocusedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelBackgroundUnfocusedArgb, storedPanelColors.panelBackgroundUnfocusedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelBorderFocusedArgb, storedPanelColors.panelBorderFocusedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelBorderUnfocusedArgb, storedPanelColors.panelBorderUnfocusedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelTabIconArgb, storedPanelColors.panelTabIconArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelTabBackgroundUnselectedArgb, storedPanelColors.panelTabBackgroundUnselectedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePanelTabBackgroundSelectedArgb, storedPanelColors.panelTabBackgroundSelectedArgb),

            XmlHelper::readUnsignedInt(xmlElement, nodePanelHandleMouseOverArgb, storedPanelColors.panelHandleMouseOverArgb)
    );

    const auto& storedButtonColors = defaultStoredLookAndFeel.buttonColors;
    const StoredLookAndFeel::ButtonColors buttonColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonBorderArgb, storedButtonColors.buttonBorderArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonArgb, storedButtonColors.buttonArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonClickedArgb, storedButtonColors.buttonTopClickedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonMouseOverArgb, storedButtonColors.buttonTopMouseOverArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonTextArgb, storedButtonColors.buttonTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeButtonTextOffArgb, storedButtonColors.buttonTextOffArgb)
    );

    const auto& storedToolbarColors = defaultStoredLookAndFeel.toolbarColors;
    const StoredLookAndFeel::ToolbarColors toolbarColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeMenuBarArgb, storedToolbarColors.menuBarArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMenuBarTextArgb, storedToolbarColors.menuBarTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMenuBarTextHighlightArgb, storedToolbarColors.menuBarTextHighlightArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMenuBarTextBackgroundHighlightArgb, storedToolbarColors.menuBarTextBackgroundHighlightArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePopupMenuTextArgb, storedToolbarColors.popupMenuTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePopupMenuTextHighlightArgb, storedToolbarColors.popupMenuTextHighlightArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePopupMenuBackgroundArgb, storedToolbarColors.popupMenuBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePopupMenuBackgroundHighlightArgb, storedToolbarColors.popupMenuBackgroundHighlightArgb)
    );

    const auto& storedListColors = defaultStoredLookAndFeel.listColors;
    const StoredLookAndFeel::ListColors listColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeArpeggioListItemBackgroundEvenArgb, storedListColors.arpeggioListItemBackgroundEvenArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeArpeggioListItemBackgroundOddArgb, storedListColors.arpeggioListItemBackgroundOddArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeArpeggioListItemBackgroundSelectedArgb, storedListColors.arpeggioListItemBackgroundSelectedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePitchListItemBackgroundEvenArgb, storedListColors.pitchListItemBackgroundEvenArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePitchListItemBackgroundOddArgb, storedListColors.pitchListItemBackgroundOddArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePitchListItemBackgroundSelectedArgb, storedListColors.pitchListItemBackgroundSelectedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeInstrumentListItemBackgroundEvenArgb, storedListColors.instrumentListItemBackgroundEvenArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeInstrumentListItemBackgroundOddArgb, storedListColors.instrumentListItemBackgroundOddArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeInstrumentListItemBackgroundSelectedArgb, storedListColors.instrumentListItemBackgroundSelectedArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeListItemTextArgb, storedListColors.listItemTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeListItemTextSelectedArgb, storedListColors.listItemTextSelectedArgb)
    );

    const auto& storedMeterColors = defaultStoredLookAndFeel.meterColors;
    const StoredLookAndFeel::MeterColors meterColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersBackgroundArgb, storedMeterColors.metersBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersScaleLinesArgb, storedMeterColors.metersScaleLinesArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersLabelBackgroundArgb, storedMeterColors.metersLabelBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersLabelArgb, storedMeterColors.metersLabelArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersShapeArgb, storedMeterColors.metersShapeArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersMuteFilterArgb, storedMeterColors.metersMuteFilterArgb)
    );

    const auto& storedLinkerColors = defaultStoredLookAndFeel.linkerColors;
    const StoredLookAndFeel::LinkerColors linkerColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerPlayCursorTimelineArgb, storedLinkerColors.linkerPlayCursorTimelineArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerPositionWithinSongRangeBackgroundArgb, storedLinkerColors.linkerPositionWithinSongRangeBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerPositionOutsideSongRangeBackgroundArgb, storedLinkerColors.linkerPositionOutsideSongRangeBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerPositionBorderArgb, storedLinkerColors.linkerPositionBorderArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeLinkerPositionCurrentlyPlayedBorderArgb, storedLinkerColors.linkerPositionCurrentlyPlayedBorderArgb)
    );

    const auto& storedEditorWithBarsColors = defaultStoredLookAndFeel.editorWithBarsColors;
    const StoredLookAndFeel::EditorWithBarsColors editorWithBarsColors(
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeBackground, storedEditorWithBarsColors.barSoundTypeBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeNoSoftNoHard, storedEditorWithBarsColors.barSoundTypeNoSoftNoHardArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeSoft, storedEditorWithBarsColors.barSoundTypeSoftArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeHard, storedEditorWithBarsColors.barSoundTypeHardArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeSoftToHard, storedEditorWithBarsColors.barSoundTypeSoftToHardArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeHardToSoft, storedEditorWithBarsColors.barSoundTypeHardToSoftArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarSoundTypeSoftAndHard, storedEditorWithBarsColors.barSoundTypeSoftAndHardArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarEnvelopeBackground, storedEditorWithBarsColors.barEnvelopeBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarEnvelopeSoft, storedEditorWithBarsColors.barEnvelopeSoftArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarEnvelopeHard, storedEditorWithBarsColors.barEnvelopeHardArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarNoiseBackground, storedEditorWithBarsColors.barNoiseBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarNoise, storedEditorWithBarsColors.barNoiseArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarPeriodBackground, storedEditorWithBarsColors.barPeriodBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarPeriod, storedEditorWithBarsColors.barPeriodArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarArpeggioNoteBackground, storedEditorWithBarsColors.barArpeggioNoteBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarArpeggioNote, storedEditorWithBarsColors.barArpeggioNoteArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarArpeggioOctaveBackground, storedEditorWithBarsColors.barArpeggioOctaveBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarArpeggioOctave, storedEditorWithBarsColors.barArpeggioOctaveArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarPitchBackground, storedEditorWithBarsColors.barPitchBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeBarPitch, storedEditorWithBarsColors.barPitchArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeAutoSpreadLoop, storedEditorWithBarsColors.autoSpreadLoopArgb)
            );

    const auto& storedPatternViewerColors = defaultStoredLookAndFeel.patternViewerColors;
    const StoredLookAndFeel::PatternViewerColors patternViewerColors(
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerBackgroundArgb, storedPatternViewerColors.patternViewerBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerNoNoteArgb, storedPatternViewerColors.patternViewerNoNoteArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerErrorArgb, storedPatternViewerColors.patternViewerErrorArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerNoEffectArgb, storedPatternViewerColors.patternViewerNoEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerArpeggioEffectArgb, storedPatternViewerColors.patternViewerArpeggioEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerPitchEffectArgb, storedPatternViewerColors.patternViewerPitchEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerVolumeEffectArgb, storedPatternViewerColors.patternViewerVolumeEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerResetEffectArgb, storedPatternViewerColors.patternViewerResetEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerInstrumentSpeedEffectArgb, storedPatternViewerColors.patternViewerInstrumentSpeedEffectArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerCursorTextHighlightArgb, storedPatternViewerColors.patternViewerCursorTextHighlightArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerCursorBackgroundArgb, storedPatternViewerColors.patternViewerCursorBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerSelectionArgb, storedPatternViewerColors.patternViewerSelectionArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerCursorLineBackgroundArgb, storedPatternViewerColors.patternViewerCursorLineBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerCursorLineHighlightTextArgb, storedPatternViewerColors.patternViewerCursorLineHighlightTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerHighlightedLineBackgroundPrimaryArgb, storedPatternViewerColors.patternViewerHighlightedLineBackgroundPrimaryArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerHighlightedLineBackgroundSecondaryArgb, storedPatternViewerColors.patternViewerHighlightedLineBackgroundSecondaryArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerHighlightTextPrimaryArgb, storedPatternViewerColors.patternViewerHighlightTextPrimaryArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerHighlightTextSecondaryArgb, storedPatternViewerColors.patternViewerHighlightTextSecondaryArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerTrackBorderArgb, storedPatternViewerColors.patternViewerTrackBorderArgb),

            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerTrackHeaderBackgroundOnArgb, storedPatternViewerColors.patternViewerTrackHeaderBackgroundOnArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerTrackHeaderBackgroundOffArgb, storedPatternViewerColors.patternViewerTrackHeaderBackgroundOffArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerTrackHeaderBackgroundChannelOnArgb, storedPatternViewerColors.patternViewerTrackHeaderBackgroundChannelOnArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerTrackHeaderBackgroundChannelOffArgb, storedPatternViewerColors.patternViewerTrackHeaderBackgroundChannelOffArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerLineTrackHeaderBackgroundArgb, storedPatternViewerColors.patternViewerLineTrackHeaderBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerLineTrackTextArgb, storedPatternViewerColors.patternViewerLineTrackTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerSpeedTrackHeaderBackgroundArgb, storedPatternViewerColors.patternViewerSpeedTrackHeaderBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerSpeedTrackTextArgb, storedPatternViewerColors.patternViewerSpeedTrackTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerEventTrackHeaderBackgroundArgb, storedPatternViewerColors.patternViewerEventTrackHeaderBackgroundArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerEventTrackTextArgb, storedPatternViewerColors.patternViewerEventTrackTextArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodePatternViewerPlayedLineBackgroundArgb, storedPatternViewerColors.patternViewerPlayedLineBackgroundArgb),

            XmlHelper::readUnsignedInt(xmlElement, nodeMetersVolumeArgb, storedPatternViewerColors.patternViewerMetersSoftwareBarArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersVolumeIfHardwareArgb, storedPatternViewerColors.patternViewerMetersHardwareBarArgb),
            XmlHelper::readUnsignedInt(xmlElement, nodeMetersNoiseArgb, storedPatternViewerColors.patternViewerMetersNoiseBarArgb)
    );
    
    return std::make_unique<StoredLookAndFeel>(
            XmlHelper::readInt(xmlElement, nodeId),
            XmlHelper::readString(xmlElement, nodeDisplayedName),
            XmlHelper::readBool(xmlElement, nodeReadOnly),
            
            generalColors,
            panelColors,
            buttonColors,
            toolbarColors,
            listColors,
            meterColors,
            linkerColors,
            editorWithBarsColors,
            patternViewerColors);
}

}   // namespace arkostracker
