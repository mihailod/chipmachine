#include "SongControllerImpl.h"

#include <unordered_map>

#include "../business/CompoundAction.h"
#include "../business/actions/expressions/AddExpressions.h"
#include "../business/actions/expressions/DeleteExpressionCell.h"
#include "../business/actions/expressions/DeleteExpressions.h"
#include "../business/actions/expressions/DuplicateExpressionCell.h"
#include "../business/actions/expressions/ModifyExpressionCells.h"
#include "../business/actions/expressions/MoveExpressions.h"
#include "../business/actions/expressions/RenameExpression.h"
#include "../business/actions/expressions/SetExpressionCell.h"
#include "../business/actions/expressions/SetExpressionCellMetadata.h"
#include "../business/actions/instruments/AddInstruments.h"
#include "../business/actions/instruments/DeleteInstruments.h"
#include "../business/actions/instruments/MoveInstruments.h"
#include "../business/actions/instruments/RenameInstrument.h"
#include "../business/actions/instruments/SetInstrumentColor.h"
#include "../business/actions/instruments/psg/DeletePsgInstrumentCell.h"
#include "../business/actions/instruments/psg/DuplicatePsgInstrumentCell.h"
#include "../business/actions/instruments/psg/ModifyPsgInstrumentCells.h"
#include "../business/actions/instruments/psg/SetPsgInstrumentCell.h"
#include "../business/actions/instruments/psg/SetPsgInstrumentMetadata.h"
#include "../business/actions/instruments/psg/action/GenerateVolumeCellsAction.h"
#include "../business/actions/instruments/psg/action/ToggleRetrigAction.h"
#include "../business/actions/instruments/sample/ChangeSample.h"
#include "../business/actions/instruments/sample/ModifySampleInstrumentMetadata.h"
#include "../business/actions/linker/ClearPatterns.h"
#include "../business/actions/linker/ClonePositions.h"
#include "../business/actions/linker/CreatePattern.h"
#include "../business/actions/linker/DeletePositions.h"
#include "../business/actions/linker/DuplicatePositions.h"
#include "../business/actions/linker/IncreasePatternInPosition.h"
#include "../business/actions/linker/InsertPositions.h"
#include "../business/actions/linker/ModifyPattern.h"
#include "../business/actions/linker/ModifyPosition.h"
#include "../business/actions/linker/ModifyPositionMarker.h"
#include "../business/actions/linker/ModifyPositionsData.h"
#include "../business/actions/linker/MovePositionMarker.h"
#include "../business/actions/linker/MovePositions.h"
#include "../business/actions/linker/RearrangePatterns.h"
#include "../business/actions/linker/SetLoopStartAndEnd.h"
#include "../business/actions/patternViewer/ClearCells.h"
#include "../business/actions/patternViewer/LinkSpecialTrack.h"
#include "../business/actions/patternViewer/LinkTrack.h"
#include "../business/actions/patternViewer/PasteCells.h"
#include "../business/actions/patternViewer/SetCellData.h"
#include "../business/actions/patternViewer/SetPositionHeight.h"
#include "../business/actions/patternViewer/SetSpecialCellData.h"
#include "../business/actions/patternViewer/SetSpecialTrackName.h"
#include "../business/actions/patternViewer/SetTrackColor.h"
#include "../business/actions/patternViewer/SetTrackName.h"
#include "../business/actions/patternViewer/ToggleReadOnly.h"
#include "../business/actions/patternViewer/Transpose.h"
#include "../business/actions/patternViewer/UnlinkSpecialTrack.h"
#include "../business/actions/patternViewer/UnlinkTrack.h"
#include "../business/actions/patternViewer/insertOrRemoveCellAt/InsertCellAt.h"
#include "../business/actions/patternViewer/insertOrRemoveCellAt/RemoveCellAt.h"
#include "../business/actions/sfx/SetExportedSoundEffects.h"
#include "../business/actions/song/SetSongMetadata.h"
#include "../business/actions/subsong/CreateSubsong.h"
#include "../business/actions/subsong/DeleteSubsong.h"
#include "../business/actions/subsong/SetSubsongMetadata.h"
#include "../business/actions/subsong/SetSubsongPsgs.h"
#include "../controllers/MainController.h"
#include "../controllers/PlayerController.h"
#include "../utils/NumberUtil.h"
#include "observers/LinkerObserver.h"
#include "observers/SongMetadataObserver.h"

namespace arkostracker 
{

SongControllerImpl::SongControllerImpl(MainController& pMainController, const std::shared_ptr<Song>& pSong) noexcept :
        mainController(pMainController),
        currentlyShownLocation(pSong->getFirstSubsongId(), 0, 0),
        songMetadataObservers(),
        subsongMetadataObservers(),
        arpeggioChangeListeners(),
        pitchChangeListeners(),
        linkerObservers(),
        instrumentChangeObservers(),
        trackChangeObservers(),
        song(pSong),
        undoManager(5000),
        currentlyShownLine(0)
{
}

MainController& SongControllerImpl::getMainController()
{
    return mainController;
}

Location SongControllerImpl::getCurrentViewedLocation() const noexcept
{
    return currentlyShownLocation;
}

void SongControllerImpl::storeCurrentLocation(const Location& newLocation) noexcept
{
    currentlyShownLocation = newLocation;
}

void SongControllerImpl::setCurrentLocation(const Location& originalNewLocation, const bool alsoChangeLine) noexcept
{
    // If it was playing a pattern, continues to do so.
    auto& playerController = mainController.getPlayerController();
    const auto isPlayingPattern = playerController.isPlayingOnPattern();

    // Needs to correct the position?
    const auto newLocationSubsong = originalNewLocation.getSubsongId();
    auto lastPosition = 0;
    auto newLocationPosition = 0;
    auto newLocationLine = 0;
    song->performOnConstSubsong(newLocationSubsong, [&] (const Subsong& subsong) noexcept {
        lastPosition = subsong.getLength() - 1;
        newLocationPosition = NumberUtil::correctNumber(originalNewLocation.getPosition(), 0, lastPosition);

        // Corrects the line number for the new position.
        const auto positionHeight = subsong.getPositionHeight(newLocationPosition);
        // When changing position, keeps the line we were previously on, unless specifically wanted.
        newLocationLine = std::min(alsoChangeLine ? originalNewLocation.getLine() : currentlyShownLocation.getLine(),
            positionHeight - 1);
    });

    // Builds the new corrected location.
    const Location newLocation(newLocationSubsong, newLocationPosition, newLocationLine);
    if (currentlyShownLocation == newLocation) {
        return;
    }
    const auto isNewSubsong = (currentlyShownLocation.getSubsongId() != newLocation.getSubsongId());
    currentlyShownLocation = newLocation;

    getSongMetadataObservers().applyOnObservers([&](SongMetadataObserver* observer) noexcept {
        observer->onSongMetadataChanged(SongMetadataObserver::What::shownLocation);
    });

    // If playing, we try to get there. If not playing, go there but only marks it as a future location.
    if (playerController.isPlaying()) {
        if (isPlayingPattern) {
            playerController.playPatternFromShownLocationOrBlock();
        } else {
            playerController.playSongFromLocation(currentlyShownLocation);
        }
    } else {
        playerController.setCurrentlyPlayedLocation(currentlyShownLocation.withLine());
    }

    // When changing Subsong, asks for an unmute. This is not logical to have the same muted channels from a Subsong to another.
    if (isNewSubsong) {
        mainController.unmuteAll();
    }
}

Id SongControllerImpl::getCurrentSubsongId() const noexcept
{
    return currentlyShownLocation.getSubsongId();
}

std::shared_ptr<Song> SongControllerImpl::getSong()
{
    return song;
}

std::shared_ptr<const Song> SongControllerImpl::getConstSong() const
{
    return song;
}

void SongControllerImpl::setSong(const std::shared_ptr<Song> newSong)
{
    song = newSong;
    currentlyShownLocation = Location(song->getFirstSubsongId(), 0);
}

Id SongControllerImpl::determineSubsongId(const OptionalValue<Id>& subsongId) const noexcept
{
    return subsongId.isAbsent() ? currentlyShownLocation.getSubsongId() : subsongId.getValue();
}

Observers<SongMetadataObserver>& SongControllerImpl::getSongMetadataObservers() noexcept
{
    return songMetadataObservers;
}

Observers<SubsongMetadataObserver>& SongControllerImpl::getSubsongMetadataObservers() noexcept
{
    return subsongMetadataObservers;
}

Observers<ExpressionChangeObserver>& SongControllerImpl::getExpressionObservers(const bool isArpeggio) noexcept
{
    if (isArpeggio) {
        return arpeggioChangeListeners;
    }

    return pitchChangeListeners;
}

Observers<TrackChangeObserver>& SongControllerImpl::getTrackObservers() noexcept
{
    return trackChangeObservers;
}

juce::String SongControllerImpl::getTitle() const noexcept
{
    return song->getTitle();
}

juce::String SongControllerImpl::getAuthor() const noexcept
{
    return song->getAuthor();
}

juce::String SongControllerImpl::getComposer() const noexcept
{
    return song->getComposer();
}

juce::String SongControllerImpl::getComments() const noexcept
{
    return song->getComments();
}

void SongControllerImpl::setSongMetadata(OptionalValue<juce::String> newTitle, OptionalValue<juce::String> newAuthor, OptionalValue<juce::String> newComposer,
                                         OptionalValue<juce::String> newComments) noexcept
{
    auto action = std::make_unique<SetSongMetadata>(*this, newTitle, newAuthor, newComposer, newComments);
    performAction(std::move(action), juce::translate("Modify song metadata"));
}

bool SongControllerImpl::canUndo() const
{
    return undoManager.canUndo();
}

bool SongControllerImpl::canRedo() const
{
    return undoManager.canRedo();
}

void SongControllerImpl::undo()
{
    undoManager.undo();
    notifyAboutModifiedState();
}

void SongControllerImpl::redo()
{
    undoManager.redo();
    notifyAboutModifiedState();
}

void SongControllerImpl::markSongAsSaved() noexcept
{
    undoManager.setCurrentStateAsNonModified();
    notifyAboutModifiedState();
}

bool SongControllerImpl::isSongModified() const noexcept
{
    return undoManager.isCurrentStateModified();
}

void SongControllerImpl::notifyAboutModifiedState() noexcept
{
    getSongMetadataObservers().applyOnObservers([](SongMetadataObserver* observer) {
        observer->onSongMetadataChanged(SongMetadataObserver::What::modified);
    });
}

juce::String SongControllerImpl::getUndoDescription()
{
    return undoManager.getUndoDescription();
}

juce::String SongControllerImpl::getRedoDescription()
{
    return undoManager.getRedoDescription();
}

void SongControllerImpl::clearUndo()
{
    undoManager.clearUndoHistory();
}

bool SongControllerImpl::performAction(std::unique_ptr<juce::UndoableAction> action, const juce::String& name) noexcept
{
    undoManager.beginNewTransaction(name);

    const auto performed = undoManager.perform(action.release());
    if (performed) {
        notifyAboutModifiedState();
    }

    return performed;
}


// Expressions.
// ====================================================

int SongControllerImpl::getExpressionCount(const bool isArpeggio) const noexcept
{
    return song->getExpressionHandler(isArpeggio).getCount();
}

bool SongControllerImpl::isReadOnlyExpression(const bool isArpeggio, const Id& expressionId) const noexcept
{
    return song->getExpressionHandler(isArpeggio).isReadOnly(expressionId);
}

std::vector<juce::String> SongControllerImpl::getExpressionNames(const bool isArpeggio) const noexcept
{
    return song->getExpressionHandler(isArpeggio).getNames();
}

void SongControllerImpl::setExpressionName(bool isArpeggio, const Id& expressionId, const juce::String& newName) noexcept
{
    auto action = std::make_unique<RenameExpression>(*this, isArpeggio, expressionId, newName);
    performAction(std::move(action), juce::translate("Rename ") + getExpressionTypeName(isArpeggio));
}

juce::String SongControllerImpl::getExpressionName(const bool isArpeggio, const Id& expressionId) const noexcept
{
    return song->getExpressionHandler(isArpeggio).getName(expressionId);
}

juce::String SongControllerImpl::getExpressionNameFromIndex(const bool isArpeggio, const int expressionIndex) const noexcept
{
    return song->getExpressionHandler(isArpeggio).getNameFromIndex(expressionIndex);
}

void SongControllerImpl::deleteExpressions(bool isArpeggio, std::set<int> indexesToDelete) noexcept
{
    auto action = std::make_unique<DeleteExpressions>(*this, isArpeggio, indexesToDelete);
    performAction(std::move(action), juce::translate("Delete ") + getExpressionTypeName(isArpeggio));
}

void SongControllerImpl::insertExpressions(bool isArpeggio, int expressionIndexWhereToInsert, std::vector<std::unique_ptr<Expression>> expressions) noexcept
{
    jassert(expressionIndexWhereToInsert != 0);         // Forbidden, we don't insert at 0.

    const auto expressionCount = mainController.getSongController().getExpressionCount(isArpeggio);
    // If -1, selects the last one.
    if (expressionIndexWhereToInsert == -1) {
        expressionIndexWhereToInsert = expressionCount;
    }

    auto currentlySelectedIndex = (expressionCount > 1) ? 1 : 0;    // Fallback value.
    // Gets the current selection index, this is only for the Undo to select it back.
    const auto expressionIdOptional = mainController.getSelectedExpressionId(isArpeggio);
    if (expressionIdOptional.isPresent()) {
        const auto expressionIndexOptional = mainController.getExpressionIndex(isArpeggio, expressionIdOptional.getValueRef());
        if (expressionIndexOptional.isPresent()) {
            currentlySelectedIndex = expressionIndexOptional.getValue();
        } else {
            jassertfalse;       // Does not exist?
        }
    }

    auto action = std::make_unique<AddExpressions>(*this, isArpeggio, expressionIndexWhereToInsert, currentlySelectedIndex, std::move(expressions));
    performAction(std::move(action), translate("Add " + getExpressionTypeName(isArpeggio)));
}

juce::String SongControllerImpl::getExpressionTypeName(const bool isArpeggio) noexcept
{
    return isArpeggio ? "arpeggio" : "pitch";
}

void SongControllerImpl::moveExpressions(bool isArpeggio, const std::set<int>& indexesToDelete, int destinationIndex) noexcept
{
    jassert(destinationIndex > 0);                                  // Moving 0 is forbidden!
    jassert(indexesToDelete.find(0) == indexesToDelete.cend());     // Cannot move 0!!

    auto action = std::make_unique<MoveExpressions>(*this, isArpeggio, indexesToDelete, destinationIndex);
    performAction(std::move(action), translate("Move " + getExpressionTypeName(isArpeggio)));
}

Expression SongControllerImpl::getExpression(const bool isArpeggio, const Id& expressionId) noexcept
{
    return song->getExpressionHandler(isArpeggio).getExpressionCopy(expressionId);
}

ExpressionHandler& SongControllerImpl::getExpressionHandler(const bool isArpeggio) noexcept
{
    return song->getExpressionHandler(isArpeggio);
}

void SongControllerImpl::setExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex, int value) noexcept
{
    auto action = std::make_unique<SetExpressionCell>(*this, isArpeggio, expressionId, cellIndex, value);
    performAction(std::move(action), translate("Modify " + getExpressionTypeName(isArpeggio)));
}

void SongControllerImpl::applyOnExpressionCells(const Id& expressionId, bool isArpeggio, int cellIndex, int cellCountToModify, const juce::String& actionName,
        std::function<int(int iterationIndex, int initialValue)> actionOnCell) noexcept
{
    jassert(cellIndex >= 0);

    auto action = std::make_unique<ModifyExpressionCells>(*this, isArpeggio, expressionId, cellIndex, cellCountToModify, true, actionOnCell);
    performAction(std::move(action), actionName);
}

bool SongControllerImpl::isExpressionArpeggio(const Id& expressionId) const noexcept
{
    const auto itemId = song->getExpressionHandler(true).find(expressionId);
    return itemId.isPresent();
}

void SongControllerImpl::setExpressionMetadata(bool isArpeggio, const Id& expressionId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalInt newSpeed,
                                               OptionalInt newShift)
{
    auto action = std::make_unique<SetExpressionCellMetadata>(*this, isArpeggio, expressionId, newLoopStart, newEnd, newSpeed, newShift);
    performAction(std::move(action), translate("Modify " + getExpressionTypeName(isArpeggio) + " metadata"));
}

bool SongControllerImpl::duplicateExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept
{
    auto action = std::make_unique<DuplicateExpressionCell>(*this, isArpeggio, expressionId, cellIndex);
    return performAction(std::move(action), translate("Duplicate " + getExpressionTypeName(isArpeggio) + " cell"));
}

bool SongControllerImpl::deleteExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept
{
    auto action = std::make_unique<DeleteExpressionCell>(*this, isArpeggio, expressionId, cellIndex);
    return performAction(std::move(action), translate("Delete " + getExpressionTypeName(isArpeggio) + " cell"));
}

OptionalInt SongControllerImpl::getExpressionIndex(const bool isArpeggio, const Id& expressionId) const noexcept
{
    return song->getExpressionHandler(isArpeggio).find(expressionId);
}

OptionalId SongControllerImpl::getExpressionId(const bool isArpeggio, const int expressionIndex) const noexcept
{
    return song->getExpressionHandler(isArpeggio).find(expressionIndex);
}

Id SongControllerImpl::getLastExpressionId(const bool isArpeggio) const noexcept
{
    return song->getExpressionHandler(isArpeggio).getLastId();
}


// Linker.
// =============================================================

Observers<LinkerObserver>& SongControllerImpl::getLinkerObservers() noexcept
{
    return linkerObservers;
}

void SongControllerImpl::setPositionMarker(const Id& subsongId, int positionIndex, const juce::String& newName, juce::Colour newColor) noexcept
{
    auto action = std::make_unique<ModifyPositionMarker>(*this, subsongId, positionIndex, newName, newColor);
    performAction(std::move(action), juce::translate("Modify marker"));
}

void SongControllerImpl::modifyPosition(const Id& subsongId, int positionIndex, const Position& position) noexcept
{
    auto action = std::make_unique<ModifyPosition>(*this, subsongId, positionIndex, position);
    performAction(std::move(action), juce::translate("Modify position"));
}

void SongControllerImpl::modifyPattern(const Id& subsongId, int patternIndex, const Pattern& pattern) noexcept
{
    auto action = std::make_unique<ModifyPattern>(*this, subsongId, patternIndex, pattern);
    performAction(std::move(action), juce::translate("Modify pattern"));
}

void SongControllerImpl::modifyPositionsData(const Id& subsongId, const std::set<int>& positionIndexes, OptionalInt newHeight,
                                             OptionalValue<juce::uint32> newPatternColorArgb) noexcept
{
    auto action = std::make_unique<ModifyPositionsData>(*this, subsongId, positionIndexes, newHeight, newPatternColorArgb);
    performAction(std::move(action), juce::translate("Modify positions and patterns"));
}

void SongControllerImpl::movePositionMarker(const Id& subsongId, int sourcePosition, int destinationPosition) noexcept
{
    if (sourcePosition == destinationPosition) {
        return;
    }

    auto action = std::make_unique<MovePositionMarker>(*this, subsongId, sourcePosition, destinationPosition);
    performAction(std::move(action), juce::translate("Move marker"));
}

void SongControllerImpl::performOnConstSubsong(const Id& subsongId, const std::function<void(const Subsong&)>& function) const noexcept
{
    song->performOnConstSubsong(subsongId, function);
}

void SongControllerImpl::performOnSubsong(const Id& subsongId, const std::function<void(Subsong&)>& function) const noexcept
{
    song->performOnSubsong(subsongId, function);
}

void SongControllerImpl::duplicatePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int destinationPosition,
                                            bool insertAfter, bool alsoDuplicateMarker) noexcept
{
    auto action = std::make_unique<DuplicatePositions>(*this, subsongId, positionsToDuplicate, destinationPosition, insertAfter, alsoDuplicateMarker);
    performAction(std::move(action), juce::translate("Duplicate positions"));
}

void SongControllerImpl::createNewPositionAndPattern(const Id& subsongId, int positionAfterWhichToInsert) noexcept
{
    auto action = std::make_unique<CreatePattern>(*this, subsongId, positionAfterWhichToInsert);
    performAction(std::move(action), juce::translate("Create new pattern"));
}

void SongControllerImpl::clonePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int destinationPosition, bool insertAfter) noexcept
{
    auto action = std::make_unique<ClonePositions>(*this, subsongId, positionsToDuplicate, destinationPosition, insertAfter);
    performAction(std::move(action), juce::translate("Clone positions"));
}

void SongControllerImpl::deletePositions(const Id& subsongId, const std::set<int>& positionsToDelete, const bool deleteOrCut) noexcept
{
    auto action = std::make_unique<DeletePositions>(*this, subsongId, positionsToDelete);
    performAction(std::move(action), deleteOrCut
                                     ? juce::translate("Delete positions")
                                     : juce::translate("Cut positions"));
}

void SongControllerImpl::movePositions(const Id& subsongId, const std::set<int>& positions, int destinationPosition, bool insertAfter) noexcept
{
    auto action = std::make_unique<MovePositions>(*this, subsongId, positions, destinationPosition, insertAfter);
    performAction(std::move(action), juce::translate("Move positions"));
}

void SongControllerImpl::insertPositions(const Id& subsongId, const std::vector<Position>& positions, int destinationPosition, bool insertAfter,
                                         bool alsoDuplicateMarker) noexcept
{
    auto action = std::make_unique<InsertPositions>(*this, subsongId, positions, destinationPosition, insertAfter, alsoDuplicateMarker);
    performAction(std::move(action), juce::translate("Insert positions"));
}

void SongControllerImpl::setPositionHeight(const Location& location, int newHeight) noexcept
{
    auto action = std::make_unique<SetPositionHeight>(*this, location, newHeight);
    performAction(std::move(action), juce::translate("Set position height"));
}

void SongControllerImpl::setPositionTransposition(const Location& location, int channelIndex, int newTransposition) noexcept
{
    // Simplifies by copying the current position. This is not an atomic process, but shouldn't be a problem.
    auto position = Position::buildEmptyInstance();
    const auto subsongId = location.getSubsongId();
    const auto positionIndex = location.getPosition();
    song->performOnConstSubsong(subsongId, [&position, &positionIndex, &channelIndex, &newTransposition] (const Subsong& subsong) noexcept {
        position = subsong.getPosition(positionIndex).withTransposition(channelIndex, newTransposition);
    });

    auto action = std::make_unique<ModifyPosition>(*this, subsongId, positionIndex, position);
    performAction(std::move(action), juce::translate("Set channel transposition"));
}

void SongControllerImpl::increasePatternIndex(const Id& subsongId, const int positionIndex, const int increaseStep) noexcept
{
    auto action = std::make_unique<IncreasePatternInPosition>(*this, subsongId, positionIndex, increaseStep);
    performAction(std::move(action), juce::translate("Change pattern index"));
}

void SongControllerImpl::setTrackNameAndColor(const Location& location, int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) noexcept
{
    const auto subsongId = location.getSubsongId();
    const auto positionIndex = location.getPosition();

    // Creates a compound action.
    auto action = std::make_unique<CompoundAction>();
    auto actionRename = std::make_unique<SetTrackName>(*this, subsongId, positionIndex, channelIndex, newName);
    auto actionSetColor = std::make_unique<SetTrackColor>(*this, subsongId, positionIndex, channelIndex, newColor);
    action->addAction(std::move(actionRename));
    action->addAction(std::move(actionSetColor));
    performAction(std::move(action), juce::translate("Set track name and color"));
}

void SongControllerImpl::setSpecialTrackName(const Location& location, const bool isSpeedTrack, const juce::String& newName) noexcept
{
    const auto subsongId = location.getSubsongId();
    const auto positionIndex = location.getPosition();

    auto action = std::make_unique<SetSpecialTrackName>(*this, subsongId, positionIndex, isSpeedTrack, newName);
    performAction(std::move(action), juce::translate("Rename special track"));
}

void SongControllerImpl::createTrackLink(const Id& subsongId, int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex)
{
    auto action = std::make_unique<LinkTrack>(*this, subsongId, sourcePositionIndex, sourceChannelIndex, targetPositionIndex, targetChannelIndex);
    performAction(std::move(action), juce::translate("Link track"));
}

void SongControllerImpl::unlinkTrack(const Id& subsongId, int positionIndexToUnlink, int channelIndexToUnlink) noexcept
{
    auto action = std::make_unique<UnlinkTrack>(*this, subsongId, positionIndexToUnlink, channelIndexToUnlink);
    performAction(std::move(action), juce::translate("Unlink track"));
}

void SongControllerImpl::createSpecialTrackLink(const Id& subsongId, const bool isSpeedTrack, const int sourcePositionIndex, const int targetPositionIndex)
{
    auto action = std::make_unique<LinkSpecialTrack>(*this, subsongId, isSpeedTrack, sourcePositionIndex, targetPositionIndex);
    performAction(std::move(action), juce::translate("Link special track"));
}

void SongControllerImpl::unlinkSpecialTrack(const Id& subsongId, const bool isSpeedTrack, const int positionIndexToUnlink) noexcept
{
    auto action = std::make_unique<UnlinkSpecialTrack>(*this, subsongId, isSpeedTrack, positionIndexToUnlink);
    performAction(std::move(action), juce::translate("Unlink special track"));
}

void SongControllerImpl::clearPatterns(const Id& subsongId) noexcept
{
    auto action = std::make_unique<ClearPatterns>(*this, subsongId);
    performAction(std::move(action), juce::translate("Clear patterns"));
}

void SongControllerImpl::rearrangePatterns(const Id& subsongId, bool deleteUnused) noexcept
{
    auto action = std::make_unique<RearrangePatterns>(*this, subsongId, deleteUnused);
    performAction(std::move(action),
        deleteUnused
            ? juce::translate("Rearrange patterns")
            : juce::translate("Rearrange and delete unused patterns")
        );
}


// Subsong Metadata
// ======================================================================

int SongControllerImpl::getChannelCount(const Id& subsongId) const noexcept
{
    return song->getChannelCount(subsongId);
}

int SongControllerImpl::getChannelCount() const noexcept
{
    const auto subsongIndex = getCurrentSubsongId();
    return getChannelCount(subsongIndex);
}

void SongControllerImpl::setLoopStartAndEnd(const Id& subsongId, int loopStartPosition, int endPosition) noexcept
{
    jassert((loopStartPosition >= 0) && (endPosition >= 0));
    jassert(loopStartPosition <= endPosition);

    auto action = std::make_unique<SetLoopStartAndEnd>(*this, determineSubsongId(subsongId), loopStartPosition, endPosition);
    performAction(std::move(action), juce::translate("Set loop start/end"));
}

std::vector<Psg> SongControllerImpl::getPsgs(const Id& subsongId) const noexcept
{
    std::vector<Psg> psgs;

    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        psgs = subsong.getPsgs();
    });
    return psgs;
}

int SongControllerImpl::getPsgCount(const Id& subsongId) const noexcept
{
    int psgCount;           // NOLINT(*-init-variables)
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        psgCount = subsong.getPsgCount();
    });

    return psgCount;
}


// Instruments
// ======================================================================

std::unordered_map<int, juce::uint32> SongControllerImpl::buildInstrumentToColor() const noexcept
{
    return song->buildInstrumentToColor();
}

int SongControllerImpl::getInstrumentCount() const noexcept
{
    return song->getInstrumentCount();
}

OptionalId SongControllerImpl::getInstrumentId(const int index) const noexcept
{
    return song->getInstrumentId(index);
}

Id SongControllerImpl::getLastInstrumentId() const noexcept
{
    return song->getLastInstrumentId();
}

OptionalInt SongControllerImpl::getInstrumentIndex(const Id& instrumentId) const noexcept
{
    return song->getInstrumentIndex(instrumentId);
}

juce::String SongControllerImpl::getInstrumentName(const Id& instrumentId) const noexcept
{
    return song->getInstrumentName(instrumentId);
}

juce::String SongControllerImpl::getInstrumentNameFromIndex(const int instrumentIndex) const noexcept
{
    return song->getInstrumentNameFromIndex(instrumentIndex);
}

void SongControllerImpl::performOnConstInstrument(const Id& instrumentId, const std::function<void(const Instrument&)>& function) const noexcept
{
    song->performOnConstInstrument(instrumentId, function);
}

void SongControllerImpl::performOnInstrument(const Id& instrumentId, const std::function<void(Instrument&)>& function) const noexcept
{
    song->performOnInstrument(instrumentId, function);
}

Observers<InstrumentChangeObserver>& SongControllerImpl::getInstrumentObservers() noexcept
{
    return instrumentChangeObservers;
}

void SongControllerImpl::setInstrumentName(const Id& instrumentId, const juce::String& newName) noexcept
{
    auto action = std::make_unique<RenameInstrument>(*this, instrumentId, newName);
    performAction(std::move(action), juce::translate("Rename instrument"));
}

void SongControllerImpl::setInstrumentColor(const Id& instrumentId, juce::Colour newColor) noexcept
{
    auto action = std::make_unique<SetInstrumentColor>(*this, instrumentId, newColor);
    performAction(std::move(action), juce::translate("Set instrument color"));
}

void SongControllerImpl::deleteInstruments(const std::set<int>& instrumentIndexesToDelete) noexcept
{
    auto action = std::make_unique<DeleteInstruments>(*this, instrumentIndexesToDelete);
    performAction(std::move(action), juce::translate("Delete instruments"));
}

bool SongControllerImpl::deleteInstrument(const Id& id) noexcept
{
    const auto index = getInstrumentIndex(id);
    if (index.isAbsent()) {
        jassertfalse;
        return false;
    }

    // Ideally, it should be the Delete Instruments that should only have IDs, but this is old code... Oh, well.
    deleteInstruments({ index.getValue() });

    return true;
}

void SongControllerImpl::performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) noexcept
{
    song->performOnInstruments(lockedOperation);
}

bool SongControllerImpl::isReadOnlyInstrument(const Id& instrumentId) const noexcept
{
    return song->isReadOnlyInstrument(instrumentId);
}

void SongControllerImpl::moveInstruments(const std::set<int>& indexesToDelete, int destinationIndex) noexcept
{
    jassert(destinationIndex > 0);                                  // Moving 0 is forbidden!
    jassert(indexesToDelete.find(0) == indexesToDelete.cend());     // Cannot move 0!!

    auto action = std::make_unique<MoveInstruments>(*this, indexesToDelete, destinationIndex);
    performAction(std::move(action), juce::translate("Move instruments"));
}

void SongControllerImpl::insertInstruments(int indexWhereToInsert, std::vector<std::unique_ptr<Instrument>> instruments) noexcept
{
    // If -1, adds at the end.
    const auto instrumentCount = mainController.getSongController().getInstrumentCount();
    if (indexWhereToInsert < 0) {
        indexWhereToInsert = instrumentCount;
    }

    jassert(indexWhereToInsert > 0);                                // Inserting at 0 is forbidden!

    // Gets the current selection index, this is only for the Undo to select it back.
    auto currentlySelectedIndex = (instrumentCount > 1) ? 1 : 0;    // Fallback value.
    const auto instrumentIdOptional = mainController.getSelectedInstrumentId();
    if (instrumentIdOptional.isPresent()) {
        const auto instrumentIndexOptional = mainController.getInstrumentIndex(instrumentIdOptional.getValueRef());
        if (instrumentIndexOptional.isPresent()) {
            currentlySelectedIndex = instrumentIndexOptional.getValue();
        } else {
            jassertfalse;       // Does not exist?
        }
    }

    auto action = std::make_unique<AddInstruments>(*this, indexWhereToInsert, currentlySelectedIndex, std::move(instruments));
    performAction(std::move(action), juce::translate("Add instruments"));
}

void SongControllerImpl::addInstrument(std::unique_ptr<Instrument> instrument) noexcept
{
    const auto& songController = mainController.getSongController();
    const auto instrumentIndex = songController.getInstrumentCount();

    std::vector<std::unique_ptr<Instrument>> instrumentsToAdd;
    instrumentsToAdd.push_back(std::move(instrument));

    insertInstruments(instrumentIndex, std::move(instrumentsToAdd));
}

void SongControllerImpl::setPsgInstrumentCell(const Id& instrumentId, int cellIndex, const PsgInstrumentCell& cell) noexcept
{
    jassert(cellIndex >= 0);

    auto action = std::make_unique<SetPsgInstrumentCell>(*this, instrumentId, cellIndex, cell);
    performAction(std::move(action), juce::translate("Modify PSG instrument cell"));
}

bool SongControllerImpl::duplicateInstrumentCell(const Id& instrumentId, int cellIndex) noexcept
{
    jassert(cellIndex >= 0);
    auto action = std::make_unique<DuplicatePsgInstrumentCell>(*this, instrumentId, cellIndex);
    return performAction(std::move(action), juce::translate("Duplicate PSG instrument cell"));
}

bool SongControllerImpl::deleteInstrumentCell(const Id& instrumentId, int cellIndex) noexcept
{
    jassert(cellIndex >= 0);
    auto action = std::make_unique<DeletePsgInstrumentCell>(*this, instrumentId, cellIndex);
    return performAction(std::move(action), juce::translate("Delete PSG instrument cell"));
}

void SongControllerImpl::toggleRetrig(const Id& instrumentId, int cellIndex) noexcept
{
    jassert(cellIndex >= 0);
    auto action = std::make_unique<ModifyPsgInstrumentCells>(*this, instrumentId, cellIndex, 1, false, ToggleRetrigAction());
    performAction(std::move(action), juce::translate("Toggle retrig"));
}

void SongControllerImpl::generateIncreasingVolume(const Id& instrumentId, int cellIndex) noexcept
{
    jassert(cellIndex >= 0);

    const GenerateVolumeCellsAction generateVolumeCellsAction(0, 1.0);
    auto action = std::make_unique<ModifyPsgInstrumentCells>(*this, instrumentId, cellIndex, 16, true,
                                                             generateVolumeCellsAction);
    performAction(std::move(action), juce::translate("Generate increasing volume"));
}

void SongControllerImpl::generateDecreasingVolume(const Id &instrumentId, int cellIndex) noexcept
{
    jassert(cellIndex >= 0);

    const GenerateVolumeCellsAction generateVolumeCellsAction(15, -1.0);
    auto action = std::make_unique<ModifyPsgInstrumentCells>(*this, instrumentId, cellIndex, 15, true, generateVolumeCellsAction);
    performAction(std::move(action), juce::translate("Generate decreasing volume"));
}

void SongControllerImpl::applyOnCells(const Id& instrumentId, int cellIndex, int cellCountToModify, const juce::String& actionName,
                                      std::function<PsgInstrumentCell(int iterationIndex, const PsgInstrumentCell& cell)> actionOnCell) noexcept
{
    jassert(cellIndex >= 0);

    auto action = std::make_unique<ModifyPsgInstrumentCells>(*this, instrumentId, cellIndex, cellCountToModify, true, actionOnCell);
    performAction(std::move(action), actionName);
}

void SongControllerImpl::setPsgInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop, OptionalBool newIsRetrig,
                                                  OptionalInt newSpeed, std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop)
{
    auto action = std::make_unique<SetPsgInstrumentMetadata>(*this, instrumentId, newLoopStart, newEnd, newIsLoop, newIsRetrig, newSpeed,
            modifiedSectionToAutoSpreadLoop);
    performAction(std::move(action), juce::translate("Modify PSG instrument"));
}

void SongControllerImpl::setSampleInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop,
                                                     OptionalFloat newAmplification, OptionalInt newDigiNote)
{
    auto action = std::make_unique<ModifySampleInstrumentMetadata>(*this, instrumentId, newLoopStart, newEnd, newIsLoop, newAmplification, newDigiNote);
    performAction(std::move(action), juce::translate("Modify sample instrument"));
}

void SongControllerImpl::setSample(const Id& instrumentId, std::unique_ptr<Sample> sample, int sampleFrequencyHz, const juce::String& originalFileName)
{
    auto action = std::make_unique<ChangeSample>(*this, instrumentId, std::move(sample), sampleFrequencyHz, originalFileName);
    performAction(std::move(action), juce::translate("Change sample"));
}

void SongControllerImpl::setCell(const Location& location, int channelIndex, OptionalValue<Note> newNote, OptionalInt newInstrument,
                                 OptionalValue<EffectDigit> newEffectDigit, OptionalValue<EffectNumber> newEffectNumber,
                                 OptionalValue<Digit> newInstrumentDigit) noexcept
{
    jassert(newNote.isPresent() || newInstrument.isPresent() || newEffectDigit.isPresent()
        || newEffectNumber.isPresent() || newInstrumentDigit.isPresent());      // No data? Strange.

    auto action = std::make_unique<SetCellData>(*this, location, channelIndex, newNote, newInstrument, newEffectDigit, newEffectNumber, newInstrumentDigit);
    performAction(std::move(action), juce::translate("Modify track"));
}

void SongControllerImpl::setSpecialCell(bool isSpeedTrack, const Location& location, Digit digit) noexcept
{
    auto action = std::make_unique<SetSpecialCellData>(*this, location, isSpeedTrack, digit);
    performAction(std::move(action), juce::translate("Modify special track"));
}

void SongControllerImpl::pasteCells(const Location& location, const CursorLocation& cursorLocation, const PasteData& pasteData,
    const bool pasteMix, const bool pasteContinuously) noexcept
{
    auto action = std::make_unique<PasteCells>(*this, location, cursorLocation, pasteData, pasteMix, pasteContinuously);
    performAction(std::move(action), juce::translate("Paste cells"));
}

void SongControllerImpl::transpose(TransposeRate transposeRate, const SelectedData& selectedData) noexcept
{
    auto action = std::make_unique<Transpose>(*this, transposeRate, selectedData);
    performAction(std::move(action), juce::translate("Transpose"));
}

void SongControllerImpl::toggleTracksReadOnly(Id subsongId, const std::set<int>& trackIndexes, OptionalInt speedTrackIndex, OptionalInt eventTrackIndex) noexcept
{
    auto action = std::make_unique<ToggleReadOnly>(*this, std::move(subsongId), trackIndexes, speedTrackIndex, eventTrackIndex);
    performAction(std::move(action), juce::translate("Toggle read-only"));
}

void SongControllerImpl::clearSelection(const SelectedData& selectedData) noexcept
{
    auto action = std::make_unique<ClearCells>(*this, selectedData);
    performAction(std::move(action), juce::translate("Clear selection"));
}

void SongControllerImpl::insertCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept
{
    auto action = std::make_unique<InsertCellAt>(*this, location, cursorLocation);
    performAction(std::move(action), juce::translate("Insert cell"));
}

void SongControllerImpl::removeCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept
{
    auto action = std::make_unique<RemoveCellAt>(*this, location, cursorLocation);
    performAction(std::move(action), juce::translate("Remove cell"));
}

void SongControllerImpl::createNewSubsong(const std::vector<Psg>& psgs, const Properties& metadata) noexcept
{
    auto action = std::make_unique<CreateSubsong>(*this, metadata, psgs);
    performAction(std::move(action), juce::translate("Create subsong"));
}

void SongControllerImpl::deleteSubsong(const Id& subsongId) noexcept
{
    auto action = std::make_unique<DeleteSubsong>(*this, subsongId);
    performAction(std::move(action), juce::translate("Delete subsong"));
}

void SongControllerImpl::changeSubsongMetadata(const Id& subsongId, const Properties& metadata) noexcept
{
    auto action = std::make_unique<SetSubsongMetadata>(*this, subsongId, metadata);
    performAction(std::move(action), juce::translate("Change subsong metadata"));
}

int SongControllerImpl::getSubsongCount() const noexcept
{
    return song->getSubsongCount();
}

void SongControllerImpl::changeSubsongPsgAndMetadata(const Id& subsongId, const std::vector<Psg>& psgs, const Properties& metadata) noexcept
{
    auto action = std::make_unique<SetSubsongPsgs>(*this, subsongId, psgs, metadata);
    performAction(std::move(action), juce::translate("Change PSGs"));
}

void SongControllerImpl::setCurrentlyShownLine(const int shownLine) noexcept
{
    currentlyShownLine = shownLine;
}

int SongControllerImpl::getCurrentlyShownLine() const noexcept
{
    return currentlyShownLine;
}

void SongControllerImpl::setExportedSoundEffects(const std::unordered_set<int>& exportedInstrumentIndexes)
{
    auto action = std::make_unique<SetExportedSoundEffects>(*this, exportedInstrumentIndexes);
    performAction(std::move(action), juce::translate("Change exported sound effects"));
}

}   // namespace arkostracker
