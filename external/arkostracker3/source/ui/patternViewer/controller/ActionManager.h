#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../business/actions/patternViewer/SelectedData.h"
#include "../../../business/actions/patternViewer/TransposeRate.h"
#include "../../../song/Location.h"
#include "../../../song/cells/Cell.h"
#include "../../../song/cells/SpecialCell.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../CursorLocation.h"
#include "PasteData.h"
#include "Selection.h"

namespace arkostracker 
{

class PatternViewerControllerImpl;
class Selection;
class SongController;

/** Class used by the PatterViewer Controller, helps manages actions such as copy/paste, etc. */
class ActionManager
{
public:
    /** Abstract controller to the ActionManager. */
    class Controller
    {
    public:
        /** Destructor. */
        virtual ~Controller() = default;

        /** @return the current selection. */
        virtual Selection getCurrentSelection() const noexcept = 0;
        /** @return how many channel there are in the current Subsong. */
        virtual int getChannelCount() const noexcept = 0;
        /** @return how many channel there are in the given Subsong. */
        virtual int getChannelCount(const Id& subsongId) const noexcept = 0;
        /** @return the height of the current position. */
        virtual int getPositionHeight() const noexcept = 0;

        /** @return the current cursor location. */
        virtual CursorLocation getCursorLocation() const noexcept = 0;
        /** @return the current cursor location. */
        virtual Location getShownLocation() const noexcept = 0;

        /**
         * @return the length of the Subsong.
         * @param subsongId the ID of the Subsong. Must exist.
         */
        virtual int getSubsongLength(const Id& subsongId) const noexcept = 0;

        /**
         * @return a Cell from the currently *shown* tracks. If the input data is invalid, an empty Cell is returned.
         * @param channelIndex the channel index. May be invalid.
         * @param lineIndex the line index. May be invalid.
         */
        virtual Cell getCell(int channelIndex, int lineIndex) const noexcept = 0;

        /**
         * @return a Special Cell from the currently *shown* tracks. If the input data is invalid, an empty Cell is returned.
         * @param isSpeedTrack true if Speed Track, false if Event Track.
         * @param lineIndex the line index. May be invalid.
         */
        virtual SpecialCell getSpecialCell(bool isSpeedTrack, int lineIndex) const noexcept = 0;

        /** @return the Command Manager. */
        virtual juce::ApplicationCommandManager& getCommandManager() const noexcept = 0;
        /** @return the Song Controller. */
        virtual SongController& getSongController() const noexcept = 0;

        /** @return true if the record mode is on. */
        virtual bool isRecording() const noexcept = 0;

        /**
         * Selects an Instrument.
         * @param instrumentIndex the instrument index. Must be valid.
         */
        virtual void setSelectedInstrument(int instrumentIndex) noexcept = 0;

        /**
         * Selects a default effect.
         * @param effect the effect.
         */
        virtual void setSelectedEffect(Effect effect) noexcept = 0;

        /**
         * Selects an Expression.
         * @param isArpeggio true if Arpeggio, false if Pitch.
         * @param expressionIndex the expression index. May be invalid.
         */
        virtual void setSelectedExpression(bool isArpeggio, int expressionIndex) noexcept = 0;

        /** Notifies that an action has failed because Record is off. */
        virtual void notifyActionFailedBecauseRecordingOff() noexcept = 0;

        /**
         * Tries to move vertically. This loses the selection.
         * @param cellCount how many lines. May be negative.
         */
        virtual void tryToMoveVertically(int cellCount) noexcept = 0;
    };

    /**
     * Constructor.
     * @param controller the Controller.
     */
    explicit ActionManager(Controller& controller) noexcept;

    /** The user wants to copy the selection. */
    void onUserWantsToCopy() const noexcept;
    /** The user wants to cut the selection. */
    void onUserWantsToCut() const noexcept;
    /**
     * The user wants to paste the selection.
     * @param moveBelowAfter true to move below the copy after.
     * @param pasteContinuously true to paste up to the position height.
     */
    void onUserWantsToPaste(bool moveBelowAfter, bool pasteContinuously) const noexcept;
    /** The user wants to paste-mix the selection. */
    void onUserWantsToPasteMix() const noexcept;
    /** The user wants to delete what is selected. */
    void onUserWantsToClearSelection() const noexcept;

    /**
     * The user wants to transpose what is selected.
     * @param transposeRate of how fast to transpose.
     */
    void onUserWantsToTransposeSelection(TransposeRate transposeRate) const noexcept;

    /** The user wants to toggle the read-only tracks that are selected. */
    void onUserWantsToToggleReadOnly() const noexcept;

    /** The user wants to insert an empty cell where the cursor is. */
    void onUserWantsToInsert() const noexcept;
    /** The user wants to remove the cell where the cursor is, shifting the cells. */
    void onUserWantsToRemove() const noexcept;

    /** Opens the contextual menu. */
    void openContextualMenu() const noexcept;

    /** The user wants to capture the effect, expression, or nearest instrument. */
    void onUserWantsToCapture() const noexcept;

    /** The user wants to create an arpeggio from the current selection. Nothing happens if invalid. The UI should be checked that previously to allow this action. */
    void onUserWantsToGenerateArpeggio() noexcept;

    /** The user wants to create an instrument from the current selection. Nothing happens if invalid. The UI should be checked that previously to allow this action. */
    void onUserWantsToGenerateInstrument() noexcept;

    /**
     * @return a Selection, as marked by the cursor only.
     * NOTE: if the cursor was on a note, by default we want the instrument to be included too. This is useful to delete the instrument as well as the note, else legato
     * can never use a non-instrument on a deleted note. This behavior could be added as a flag, but this is not needed for now.
     * @param growToWholeCell true to grow the selection to the whole Cell/Special Cell.
     * */
    Selection getCursorAsSelection(bool growToWholeCell /*, bool selectInstrumentIfWasOnNote*/) const noexcept;

    /**
     * @return the selection data for various Actions, from a given selection.
     * @param selection the selection to build the data from.
     */
    SelectedData buildSelectedData(const Selection& selection) const noexcept;

    /** @return the selection for various Actions. */
    SelectedData buildSelectedData() const noexcept;
    /** @return the selection as the whole Pattern, for various Actions. */
    SelectedData buildSelectedDataAsPattern() const noexcept;
    /** @return the selection as the whole Subsong, for various Actions. */
    SelectedData buildSelectedDataAsCurrentSubsong() const noexcept;
    /** @return the selections with all the Subsongs, for various Actions. */
    std::vector<SelectedData> buildSelectedDataWithAllSubsongs() const noexcept;

    /**
     * @return the possible notes to generate an Arpeggio/instrument, from the current selection. If invalid, an empty set is returned.
     * One music channel must be selected for validity.
     */
    std::vector<int> getNotesForGeneration() const noexcept;

    /** @return the current selection, or the cursor if there is none. */
    Selection getSelectionOrCursor() const noexcept;

    /**
     * @return a new set of toggled minimized tracks from the cursor/selection.
     * @param minimizedChannelIndexes the input minimized channel indexes.
     */
    std::unordered_set<int> toggleMinimize(const std::unordered_set<int>& minimizedChannelIndexes) const noexcept;

private:
    /** Copies the selection to the clipboard. */
    void copySelectionToClipboard() const noexcept;

    /**
     * @return the possible Special Cells extracted from the Special Tracks from the selection.
     * None will be returned if the selection does not encompass a Special Track.
     * @param isSpeedTrack true of Speed Track, false if Event Track.
     * @param selection the selection.
     */
    std::vector<SpecialCell> extractSpecialCells(bool isSpeedTrack, const Selection& selection) const noexcept;

    /** @return the deserialized copy/cut data from the clipboard. If unable to do so, the returned height is 0. */
    static PasteData getPasteDataFromClipboard() noexcept;

    /**
     * Pastes the data from the clipboard.
     * @param pasteMix true to paste-mix.
     * @param moveBelowAfter true to move below the copy after.
     * @param pasteContinuously true to paste up to the position height.
     */
    void paste(bool pasteMix, bool moveBelowAfter, bool pasteContinuously) const noexcept;
    /** Clears the selection or cursor. */
    void clearSelection() const noexcept;

    /**
     * Builds a list of CaptureFlags, each for each normal track from the given selection.
     * @param selection the selection.
     * @param channelCount how many channels there are in the Subsong.
     * @return the list of CaptureFlags, if any.
     */
    static std::vector<CaptureFlags> buildCaptureFlags(const Selection& selection, int channelCount) noexcept;

    /**
     * @return the possible instrument of the Cell, if found.
     * @param channelIndex the channel index. Must be valid.
     * @param cellIndex the cell index. Must be valid.
     */
    OptionalInt getInstrumentIfPossible(int channelIndex, int cellIndex) const noexcept;

    /**
     * Called when the Generate Arpeggio dialog is closed, with data from the user.
     * @param notes the arpeggio notes.
     * @param arpeggioName the name of the new Arpeggio.
     * @param baseNote the base note, to be subtracted to each one.
     */
    void onGeneratedArpeggioReceived(const std::vector<int>& notes, const juce::String& arpeggioName, int baseNote) noexcept;

    /**
     * Called when the Generate Instrument dialog is closed, with data from the user.
     * @param instrumentName the name of the new Instrument.
     * @param usePeriods true to use periods, false to use notes/shifts.
     */
    void onGeneratedInstrumentReceived(const juce::String& instrumentName, bool usePeriods) noexcept;

    /** @return the Cell where the cursor is. If in a Special Track, an empty Cell is returned. */
    Cell getCell() const noexcept;

    /** @return the index of an Instrument if found near the cursor. */
    OptionalInt tryToCaptureInstrument() const noexcept;

    /**
     * Checks whether recording is possible. If false, returns false and notifies the controller.
     * This is useful to notify an action could not be performed because recording is off.
     */
    bool checkIfRecordingAndNotify() const noexcept;

    /**
     * @return the selection for a whole Subsong, for various Actions.
     * @param subsongId the SubsongId. Must exist.
     */
    SelectedData buildSelectedDataAsWholeSubsong(const Id& subsongId) const noexcept;

    Controller& controller;

    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker
