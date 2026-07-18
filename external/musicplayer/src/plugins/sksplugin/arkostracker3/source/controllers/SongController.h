#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

#include "../business/actions/patternViewer/TransposeRate.h"
#include "../business/model/Properties.h"
#include "../song/Location.h"
#include "../song/Song.h"
#include "../ui/patternViewer/CursorLocation.h"
#include "../ui/patternViewer/controller/PasteData.h"
#include "../ui/patternViewer/controller/dataItem/Digit.h"
#include "../ui/patternViewer/controller/dataItem/EffectDigit.h"
#include "../ui/patternViewer/controller/dataItem/EffectNumber.h"
#include "../utils/Observers.h"
#include "observers/InstrumentChangeObserver.h"
#include "observers/TrackChangeObserver.h"

namespace arkostracker 
{

class ExpressionChangeObserver;
class MainController;
class SongMetadataObserver;
class SubsongMetadataObserver;
class LinkerObserver;
class SelectedData;

/**
 * This Controller:
 * - holds the Song.
 * - can register observers to it.
 * - performs the Commands on the Song.
 *
 * It does NOT:
 * - lock the Song.
 *
 * Only mostly used access methods (for Song) are defined here, because most actions should be done via performOn[Const]Subsong.
 * We don't want this class to become too huge.
 */
class SongController
{
public:
    /** Destructor. */
    virtual ~SongController() = default;

    /**
     * Changes the song. This only changes the song of THIS controller!
     * This forces the location of this Controller at the beginning of the first Subsong.
     * No observation is called.
     */
    virtual void setSong(std::shared_ptr<Song> newSong) = 0;

    /** @return the Song. */
    virtual std::shared_ptr<Song> getSong() = 0;
    /** @return the immutable Song. */
    virtual std::shared_ptr<const Song> getConstSong() const = 0;

    /** @return the Main Controller, as a convenience. */
    virtual MainController& getMainController() = 0;

    /** @return the id of the current Subsong. This is the shown Subsong, NOT the played one. */
    virtual Id getCurrentSubsongId() const noexcept = 0;
    /** @return the location being current viewed. This is NOT the location being played. */
    virtual Location getCurrentViewedLocation() const noexcept = 0;

    /**
     * Changes the current location. If out of bounds, it is corrected. However, the Subsong index must be valid!
     * Nothing happens if it was already here. Notifies.
     * If it was playing, changes the new played location accordingly. If not playing changes the future location to be played.
     * @param location the new current location.
     * @param alsoChangeLine false by default, the line does not change when changing position.
     */
    virtual void setCurrentLocation(const Location& location, bool alsoChangeLine) noexcept = 0;

    /**
     * Stores the current location. Nothing else happens.
     * @param location the new location.
     */
    virtual void storeCurrentLocation(const Location& location) noexcept = 0;

    /** @return the observers on the Song Metadata, on which a class can subscribe. */
    virtual Observers<SongMetadataObserver>& getSongMetadataObservers() noexcept = 0;

    /** @return the Observers on the Subsongs Metadata. */
    virtual Observers<SubsongMetadataObserver>& getSubsongMetadataObservers() noexcept = 0;

    /** @return the Observers on the Linker. */
    virtual Observers<LinkerObserver>& getLinkerObservers() noexcept = 0;

    /** @return the song title. */
    virtual juce::String getTitle() const noexcept = 0;
    /** @return the song author. */
    virtual juce::String getAuthor() const noexcept = 0;
    /** @return the song composer. */
    virtual juce::String getComposer() const noexcept = 0;
    /** @return the song comments. */
    virtual juce::String getComments() const noexcept = 0;

    /**
     * Sets the Song metadata. If none has changed, nothing is done.
     * @param newTitle the possible new title.
     * @param newAuthor the possible new author.
     * @param newComposer the possible new composer.
     * @param newComments the possible new comments.
     */
    virtual void setSongMetadata(OptionalValue<juce::String> newTitle, OptionalValue<juce::String> newAuthor, OptionalValue<juce::String> newComposer,
                                 OptionalValue<juce::String> newComments) noexcept = 0;


    // ========================================================================

    /**
     * Convenience method to perform an action on the current Subsong, without modifying it. All are locked during the process.
     * @param subsongId the id of the Subsong. Must be valid. Nothing happens if it is not found (asserts).
     * @param function the function to call.
     */
    virtual void performOnConstSubsong(const Id& subsongId, const std::function<void(const Subsong&)>& function) const noexcept = 0;

    /**
     * Convenience method to perform an action on the current Subsong. All are locked during the process.
     * @param subsongId the id of the Subsong. Must be valid. Nothing happens if it is not found (asserts).
     * @param function the function to call.
     */
    virtual void performOnSubsong(const Id& subsongId, const std::function<void(Subsong&)>& function) const noexcept = 0;

    // ========================================================================

    /** @return true if an Undo is possible. */
    virtual bool canUndo() const = 0;
    /** @return true if a Redo is possible. */
    virtual bool canRedo() const = 0;
    /** Performs an Undo, if possible. */
    virtual void undo() = 0;
    /** Performs a Redo, if possible. */
    virtual void redo() = 0;
    /** @returns the displayable name of the Undo action. */
    virtual juce::String getUndoDescription() = 0;
    /** @returns the displayable name of the Redo action. */
    virtual juce::String getRedoDescription() = 0;
    /** Clears the Undo/redo stack. */
    virtual void clearUndo() = 0;
    /** Marks the song as saved. This notifies observers. */
    virtual void markSongAsSaved() noexcept = 0;
    /** @return true if the song is modified. */
    virtual bool isSongModified() const noexcept = 0;

    /**
     * Performs an Action, to be Undo-able and Redo-able. A transaction is created so that the Undo only undoes the given action.
     * This shouldn't be called directly most of the time, but can be anyway when a controller builds some specific actions.
     * @param action the action.
     * @param name the name to be displayed
     * @return true if the action was performed (and thus added to the Undo/redo list).
     */
    virtual bool performAction(std::unique_ptr<juce::UndoableAction> action, const juce::String& name) noexcept = 0;

    // Cells.
    // =========================================================================

    /** @return the observers to be notified of change in Tracks (cell modified). */
    virtual Observers<TrackChangeObserver>& getTrackObservers() noexcept = 0;

    /**
     * Sets a cell data to a track.
     * @param location where to write. May be invalid.
     * @param channelIndex the channel. May be invalid.
     * @param newNote the note, or absent if none must be written.
     * @param newInstrument the instrument, or absent if none must be written.
     * @param newEffectDigit an effect digit, or absent if none must be written.
     * @param newEffectNumber an effect number, or absent if none must be written.
     * @param newInstrumentDigit the possible instrument digit.
     */
    virtual void setCell(const Location& location, int channelIndex, OptionalValue<Note> newNote, OptionalInt newInstrument,
                         OptionalValue<EffectDigit> newEffectDigit, OptionalValue<EffectNumber> newEffectNumber,
                         OptionalValue<Digit> newInstrumentDigit) noexcept = 0;

    /**
     * Pastes cells in tracks/special tracks.
     * @param location where to paste.
     * @param cursorLocation where to paste occurs.
     * @param pasteData the data to paste.
     * @param pasteMix true to paste-mix.
     * @param pasteContinuously true to paste continuously up to the position height.
     */
    virtual void pasteCells(const Location& location, const CursorLocation& cursorLocation, const PasteData& pasteData, bool pasteMix, bool pasteContinuously) noexcept = 0;

    /**
     * Transposes a part of the Subsong.
     * @param transposeRate of how many semitones to transpose.
     * @param selectedData info about the data to process.
     */
    virtual void transpose(TransposeRate transposeRate, const SelectedData& selectedData) noexcept = 0;

    /**
     * Transposes a part of the Subsong.
     * @param subsongId the Subsong Id.
     * @param trackIndexes the track indexes. May be empty.
     * @param speedTrackIndex index if the speed track must be toggled.
     * @param eventTrackIndex index if the event track must be toggled.
     */
    virtual void toggleTracksReadOnly(Id subsongId, const std::set<int>& trackIndexes, OptionalInt speedTrackIndex, OptionalInt eventTrackIndex) noexcept = 0;

    /**
     * Inserts an empty cell (normal or special), shifting the rest.
     * @param location where to insert.
     * @param cursorLocation where the cursor is.
     */
    virtual void insertCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept = 0;

    /**
     * Inserts an empty cell (normal or special), shifting the rest.
     * @param location where to insert.
     * @param cursorLocation where the cursor is.
     */
    virtual void removeCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept = 0;

    /**
     * Clears the given Selection.
     * @param selectedData the selection.
     */
    virtual void clearSelection(const SelectedData& selectedData) noexcept = 0;


    // Special Cells.
    // =========================================================================

    /**
     * Sets a Special Cell data to a track.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param location where to write. May be invalid.
     * @param digit the digit (index and value).
     */
    virtual void setSpecialCell(bool isSpeedTrack, const Location& location, Digit digit) noexcept = 0;


    // Expressions.
    // =========================================================================

    /**
     * @return the Observers on the Expressions.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual Observers<ExpressionChangeObserver>& getExpressionObservers(bool isArpeggio) noexcept = 0;

    /**
     * @return the names of all the Expression, except for the first one.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual std::vector<juce::String> getExpressionNames(bool isArpeggio) const noexcept = 0;
    /**
     * @returns the name of the Expression. If not found, an empty String is returned (abnormal).
     * @param isArpeggio true if Arpeggio, false if Pitch.
    * @param expressionId the ID.
     */
    virtual juce::String getExpressionName(bool isArpeggio, const Id& expressionId) const noexcept = 0;

    /**
     * @returns the name of the Expression. If not found, an empty String is returned (abnormal).
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionIndex the index. Should be valid.
     */
    virtual juce::String getExpressionNameFromIndex(bool isArpeggio, int expressionIndex) const noexcept = 0;

    /**
     * Renames an Expression. Notifies the listeners.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID. It must exist.
     * @param newName the new name.
     */
    virtual void setExpressionName(bool isArpeggio, const Id& expressionId, const juce::String& newName) noexcept = 0;

    /**
     * Deletes an Expression. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param indexesToDelete the indexes (should be >0, as 0 cannot be selected). Must be valid.
     */
    virtual void deleteExpressions(bool isArpeggio, std::set<int> indexesToDelete) noexcept = 0;

    /**
     * @return how many there are (>=0, but in practice, should be >=1 as the default one is always present).
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual int getExpressionCount(bool isArpeggio) const noexcept = 0;

    /**
     * @return true if the Expression is read-only (i.e. is the first one).
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID of the Expression. If not found, returns true.
     */
    virtual bool isReadOnlyExpression(bool isArpeggio, const Id& expressionId) const noexcept = 0;

    /**
     * Inserts the given Expressions. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionIndexWhereToInsert the index where to insert. If -1, adds to the end. Must never be 0! Because we won't insert before the 0th Expression.
     * @param expressions the Expressions to insert.
     */
    virtual void insertExpressions(bool isArpeggio, int expressionIndexWhereToInsert, std::vector<std::unique_ptr<Expression>> expressions) noexcept = 0;

    /**
     * Moves Expressions. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param indexesToDelete the indexes to delete. Must never be 0! Because we won't insert before the 0th Expression.
     * @param destinationIndex the destination index, without taking in account the moved items. Must never be 0!
     */
    virtual void moveExpressions(bool isArpeggio, const std::set<int>& indexesToDelete, int destinationIndex) noexcept = 0;

    /**
     * @return a copy of an Expression. If invalid, an empty one is returned.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID.
     */
    virtual Expression getExpression(bool isArpeggio, const Id& expressionId) noexcept = 0;

    /**
     * Gets the Expression Handler, to perform atomic actions on them.
     * @param isArpeggio true to receive the Arpeggio Handler, false for the Pitch Handler.
     */
    virtual ExpressionHandler& getExpressionHandler(bool isArpeggio) noexcept = 0;

    /**
     * Sets the cell of an Expression. If the cell index is out of bounds, cells are added with the new value. The end may be updated.
     * Nothing happens if the value is the same, or if the cell index is <0 (asserts).
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param expressionId the ID. It must exist.
     * @param cellIndex the cell index (>=0).
     * @param value the new value.
     */
    virtual void setExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex, int value) noexcept = 0;

    /**
     * Generic method to modify cells of an Expression (such as generating/modifying cells). Out of bounds cells will be added.
     * @param expressionId the ID of the Expression.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param cellIndex the cell index where to start. May be invalid (out of bounds).
     * @param cellCountToModify how many cells to modify.
     * @param actionName the name of the action, to be shown in the UndoManager.
     * @param actionOnCell the action to perform on the value. If out of bounds, a new one is generated.
     */
    virtual void applyOnExpressionCells(const Id& expressionId, bool isArpeggio, int cellIndex, int cellCountToModify, const juce::String& actionName,
        std::function<int(int iterationIndex, int initialValue)> actionOnCell) noexcept = 0;

    /**
     * @return true if the Expression is an arpeggio, false if Pitch. It must exist, else the result is meaningless.
     * @param expressionId the ID of the expression.
     */
    virtual bool isExpressionArpeggio(const Id& expressionId) const noexcept = 0;

    /**
     * Sets the metadata (loop, etc.) to an Expression.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID. It must exist.
     * @param newLoopStart the possible loop start, if any. Must be valid.
     * @param newEnd the possible end, if any. Must be valid.
     * @param newSpeed the new speed, if any. Must be valid.
     * @param newShift the new shift, if any. Must be valid.
     */
    virtual void setExpressionMetadata(bool isArpeggio, const Id& expressionId, OptionalInt newLoopStart, OptionalInt newEnd,
                                       OptionalInt newSpeed, OptionalInt newShift) = 0;

    /**
     * @return the possible index of the Expression which ID is given.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID. If not found, empty is returned.
     */
    virtual OptionalInt getExpressionIndex(bool isArpeggio, const Id& expressionId) const noexcept = 0;

    /**
     * @return the possible ID of the Expression, from its index.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionIndex the index. May be invalid.
     */
    virtual OptionalId getExpressionId(bool isArpeggio, int expressionIndex) const noexcept = 0;

    /**
     * @return the ID of the last Expression.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    virtual Id getLastExpressionId(bool isArpeggio) const noexcept = 0;

    /**
     * Duplicates an Expression Cell. If out of bounds, the given cell is spread. Also, if <0, but assert.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID of the expression. It must exist.
     * @param cellIndex the cell index (>=0).
     * @return true if the action was performed.
     */
    virtual bool duplicateExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept = 0;

    /**
     * Deletes an Expression Cell. If out of bounds, the given cell is spread. If <0, nothing happens, but assert.
     * Cannot erase the last cell.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID of the expression. It must exist.
     * @param cellIndex the cell index (>=0).
     * @return true if the action was performed.
     */
    virtual bool deleteExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept = 0;


    // Linker.
    // =======================================================================

    /**
     * Sets the marker on a Position. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the index of the Position. Must be valid.
     * @param newName the new name. May be empty to "delete" the marker (only hidden).
     * @param newColor the new color. Irrelevant if hidden.
     */
    virtual void setPositionMarker(const Id& subsongId, int positionIndex, const juce::String& newName, juce::Colour newColor) noexcept = 0;

    /**
     * Modifies an existing Position. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the index of the Position to modify. Must be valid.
     * @param position the Position to replace the old Position with.
     */
    virtual void modifyPosition(const Id& subsongId, int positionIndex, const Position& position) noexcept = 0;

    /**
     * Moves a marker into another. Nothing happens if they are the same. The source marker becomes empty. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param sourcePosition the index of the source Position. Must be valid.
     * @param destinationPosition the index of the destination Position. Must be valid.
     */
    virtual void movePositionMarker(const Id& subsongId, int sourcePosition, int destinationPosition) noexcept = 0;

    /**
     * Duplicates positions. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionsToDuplicate the positions to duplicate.
     * @param positionAfterWhichToInsert where to insert the positions.
     * @param insertAfter true if the Positions must be put after the destination, false if before.
     * @param alsoDuplicateMarker true to also duplicate the marker. Most of the time, we don't want this.
     */
    virtual void duplicatePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int positionAfterWhichToInsert,
                                    bool insertAfter, bool alsoDuplicateMarker) noexcept = 0;

    /**
     * Creates a new position and pattern, after the given one. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionAfterWhichToInsert where to insert the positions.
     */
    virtual void createNewPositionAndPattern(const Id& subsongId, int positionAfterWhichToInsert) noexcept = 0;

    /**
     * Clones positions. This creates new patterns. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionsToDuplicate the positions to duplicate.
     * @param positionAfterWhichToInsert where to insert the positions.
     * @param insertAfter true if the Positions must be put after the destination, false if before.
     */
    virtual void clonePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int positionAfterWhichToInsert,
                                    bool insertAfter) noexcept = 0;

    /**
     * Deletes positions. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionsToDelete the positions to delete.
     * @param deleteOrCut true if the Action is called Delete in the Undo/Redo, false if called Cut. That is the ONLY difference.
     */
    virtual void deletePositions(const Id& subsongId, const std::set<int>& positionsToDelete, bool deleteOrCut) noexcept = 0;

    /**
     * Moves Positions. The original positions may be sparse, but they will be put in order, linearly, on the destination.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positions the Positions to move.
     * @param destinationPosition the original position where to put them.
     * @param insertAfter true if the Potions must be moved after the destination, false if before.
     */
    virtual void movePositions(const Id& subsongId, const std::set<int>& positions, int destinationPosition, bool insertAfter) noexcept = 0;

    /**
     * Inserts the given Positions.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positions the "real" Positions to insert.
     * @param destinationPosition the original position where to put them.
     * @param insertAfter true if the Potions must be moved after the destination, false if before.
     * @param alsoDuplicateMarker true to also duplicate the marker. Most of the time, we don't want this.
     */
    virtual void insertPositions(const Id& subsongId, const std::vector<Position>& positions, int destinationPosition, bool insertAfter, bool alsoDuplicateMarker) noexcept = 0;

    /**
     * Sets the height of a Position.
     * @param location where the position is. Must be valid.
     * @param newHeight the new height. Must be valid.
     */
    virtual void setPositionHeight(const Location& location, int newHeight) noexcept = 0;

    /**
     * Sets the transposition of a channel of a Position.
     * @param location where the position is. Must be valid.
     * @param channelIndex the channel index.
     * @param newTransposition the transposition.
     */
    virtual void setPositionTransposition(const Location& location, int channelIndex, int newTransposition) noexcept = 0;

    /**
     * Increases the pattern index of a position, with bounds checked.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the index of the position to modify. Must be valid.
     * @param increaseStep how much to increase. May be negative.
     */
    virtual void increasePatternIndex(const Id& subsongId, int positionIndex, int increaseStep) noexcept = 0;

    /**
     * Renames/set a color to a Music Track.
     * @param location where the position is. Must be valid.
     * @param channelIndex the channel index.
     * @param newName the name.
     * @param newColor the new color, or none.
     */
    virtual void setTrackNameAndColor(const Location& location, int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) noexcept = 0;

    /**
     * Renames a Special Track.
     * @param location where the position is. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param newName the name.
     */
    virtual void setSpecialTrackName(const Location& location, bool isSpeedTrack, const juce::String& newName) noexcept = 0;

    /**
     * Modifies an existing Pattern. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param patternIndex the index of the Pattern to modify. Must be valid.
     * @param pattern the Pattern to replace the old Pattern with.
     */
    virtual void modifyPattern(const Id& subsongId, int patternIndex, const Pattern& pattern) noexcept = 0;

    /**
     * Modifies existing Positions, and the color of their Pattern. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndexes the positions of the Positions. Must be valid.
     * @param newHeight the new height to set to all the Positions.
     * @param newPatternColorArgb the new color to set to all the Patterns of the Positions.
     */
    virtual void modifyPositionsData(const Id& subsongId, const std::set<int>& positionIndexes, OptionalInt newHeight,
                                     OptionalValue<juce::uint32> newPatternColorArgb) noexcept = 0;

    /**
     * Creates a Track Link.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param sourceChannelIndex the channel index from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     * @param targetChannelIndex the channel index to link to.
     */
    virtual void createTrackLink(const Id& subsongId, int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) = 0;

    /**
     * Unlinks a linked Track.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     * @param channelIndexToUnlink the channel index to remove the link from.
     */
    virtual void unlinkTrack(const Id& subsongId, int positionIndexToUnlink, int channelIndexToUnlink) noexcept = 0;

    /**
     * Creates a Special Track Link.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     */
    virtual void createSpecialTrackLink(const Id& subsongId, bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) = 0;

    /**
     * Unlinks a linked Special Track.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param positionIndexToUnlink the index of the position to remove the link from.
     */
    virtual void unlinkSpecialTrack(const Id& subsongId, bool isSpeedTrack, int positionIndexToUnlink) noexcept = 0;

    /**
     * Clears all the Patterns and Tracks of a Subsong, and create one position.
     * @param subsongId the id of the Subsong. Must be valid.
     */
    virtual void clearPatterns(const Id& subsongId) noexcept = 0;

    /**
     * Rearranges the patterns.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param deleteUnused true to also delete the unused patterns.
     */
    virtual void rearrangePatterns(const Id& subsongId, bool deleteUnused) noexcept = 0;


    // Subsong Metadata
    // ======================================================================

    /**
     * @return how many channels there are.
     * @param subsongId the id of the Subsong. Must be valid.
     */
    virtual int getChannelCount(const Id& subsongId) const noexcept = 0;

    /** @return how many channels there are in the current Subsong. */
    virtual int getChannelCount() const noexcept = 0;

    /**
     * Sets the loop start/end. Nothing happens if they are the same. Notifies the listeners.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param loopStartPosition the loop start position. Must be valid.
     * @param endPosition the end position. Must be valid.
     */
    virtual void setLoopStartAndEnd(const Id& subsongId, int loopStartPosition, int endPosition) noexcept = 0;

    /**
     * @return the PSG metadata for the given Subsong.
     * @param subsongId the id of the Subsong. Must be valid.
     */
    virtual std::vector<Psg> getPsgs(const Id& subsongId) const noexcept = 0;

    /**
     * @return the PSG count for the given Subsong.
     * @param subsongId the id of the Subsong. Must be valid.
     */
    virtual int getPsgCount(const Id& subsongId) const noexcept = 0;


    // Instruments
    // ======================================================================

    /** @return how many Instruments there are. */
    virtual int getInstrumentCount() const noexcept = 0;

    /**
     * @return the possible ID of the Instrument which index is given.
     * @param index the index. If invalid, empty is returned.
     */
    virtual OptionalId getInstrumentId(int index) const noexcept = 0;

    /** @return the ID of the last Instrument. */
    virtual Id getLastInstrumentId() const noexcept = 0;

    /**
     * @return the possible index of the Instrument which ID is given.
     * @param instrumentId the ID. If not found, empty is returned.
     */
    virtual OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept = 0;

    /** Builds a map linking the instrument to their color. */
    virtual std::unordered_map<int, juce::uint32> buildInstrumentToColor() const noexcept = 0;

    /**
     * @return the name of an instrument.
     * @param instrumentId the ID of the instrument. If invalid, empty is returned.
     */
    virtual juce::String getInstrumentName(const Id& instrumentId) const noexcept = 0;

    /**
     * @return the name of an instrument from its index.
     * @param instrumentIndex the index of the instrument. If invalid, empty is returned.
     */
    virtual juce::String getInstrumentNameFromIndex(int instrumentIndex) const noexcept = 0;

    /**
     * Renames an Instrument. Notifies the listeners.
     * @param instrumentId the ID of the instrument. Must be valid.
     * @param newName the new name.
     */
    virtual void setInstrumentName(const Id& instrumentId, const juce::String& newName) noexcept = 0;

    /**
     * Changes the color of an Instrument. Notifies the listeners.
     * @param instrumentId the ID of the instrument. Must be valid.
     * @param newColor the new color.
     */
    virtual void setInstrumentColor(const Id& instrumentId, juce::Colour newColor) noexcept = 0;

    /**
     * Performs an action on a given const Instrument. All the Instruments are locked during the process.
     * @param instrumentId the ID of the instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    virtual void performOnConstInstrument(const Id& instrumentId, const std::function<void(const Instrument&)>& function) const noexcept = 0;

    /**
     * Performs an action on a given non-const Instrument. All the Instruments are locked during the process.
     * @param instrumentId the ID of the instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    virtual void performOnInstrument(const Id& instrumentId, const std::function<void(Instrument&)>& function) const noexcept = 0;

    /**
     * Allows to perform an operation on the Instrument list. A lock is used during the whole operation.
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    virtual void performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) noexcept = 0;

    /**
     * @return true if the instrument is read-only (i.e. it is the first one). If not found, returns false.
     * @param instrumentId the ID of the Instrument. May not exist.
     */
    virtual bool isReadOnlyInstrument(const Id& instrumentId) const noexcept = 0;

    /** @return the observer to the change in the Instruments. */
    virtual Observers<InstrumentChangeObserver>& getInstrumentObservers() noexcept = 0;

    /**
     * Deletes Instruments. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param indexesToDelete the indexes (should be >0, as 0 cannot be selected). Must be valid.
     */
    virtual void deleteInstruments(const std::set<int>& indexesToDelete) noexcept = 0;

    /**
     * Deletes an Instrument. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param id the ID. If not found, do nothing but asserts.
     * @return true if found.
     */
    virtual bool deleteInstrument(const Id& id) noexcept = 0;

    /**
     * Moves Instruments. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param indexesToDelete the indexes to delete. Must never be 0! Because we won't insert before the 0th Instrument.
     * @param destinationIndex the destination index, without taking in account the moved items. Must never be 0!
     */
    virtual void moveInstruments(const std::set<int>& indexesToDelete, int destinationIndex) noexcept = 0;

    /**
     * Inserts the given Instruments. This also reorders them and changes the Song accordingly! Notifies the listeners.
     * @param indexWhereToInsert the index after which to insert. Must never be 0! Because we won't insert before the 0th Instrument. -1 to insert at the end.
     * @param instruments the Instruments to insert.
     */
    virtual void insertInstruments(int indexWhereToInsert, std::vector<std::unique_ptr<Instrument>> instruments) noexcept = 0;

    /**
     * Adds an Instrument at the end of the Instruments. Notifies the listeners.
     * @param instrument the Instrument.
     */
    virtual void addInstrument(std::unique_ptr<Instrument> instrument) noexcept = 0;

    /**
     * Sets the Cell of a PSG Instrument. If out of bounds, the given cell is spread. If <0, nothing happens, but assert.
     * @param instrumentId the ID of the instrument. It must exist.
     * @param cellIndex the cell index (>=0).
     * @param cell the new Cell. Will replace the previous one.
     */
    virtual void setPsgInstrumentCell(const Id& instrumentId, int cellIndex, const PsgInstrumentCell& cell) noexcept = 0;

    /**
     * Duplicates an Instrument Cell. If out of bounds, or <0, nothing happens (asserts in the later case).
     * @param instrumentId the ID of the instrument. It must exist.
     * @param cellIndex the cell index (>=0).
     * @return true if the action was performed.
     */
    virtual bool duplicateInstrumentCell(const Id& instrumentId, int cellIndex) noexcept = 0;

    /**
     * Deletes an Instrument Cell. If out of bounds, the given cell is spread. Cannot remove the last item.
     * @param instrumentId the ID of the instrument. It must exist.
     * @param cellIndex the cell index (>=0).
     * @return true if the action was performed.
     */
    virtual bool deleteInstrumentCell(const Id& instrumentId, int cellIndex) noexcept = 0;

    /**
     * Toggles the retrig an Instrument Cell. If out of bounds, nothing happens.
     * @param instrumentId the ID of the instrument. It must exist.
     * @param cellIndex the cell index (>=0).
     */
    virtual void toggleRetrig(const Id& instrumentId, int cellIndex) noexcept = 0;

    /**
     * Generates an increasing volume curve.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index where to start. May be invalid (out of bounds).
     */
    virtual void generateIncreasingVolume(const Id& instrumentId, int cellIndex) noexcept = 0;

    /**
     * Generates a decreasing volume curve.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index where to start. May be invalid (out of bounds).
     */
    virtual void generateDecreasingVolume(const Id& instrumentId, int cellIndex) noexcept = 0;

    /**
     * Generic method to modify cells of an Instrument (such as generating/modifying cells). Out of bounds cells will be added.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index where to start. May be invalid (out of bounds).
     * @param cellCountToModify how many cells to modify.
     * @param actionName the name of the action, to be shown in the UndoManager.
     * @param actionOnCell the action to perform on the cell. If out of bounds, a new one is generated.
     */
    virtual void applyOnCells(const Id& instrumentId, int cellIndex, int cellCountToModify, const juce::String& actionName,
        std::function<PsgInstrumentCell(int iterationIndex, const PsgInstrumentCell& cell)> actionOnCell) noexcept = 0;

    /**
     * Sets the metadata (loop, retrig, etc.) to a PSG Instrument.
     * @param instrumentId the ID of the instrument. It must exist.
     * @param newLoopStart the possible loop start, if any. Must be valid.
     * @param newEnd the possible end, if any. Must be valid.
     * @param newIsLoop the new loop state, if any.
     * @param newIsRetrig the new retrig state, if any.
     * @param newSpeed the new speed, if any. Must be valid.
     * @param modifiedSectionToAutoSpreadLoop the possible modified auto-spread sections.
     */
    virtual void setPsgInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd,
            OptionalBool newIsLoop, OptionalBool newIsRetrig, OptionalInt newSpeed,
            std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) = 0;

    /**
     * Modifies a sample metadata (loop, amplification).
     * @param instrumentId the instrument ID. It must exist and be a Sample instrument.
     * @param newLoopStart if present, the new loop start. Will be corrected.
     * @param newEnd if present, the new end. Will be corrected.
     * @param newIsLoop if present, the new loop state.
     * @param newAmplification if present, the new amplification. Will be corrected.
     * @param newDigiNote if present, the new diginote. Will be corrected.
     */
    virtual void setSampleInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop,
                                             OptionalFloat newAmplification, OptionalInt newDigiNote) = 0;

    /**
     * Modifies the sample itself of a Sample Instrument, useful to replace a sample with newly loaded one.
     * @param instrumentId the instrument ID. It must exist and be a Sample instrument.
     * @param sample the new sample.
     * @param sampleFrequencyHz the sample frequency, in Hz.
     * @param originalFileName the name of the original file name ("Drum.wav" for example).
     */
    virtual void setSample(const Id& instrumentId, std::unique_ptr<Sample> sample, int sampleFrequencyHz, const juce::String& originalFileName) = 0;

    // Subsongs.
    // ===================================================

    /**
     * Creates a new Subsong. It stops the player and goes to the created Subsong.
     * @param psgs the PSGs.
     * @param metadata the metadata.
     */
    virtual void createNewSubsong(const std::vector<Psg>& psgs, const Properties& metadata) noexcept = 0;

    /**
     * Changed the metadata of a Subsong (PSGs are not included).
     * @param subsongId the subsong ID. Must be valid.
     * @param metadata the metadata.
     */
    virtual void changeSubsongMetadata(const Id& subsongId, const Properties& metadata) noexcept = 0;

    /**
     * Changed the PSGs and metadata of a Subsong. This may change how many PSGs there are, and Tracks may be deleted!
     * @param subsongId the subsong ID. Must be valid.
     * @param psgs the PSGs. Must be at least one.
     * @param metadata the metadata.
     */
    virtual void changeSubsongPsgAndMetadata(const Id& subsongId, const std::vector<Psg>& psgs, const Properties& metadata) noexcept = 0;

    /**
     * Deletes an existing Subsong. It goes to the previous Subsong.
     * @param subsongId the id of the Subsong to delete. Should be valid, but nothing happens if invalid.
     */
    virtual void deleteSubsong(const Id& subsongId) noexcept = 0;

    /** @return how many Subsongs there are. Must always be >=1. */
    virtual int getSubsongCount() const noexcept = 0;


    // Sound effects.
    // ===================================================

    /**
     * Stores the exported sound effects. Nothing is performed if there is no change.
     * @param exportedInstrumentIndexes the indexes of the exported Instruments.
     */
    virtual void setExportedSoundEffects(const std::unordered_set<int>& exportedInstrumentIndexes) = 0;


    // Misc.
    // ===================================================

    /**
     * Stores the currently shown line. This is a hack to allow moving from one position to another while keeping the same
     * line. See "design flaw" in PatternViewerControllerImpl.cpp.
     * This only keep a data, does not notify.
     * @param shownLine the shown line.
     */
    virtual void setCurrentlyShownLine(int shownLine) noexcept = 0;

    /**
     * @return the currently shown line. This is a hack to allow moving from one position to another while keeping the same
     * line. See "design flaw" in PatternViewerControllerImpl.cpp.
     */
    virtual int getCurrentlyShownLine() const noexcept = 0;
};

}   // namespace arkostracker

