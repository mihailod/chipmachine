#include "Tips.h"

namespace arkostracker
{
const std::vector<Tip>& Tips::getTips() noexcept
{
    static const auto allTips = std::vector{
        Tip::buildVideoTip(
            juce::translate("Remap instruments by drag'n'dropping them! The patterns are automatically modified accordingly."),
            "instrumentDragNDrop.zip"
        ),

        Tip::buildVideoTip(
            juce::translate("Patterns can be drag'n'dropped in the linker! By pressing [ctrl], they are duplicated."),
            "linkerDragNDrop.zip"
        ),

        Tip::buildImageTip(
            juce::translate("Explore the mysteries of any YMs and VGMs via the Streamed Music Analyzer! You can even extract instruments from their innards!"),
            "streamedMusicAnalyzer.png"
        ),

        Tip::buildVideoTip(
            juce::translate("You can increase/decrease effect values too, exactly like transpositions! Via XXX or XXX, or with bigger steps using XXX or XXX."),
            "transposeEffects.zip",
            { patternViewerTransposePlusOne, patternViewerTransposeMinusOne, patternViewerTransposePlusOneOctave, patternViewerTransposeMinusOneOctave }
        ),

        Tip::buildVideoTip(
            juce::translate("Instead of entering the right note from the start, type one and transpose it! Via XXX or XXX, or octave using XXX or XXX."),
            "transposeSingleNote.zip",
            { patternViewerTransposePlusOne, patternViewerTransposeMinusOne, patternViewerTransposePlusOneOctave, patternViewerTransposeMinusOneOctave }
        ),

        Tip::buildVideoTip(
            juce::translate("You can transpose blocks by semi-tones using XXX or XXX, or octave using XXX or XXX."),
            "transposeNotes.zip",
            { patternViewerTransposePlusOne, patternViewerTransposeMinusOne, patternViewerTransposePlusOneOctave, patternViewerTransposeMinusOneOctave }
        ),

        Tip::buildVideoTip(
            juce::translate("In the pattern Viewer, use left-click on the channel number to mute/unmute a track, or right-click to solo/unsolo it."),
            "muteInPatternViewer.zip"
        ),

        Tip::buildImageTip(
            juce::translate(
                "Do you want to hear the music on the real hardware, in real-time? Then plug your hardware via serial and open the stream with the top-right icon (File > Setup > Serial to alternatively set up the connection)."),
            "serial.png",
            { }, 400
        ),

        Tip::buildImageTip(
            "Don't like the name of the effects?\nThey can all be redefined in File > Setup > Pattern Viewer",
            "setupPatternViewer.png"
        ),

        Tip::buildImageTip(
            juce::translate("Generate complex arpeggio from your notes!"),
            "generateArpeggio.png",
            { }, 400
        ),

        Tip::buildImageTip(
            juce::translate("Record part of a track into another instrument! Very useful for creating SFXs, or even crazier instruments!"),
            "recordInstrument.png",
            { }, 400
        ),

        Tip::buildImageTip(
            "In the pattern viewer, if you erase the instrument number besides a note, the note will change but the instrument will not restart. This is called a \"legato\".",
            "legato.png",
            { }, 180
        ),

        Tip::buildImageTip(
            "Transpositions can be shown in the pattern viewer. This allows transposing a track without modifying a single note of it!",
            "transpose.png",
            { }, 380
        ),

        Tip::buildImageTip(
            "Each instrument can be defined a color. Very useful to determine which instrument is a drum, a melody, a bass, etc. in the pattern viewer. Never get lost!",
            "instrumentColor.png",
            { }, 380
        ),

        Tip::buildImageTip(
            "Listen to an instrument by clicking on its left symbol.",
            "listenToInstrument.png",
            { }, 250
        ),

        Tip::buildImageTip(
            "The pattern viewer contains a toolbox to transpose, erase notes, but also swap tracks!",
            "patternViewerToolbox.png",
            { },
            250
        ),

        Tip::buildImageTip(
            juce::translate("Almost all the keyboard shortcuts can be redefined!\nGo to File > Setup > Keyboard mapping."),
            "keyboardMapping.png"
        ),
        Tip::buildImageTip(
            juce::translate("Want to create a song with more than 3 channels?\nGo to Edit > Subsong properties, and add a new PSG at the bottom."),
            "addPsg.png"
        ),
        Tip::buildImageTip(
            juce::translate("Layouts represent how panels (linker, pattern viewer, etc.) are placed in one page. Click on one or use a keystroke (such as XXX)."),
            "restoreLayout.png",
            { restoreLayout1 }
        ),
        Tip::buildVideoTip(
            juce::translate("In the Meters panel, use left-click to mute/unmute a track, or right-click to solo/unsolo it."),
            "muteInMeters.zip",
            { }, 350
        ),

        Tip::buildTextTip(
            "It is usually more versatile to have simple instruments, and add complexity with track effects (such as arpeggio, pitch, change of speed)."
        ),
        Tip::buildTextTip(
            "Remember the limitation of the AY/YM: only one hardware envelope and noise per PSG! Using more than one may sound strange. A channel has priority over the previous."
        ),
        Tip::buildTextTip(
            "No need to create two instruments with different speeds, use the S effect to force a new speed."
        ),
        Tip::buildTextTip(
            "Inline arpeggios in tracks (b37, c37c for example) have the maximum speed. You can override this with the W effect (set arpeggio speed) besides the B/C effect."
        ),
        Tip::buildTextTip(
            "Pitch table effect (P effect) can have its speed overridden with the X effect. Just write the X effect besides the P effect."
        ),

        Tip::buildTextTip(
            juce::translate("In the pattern viewer, you can play the song from your cursor thanks to XXX."),
            { playPatternFromCursorOrBlock }
        ),
        Tip::buildTextTip(
            juce::translate("Tired of using a computer keyboard to enter your notes? Plug any MIDI keyboard, set it up in File > Setup > Audio, and you're ready to go!")
        ),
        Tip::buildTextTip(
            juce::translate("There are several Paste modes! Paste mix XXX to skip empty notes, paste+move XXX to paste and move below, and paste continuously XXX "
                            "to paste till the end of the position."),
            { patternViewerPasteMix, patternViewerPasteAndMoveBelow, patternViewerPasteContinuously }
        ),

        Tip::buildImageTip(
            juce::translate("You can customize a track with a name and color! Simply click on the track header."),
            "trackColor.png"
        ),
        Tip::buildImageTip(
            juce::translate("Want to test your song quickly? You can export as DSK or SNA directly! The second quick export can be directly used in a production."),
            "quickExport.png"
        ),
    };

    return allTips;
}

} // namespace arkostracker
