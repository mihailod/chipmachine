#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class StoredLookAndFeel;

/** Serializes a StoredLookAndFeel to XML, and the opposite. */
class XmlSerializer
{
public:
    /**
     * Serializes to XML the given StoredLookAndFeel.
     * @param storedLookAndFeel the object to serialize.
     * @return the XML root node.
     */
    std::unique_ptr<juce::XmlElement> serialize(const StoredLookAndFeel& storedLookAndFeel) const noexcept;

    /**
     * Deserializes an XML into a StoredLookAndFeel.
     * @param xmlElement the XML Element. Must be valid.
     * @param defaultStoredLookAndFeel used if an XML node could not be found. Will pick a default color in this LookAndFeel.
     * @return the StoredLookAndFeel.
     */
    std::unique_ptr<StoredLookAndFeel> deserialize(const juce::XmlElement& xmlElement, const StoredLookAndFeel& defaultStoredLookAndFeel) const noexcept;

private:
    juce::String nodeLookAndFeel = "lookAndFeel";
    juce::String nodeId = "id";
    juce::String nodeDisplayedName = "displayedName";
    juce::String nodeReadOnly = "readOnly";

    juce::String nodeTextArgb = "textArgb";

    juce::String nodePanelBackgroundFocusedArgb = "panelBackgroundFocusedArgb";
    juce::String nodePanelBackgroundUnfocusedArgb = "panelBackgroundUnfocusedArgb";
    juce::String nodePanelBorderFocusedArgb = "panelBorderFocusedArgb";
    juce::String nodePanelBorderUnfocusedArgb = "panelBorderUnfocusedArgb";
    juce::String nodePanelTabIconArgb = "panelTabIconArgb";
    juce::String nodePanelTabBackgroundUnselectedArgb = "panelTabBackgroundUnselectedArgb";
    juce::String nodePanelTabBackgroundSelectedArgb = "panelTabBackgroundSelectedArgb";

    juce::String nodePanelHandleIdleArgb = "panelHandleIdleArgb";
    juce::String nodePanelHandleMouseOverArgb = "panelHandleMouseOverArgb";

    juce::String nodeMetersBackgroundArgb = "metersBackgroundArgb";
    juce::String nodeMetersScaleLinesArgb = "metersScaleLinesArgb";
    juce::String nodeMetersLabelBackgroundArgb = "metersLabelBackgroundArgb";
    juce::String nodeMetersLabelArgb = "metersLabelArgb";
    juce::String nodeMetersShapeArgb = "metersShapeArgb";
    juce::String nodeMetersVolumeArgb = "metersVolumeArgb";
    juce::String nodeMetersVolumeIfHardwareArgb = "metersVolumeIfHardwareArgb";
    juce::String nodeMetersNoiseArgb = "metersNoiseArgb";
    juce::String nodeMetersMuteFilterArgb = "metersMuteFilterArgb";

    juce::String nodeDialogBackgroundArgb = "dialogBackgroundArgb";

    juce::String nodeDialogBorderArgb = "dialogBorderArgb";
    juce::String nodeButtonBorderArgb = "buttonBorderArgb";
    juce::String nodeButtonArgb = "buttonArgb";
    juce::String nodeButtonClickedArgb = "buttonClickedArgb";
    juce::String nodeButtonMouseOverArgb = "buttonMouseOverArgb";
    juce::String nodeButtonTextArgb = "buttonTextArgb";
    juce::String nodeButtonTextOffArgb = "buttonTextOffArgb";
    juce::String nodeTitleBarBackgroundArgb = "nodeTitleBarBackgroundArgb";
    juce::String nodeMenuBarArgb = "menuBarArgb";
    juce::String nodeMenuBarTextArgb = "menuBarTextArgb";
    juce::String nodeMenuBarTextHighlightArgb = "menuBarTextHighlightArgb";
    juce::String nodeMenuBarTextBackgroundHighlightArgb = "menuBarTextBackgroundHighlightArgb";
    juce::String nodePopupMenuTextArgb = "popupMenuTextArgb";
    juce::String nodePopupMenuTextHighlightArgb = "popupMenuTextHighlightArgb";
    juce::String nodePopupMenuBackgroundArgb = "popupMenuBackgroundArgb";
    juce::String nodePopupMenuBackgroundHighlightArgb = "popupMenuBackgroundHighlightArgb";
    juce::String nodeArpeggioListItemBackgroundEvenArgb = "arpeggioListItemBackgroundEvenArgb";
    juce::String nodeArpeggioListItemBackgroundOddArgb = "arpeggioListItemBackgroundOddArgb";
    juce::String nodeArpeggioListItemBackgroundSelectedArgb = "arpeggioListItemBackgroundSelectedArgb";
    juce::String nodePitchListItemBackgroundEvenArgb = "pitchListItemBackgroundEvenArgb";
    juce::String nodePitchListItemBackgroundOddArgb = "pitchListItemBackgroundOddArgb";
    juce::String nodePitchListItemBackgroundSelectedArgb = "pitchListItemBackgroundSelectedArgb";
    juce::String nodeInstrumentListItemBackgroundEvenArgb = "instrumentListItemBackgroundEvenArgb";
    juce::String nodeInstrumentListItemBackgroundOddArgb = "instrumentListItemBackgroundOddArgb";
    juce::String nodeInstrumentListItemBackgroundSelectedArgb = "instrumentListItemBackgroundSelectedArgb";
    juce::String nodeListItemTextArgb = "listItemTextArgb";
    juce::String nodeListItemTextSelectedArgb = "listItemTextSelectedArgb";

    juce::String nodeLinkerLoopStartEndArgb = "linkerLoopStartEndArgb";
    juce::String nodeLinkerPlayCursorTimelineArgb = "linkerPlayCursorTimelineArgb";
    juce::String nodeLinkerPositionWithinSongRangeBackgroundArgb = "linkerPositionWithinSongRangeBackgroundArgb";
    juce::String nodeLinkerPositionOutsideSongRangeBackgroundArgb = "linkerPositionOutsideSongRangeBackgroundArgb";
    juce::String nodeLinkerPositionBorderArgb = "linkerPositionBorderArgb";
    juce::String nodeLinkerPositionCurrentlyPlayedBorderArgb = "linkerPositionCurrentlyPlayedBorderArgb";

    juce::String nodeToolTipBackgroundArgb = "toolTipBackgroundArgb";
    juce::String nodeToolTipTextArgb = "toolTipTextArgb";

    juce::String nodeScrollBarArgb = "scrollBarArgb";

    juce::String nodePatternViewerBackgroundArgb = "patternViewerBackgroundArgb";
    juce::String nodePatternViewerNoNoteArgb = "patternViewerNoNoteArgb";
    juce::String nodePatternViewerErrorArgb = "patternViewerErrorArgb";
    juce::String nodePatternViewerNoEffectArgb = "patternViewerNoEffectArgb";
    juce::String nodePatternViewerArpeggioEffectArgb = "patternViewerArpeggioEffectArgb";
    juce::String nodePatternViewerPitchEffectArgb = "patternViewerPitchEffectArgb";
    juce::String nodePatternViewerVolumeEffectArgb = "patternViewerVolumeEffectArgb";
    juce::String nodePatternViewerResetEffectArgb = "patternViewerResetEffectArgb";
    juce::String nodePatternViewerInstrumentSpeedEffectArgb = "patternViewerInstrumentSpeedEffectArgb";
    juce::String nodePatternViewerCursorTextHighlightArgb = "patternViewerCursorTextHighlightArgb";
    juce::String nodePatternViewerCursorBackgroundArgb = "patternViewerCursorBackgroundArgb";
    juce::String nodePatternViewerSelectionArgb = "patternViewerSelectionArgb";
    juce::String nodePatternViewerCursorLineBackgroundArgb = "patternViewerCursorLineBackgroundArgb";
    juce::String nodePatternViewerCursorLineHighlightTextArgb = "patternViewerCursorLineHighlightTextArgb";
    juce::String nodePatternViewerHighlightedLineBackgroundPrimaryArgb = "patternViewerHighlightedLineBackgroundPrimaryArgb";
    juce::String nodePatternViewerHighlightedLineBackgroundSecondaryArgb = "patternViewerHighlightedLineBackgroundSecondaryArgb";
    juce::String nodePatternViewerHighlightTextPrimaryArgb = "patternViewerHighlightTextPrimaryArgb";
    juce::String nodePatternViewerHighlightTextSecondaryArgb = "patternViewerHighlightTextSecondaryArgb";
    juce::String nodePatternViewerTrackBorderArgb = "patternViewerTrackBorderArgb";

    juce::String nodePatternViewerTrackHeaderBackgroundOnArgb = "patternViewerTrackHeaderBackgroundOnArgb";
    juce::String nodePatternViewerTrackHeaderBackgroundOffArgb = "patternViewerTrackHeaderBackgroundOffArgb";
    juce::String nodePatternViewerTrackHeaderBackgroundChannelOnArgb = "patternViewerTrackHeaderBackgroundChannelOnArgb";
    juce::String nodePatternViewerTrackHeaderBackgroundChannelOffArgb = "patternViewerTrackHeaderBackgroundChannelOffArgb";
    juce::String nodePatternViewerLineTrackHeaderBackgroundArgb = "patternViewerLineTrackHeaderBackgroundArgb";
    juce::String nodePatternViewerLineTrackTextArgb = "patternViewerLineTrackTextArgb";
    juce::String nodePatternViewerSpeedTrackHeaderBackgroundArgb = "patternViewerSpeedTrackHeaderBackgroundArgb";
    juce::String nodePatternViewerSpeedTrackTextArgb = "patternViewerSpeedTrackTextArgb";
    juce::String nodePatternViewerEventTrackHeaderBackgroundArgb = "patternViewerEventTrackHeaderBackgroundArgb";
    juce::String nodePatternViewerEventTrackTextArgb = "patternViewerEventTrackTextArgb";
    juce::String nodePatternViewerPlayedLineBackgroundArgb = "patternViewerPlayedLineBackgroundArgb";

    juce::String nodeBarSoundTypeBackground = "barSoundTypeBackground";
    juce::String nodeBarSoundTypeNoSoftNoHard = "barSoundTypeNoSoftNoHard";
    juce::String nodeBarSoundTypeSoft = "barSoundTypeSoft";
    juce::String nodeBarSoundTypeHard = "barSoundTypeHard";
    juce::String nodeBarSoundTypeSoftToHard = "barSoundTypeSoftToHard";
    juce::String nodeBarSoundTypeHardToSoft = "barSoundTypeHardToSoft";
    juce::String nodeBarSoundTypeSoftAndHard = "barSoundTypeSoftAndHard";
    juce::String nodeBarEnvelopeBackground = "barEnvelopeBackground";
    juce::String nodeBarEnvelopeSoft = "barEnvelopeSoft";
    juce::String nodeBarEnvelopeHard = "barEnvelopeHard";
    juce::String nodeBarNoiseBackground = "barNoiseBackground";
    juce::String nodeBarNoise = "barNoise";
    juce::String nodeBarPeriodBackground = "barPeriodBackground";
    juce::String nodeBarPeriod = "barPeriod";
    juce::String nodeBarArpeggioNoteBackground = "barArpeggioNoteBackground";
    juce::String nodeBarArpeggioNote = "barArpeggioNote";
    juce::String nodeBarArpeggioOctaveBackground = "barArpeggioOctaveBackground";
    juce::String nodeBarArpeggioOctave = "barArpeggioOctave";
    juce::String nodeBarPitchBackground = "barPitchBackground";
    juce::String nodeBarPitch = "barPitch";
    juce::String nodeAutoSpreadLoop = "autoSpreadLoop";

    juce::String nodeTestNotExisting = "testNotExisting";
};


}   // namespace arkostracker

