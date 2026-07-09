#include "SongControllerNoOp.h"

namespace arkostracker 
{

SongControllerNoOp::SongControllerNoOp() noexcept :
        mainController(),
        songMetadataObservers(),
        subsongMetadataObservers(),
        linkerObservers(),
        expressionChangeObservers(),
        instrumentChangeObserver(),
        trackChangeObservers(),
        expressionHandler(true, false)
{
}

std::shared_ptr<Song> SongControllerNoOp::getSong()
{
    return nullptr;
}

std::shared_ptr<const Song> SongControllerNoOp::getConstSong() const
{
    return nullptr;
}

MainController& SongControllerNoOp::getMainController()
{
    return mainController;
}

Id SongControllerNoOp::getCurrentSubsongId() const noexcept
{
    return { };
}

Location SongControllerNoOp::getCurrentViewedLocation() const noexcept
{
    return { { }, 0, 0 };
}

void SongControllerNoOp::setCurrentLocation(const Location& /*location*/, const bool /*alsoChangeLine*/) noexcept
{

}

void SongControllerNoOp::performOnConstSubsong(const Id& /*subsongIndex*/, const std::function<void(const Subsong&)>& /*function*/) const noexcept
{
}

bool SongControllerNoOp::canUndo() const
{
    return false;
}

bool SongControllerNoOp::canRedo() const
{
    return false;
}

void SongControllerNoOp::undo()
{
}

void SongControllerNoOp::redo()
{
}

juce::String SongControllerNoOp::getUndoDescription()
{
    return { };
}

juce::String SongControllerNoOp::getRedoDescription()
{
    return { };
}

std::vector<juce::String> SongControllerNoOp::getExpressionNames(bool /*isArpeggio*/) const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getExpressionName(bool /*isArpeggio*/, const Id& /*expressionId*/) const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getExpressionNameFromIndex(bool /*isArpeggio*/, int /*expressionIndex*/) const noexcept
{
    return {};
}

bool SongControllerNoOp::isReadOnlyExpression(bool /*isArpeggio*/, const Id& /*expressionId*/) const noexcept
{
    return false;
}

void SongControllerNoOp::setExpressionName(bool /*isArpeggio*/, const Id& /*expressionId*/, const juce::String& /*newName*/) noexcept
{
}

void SongControllerNoOp::deleteExpressions(bool /*isArpeggio*/, std::set<int> /*indexesToDelete*/) noexcept
{
}

int SongControllerNoOp::getExpressionCount(bool /*isArpeggio*/) const noexcept
{
    return 0;
}

void SongControllerNoOp::insertExpressions(bool /*isArpeggio*/, int /*expressionIndexWhereToInsert*/, std::vector<std::unique_ptr<Expression>> /*expressions*/) noexcept
{
}

void SongControllerNoOp::moveExpressions(bool /*isArpeggio*/, const std::set<int>& /*indexesToDelete*/, int /*destinationIndex*/) noexcept
{
}

void SongControllerNoOp::applyOnExpressionCells(const Id& /*expressionId*/, bool /*isArpeggio*/, int /*cellIndex*/, int /*cellCountToModify*/, const juce::String& /*actionName*/,
        std::function<int(int iterationIndex, int initialValue)> /*actionOnCell*/) noexcept
{
}

bool SongControllerNoOp::isExpressionArpeggio(const Id& /*expressionId*/) const noexcept
{
    return false;
}

void SongControllerNoOp::setPositionMarker(const Id& /*subsongId*/, int /* positionIndex*/, const juce::String& /*newName*/, juce::Colour /*newColor*/) noexcept
{
}

void SongControllerNoOp::modifyPosition(const Id& /*subsongId*/, int /* positionIndex*/, const Position& /*position*/) noexcept
{
}

void SongControllerNoOp::movePositionMarker(const Id& /*subsongId*/, int /*sourcePosition*/, int /*sourcePositionIndex*/) noexcept
{
}

void SongControllerNoOp::duplicatePositions(const Id& /*subsongId*/, const std::set<int>& /*positionsToDuplicate*/, int /*positionAfterWhichToInsert*/, bool /* insertAfter*/,
                                            bool /*alsoDuplicateMarker*/) noexcept
{
}

void SongControllerNoOp::createNewPositionAndPattern(const Id& /*subsongId*/, int /*positionAfterWhichToInsert*/) noexcept
{
}

void SongControllerNoOp::clonePositions(const Id& /*subsongId*/, const std::set<int>& /*positionsToDuplicate*/, int /*positionAfterWhichToInsert*/, bool /*insertAfter*/) noexcept
{
}

void SongControllerNoOp::deletePositions(const Id& /*subsongId*/, const std::set<int>& /*positionsToDelete*/, bool /*deleteOrCut*/) noexcept
{
}

void SongControllerNoOp::movePositions(const Id& /*subsongId*/, const std::set<int>& /* positions*/, int /* sourcePositionIndex*/, bool /*insertAfter*/) noexcept
{
}

void SongControllerNoOp::insertPositions(const Id& /*subsongId*/, const std::vector<Position>& /* positions*/, int /* sourcePositionIndex*/, bool /* insertAfter*/,
                                         bool /*alsoDuplicateMarker*/) noexcept
{
}

void SongControllerNoOp::modifyPositionsData(const Id& /*subsongId*/, const std::set<int>& /*positionIndexes*/, OptionalInt /*newHeight*/,
                                             OptionalValue<juce::uint32> /*newPatternColorArgb*/) noexcept
{
}

void SongControllerNoOp::modifyPattern(const Id& /*subsongId*/, int /*patternIndex*/, const Pattern& /*pattern*/) noexcept
{
}

int SongControllerNoOp::getChannelCount(const Id& /*subsongId*/) const noexcept
{
    return 0;
}

void SongControllerNoOp::createTrackLink(const Id& /*subsongId*/, int /*sourcePositionIndex*/, int /*sourceChannelIndex*/, int /*targetPositionIndex*/,
    int /*targetChannelIndex*/)
{
}

void SongControllerNoOp::unlinkTrack(const Id& /*subsongId*/, int /*positionIndexToUnlink*/, int /*channelIndexToUnlink*/) noexcept
{
}

void SongControllerNoOp::setLoopStartAndEnd(const Id& /*subsongId*/, int /*loopStartPosition*/, int /*endPosition*/) noexcept
{
}

void SongControllerNoOp::createSpecialTrackLink(const Id& /*subsongId*/, bool /*isSpeedTrack*/, int /*sourcePositionIndex*/, int /*targetPositionIndex*/)
{
}

void SongControllerNoOp::unlinkSpecialTrack(const Id& /*subsongId*/, bool /*isSpeedTrack*/, int /*positionIndexToUnlink*/) noexcept
{
}

void SongControllerNoOp::clearPatterns(const Id& /*subsongId*/) noexcept
{
}

void SongControllerNoOp::rearrangePatterns(const Id& /*subsongId*/, bool /*deleteUnused*/) noexcept
{
}

std::vector<Psg> SongControllerNoOp::getPsgs(const Id& /*subsongId*/) const noexcept
{
    return { };
}

std::unordered_map<int, juce::uint32> SongControllerNoOp::buildInstrumentToColor() const noexcept
{
    return { };
}

void SongControllerNoOp::setPositionHeight(const Location& /*location*/, int /*newHeight*/) noexcept
{
}

void SongControllerNoOp::setPositionTransposition(const Location& /*location*/, int /*channelIndex*/, int /*newTransposition*/) noexcept
{
}

void SongControllerNoOp::increasePatternIndex(const Id& /*subsongId*/, int /*positionIndex*/, int /*increaseStep*/) noexcept
{
}

void SongControllerNoOp::setTrackNameAndColor(const Location& /*location*/, int /*channelIndex*/, const juce::String& /*newName*/,
    const OptionalValue<juce::Colour>& /*newColor*/) noexcept
{
}

void SongControllerNoOp::setSpecialTrackName(const Location& /*location*/, bool /*isSpeedTrack*/, const juce::String& /*newName*/) noexcept
{
}

int SongControllerNoOp::getPsgCount(const Id& /*subsongId*/) const noexcept
{
    return 0;
}

Expression SongControllerNoOp::getExpression(bool /*isArpeggio*/, const Id& /*expressionId*/) noexcept
{
    return Expression(true, juce::String(), false);
}


OptionalId SongControllerNoOp::getInstrumentId(int /*index*/) const noexcept
{
    return { };
}

Id SongControllerNoOp::getLastInstrumentId() const noexcept
{
    return { };
}

OptionalInt SongControllerNoOp::getInstrumentIndex(const Id& /*instrumentId*/) const noexcept
{
    return { };
}

int SongControllerNoOp::getInstrumentCount() const noexcept
{
    return 0;
}

juce::String SongControllerNoOp::getInstrumentName(const Id& /*instrumentId*/) const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getInstrumentNameFromIndex(int /*instrumentIndex*/) const noexcept
{
    return { };
}

void SongControllerNoOp::performOnConstInstrument(const Id& /*instrumentId*/, const std::function<void(const Instrument&)>& /*function*/) const noexcept
{
}

void SongControllerNoOp::performOnInstrument(const Id& /*instrumentId*/, const std::function<void(Instrument&)>& /*function*/) const noexcept
{
}

Observers<InstrumentChangeObserver>& SongControllerNoOp::getInstrumentObservers() noexcept
{
    return instrumentChangeObserver;
}

void SongControllerNoOp::setInstrumentName(const Id& /*instrumentId*/, const juce::String& /*newName*/) noexcept
{
}

void SongControllerNoOp::deleteInstruments(const std::set<int>& /*indexesToDelete*/) noexcept
{
}

bool SongControllerNoOp::deleteInstrument(const Id& /*id*/) noexcept
{
    return false;
}

void SongControllerNoOp::performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& /*lockedOperation*/) noexcept
{
}

bool SongControllerNoOp::isReadOnlyInstrument(const Id& /*instrumentId*/) const noexcept
{
    return false;
}

void SongControllerNoOp::moveInstruments(const std::set<int>& /*indexesToDelete*/, int /*destinationIndex*/) noexcept
{
}

void SongControllerNoOp::insertInstruments(int /*indexWhereToInsert*/, std::vector<std::unique_ptr<Instrument>> /*instruments*/) noexcept
{
}

void SongControllerNoOp::addInstrument(std::unique_ptr<Instrument> /*instrument*/) noexcept
{
}

void SongControllerNoOp::setInstrumentColor(const Id& /*instrumentId*/, juce::Colour /*newColor*/) noexcept
{
}

Observers<SongMetadataObserver>& SongControllerNoOp::getSongMetadataObservers() noexcept
{
    return songMetadataObservers;
}

Observers<SubsongMetadataObserver>& SongControllerNoOp::getSubsongMetadataObservers() noexcept
{
    return subsongMetadataObservers;
}

Observers<LinkerObserver>& SongControllerNoOp::getLinkerObservers() noexcept
{
    return linkerObservers;
}

Observers<ExpressionChangeObserver>& SongControllerNoOp::getExpressionObservers(bool /*isArpeggio*/) noexcept
{
    return expressionChangeObservers;
}

Observers<TrackChangeObserver>& SongControllerNoOp::getTrackObservers() noexcept
{
    return trackChangeObservers;
}

void SongControllerNoOp::setPsgInstrumentCell(const Id& /*instrumentId*/, int /*cellIndex*/, const PsgInstrumentCell& /*cell*/) noexcept
{
}

void SongControllerNoOp::setPsgInstrumentMetadata(const Id& /*instrumentId*/, OptionalInt /*newLoopStart*/, OptionalInt /*newEnd*/, OptionalBool /*newIsLoop*/,
        OptionalBool /*newIsRetrig*/, OptionalInt /*newSpeed*/, std::unordered_map<PsgSection, Loop> /*modifiedSectionToAutoSpreadLoop*/)
{
}

ExpressionHandler& SongControllerNoOp::getExpressionHandler(bool /*isArpeggio*/) noexcept
{
    return expressionHandler;
}

void SongControllerNoOp::setExpressionCell(bool /*isArpeggio*/, const Id& /*expressionId*/, int /*cellIndex*/, int /*value*/) noexcept
{
}

void SongControllerNoOp::setExpressionMetadata(bool /*isArpeggio*/, const Id& /*expressionId*/, OptionalInt /*newLoopStart*/, OptionalInt /*newEnd*/,
                                               OptionalInt /*newSpeed*/, OptionalInt /*newShift*/)
{
}

void SongControllerNoOp::setSong(std::shared_ptr<Song> /*newSong*/)
{
}

void SongControllerNoOp::clearUndo()
{
}

void SongControllerNoOp::markSongAsSaved() noexcept
{
}

bool SongControllerNoOp::isSongModified() const noexcept
{
    return false;
}

bool SongControllerNoOp::performAction(std::unique_ptr<juce::UndoableAction> /*action*/, const juce::String& /*name*/) noexcept
{
    return false;
}

juce::String SongControllerNoOp::getTitle() const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getAuthor() const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getComposer() const noexcept
{
    return { };
}

juce::String SongControllerNoOp::getComments() const noexcept
{
    return { };
}

void SongControllerNoOp::setSongMetadata(OptionalValue<juce::String> /*newTitle*/, OptionalValue<juce::String> /*newAuthor*/, OptionalValue<juce::String> /*newComposer*/,
                                         OptionalValue<juce::String> /*newComments*/) noexcept
{
}

void SongControllerNoOp::performOnSubsong(const Id& /*subsongIndex*/, const std::function<void(Subsong&)>& /*function*/) const noexcept
{
}

void SongControllerNoOp::setCell(const Location& /*location*/, int /*channelIndex*/, OptionalValue<Note> /*newNote*/, OptionalInt /*newInstrument*/,
                                 OptionalValue<EffectDigit> /*newEffectDigit*/, OptionalValue<EffectNumber> /*newEffectNumber*/,
                                 OptionalValue<Digit> /*newInstrumentDigit*/) noexcept
{
}

void SongControllerNoOp::setSpecialCell(bool /*isSpeedTrack*/, const Location& /*location*/, Digit /*digit*/) noexcept
{
}

void SongControllerNoOp::pasteCells(const Location& /*location*/, const CursorLocation& /*cursorLocation*/, const PasteData& /*pasteData*/, bool /*pasteMix*/,
    bool /*pasteContinuously*/) noexcept
{
}

void SongControllerNoOp::transpose(TransposeRate /*transposeRate*/, const SelectedData& /*selectedData*/) noexcept
{
}

void SongControllerNoOp::toggleTracksReadOnly(Id /*subsongId*/, const std::set<int>& /*trackLinksOrIndexes*/, OptionalInt /*speedTrackLinkOrIndex*/,
    OptionalInt /*eventTrackLinkOrIndex*/) noexcept
{
}

void SongControllerNoOp::insertCellAt(const Location& /*location*/, const CursorLocation& /*cursorLocation*/) noexcept
{
}

void SongControllerNoOp::removeCellAt(const Location& /*location*/, const CursorLocation& /*cursorLocation*/) noexcept
{
}

void SongControllerNoOp::clearSelection(const SelectedData& /*selectedData*/) noexcept
{
}

int SongControllerNoOp::getChannelCount() const noexcept
{
    return 0;
}

void SongControllerNoOp::createNewSubsong(const std::vector<Psg>& /*psgs*/, const Properties& /*metadata*/) noexcept
{
}

int SongControllerNoOp::getSubsongCount() const noexcept
{
    return 0;
}

void SongControllerNoOp::deleteSubsong(const Id& /*subsongId*/) noexcept
{
}

void SongControllerNoOp::changeSubsongMetadata(const Id& /*subsongId*/, const Properties& /*metadata*/) noexcept
{
}

void SongControllerNoOp::changeSubsongPsgAndMetadata(const Id& /*subsongId*/, const std::vector<Psg>& /*psgs*/, const Properties& /*metadata*/) noexcept
{
}

OptionalInt SongControllerNoOp::getExpressionIndex(bool /*isArpeggio*/, const Id& /*expressionId*/) const noexcept
{
    return { };
}

OptionalId SongControllerNoOp::getExpressionId(bool /*isArpeggio*/, int /*expressionIndex*/) const noexcept
{
    return { };
}

Id SongControllerNoOp::getLastExpressionId(bool /*isArpeggio*/) const noexcept
{
    return { };
}

bool SongControllerNoOp::duplicateInstrumentCell(const Id& /*instrumentId*/, int /*cellIndex*/) noexcept
{
    return false;
}

bool SongControllerNoOp::duplicateExpressionCell(bool /*isArpeggio*/, const Id& /*expressionId*/, int /*cellIndex*/) noexcept
{
    return false;
}

bool SongControllerNoOp::deleteExpressionCell(bool /*isArpeggio*/, const Id& /*expressionId*/, int /*cellIndex*/) noexcept
{
    return false;
}

bool SongControllerNoOp::deleteInstrumentCell(const Id& /*expressionId*/, int /*cellIndex*/) noexcept
{
    return false;
}

void SongControllerNoOp::toggleRetrig(const Id& /*instrumentId*/, int /*cellIndex*/) noexcept
{
}

void SongControllerNoOp::generateIncreasingVolume(const Id& /*instrumentId*/, int /*cellIndex*/) noexcept
{
}

void SongControllerNoOp::generateDecreasingVolume(const Id& /*instrumentId*/, int /*cellIndex*/) noexcept
{
}

void SongControllerNoOp::applyOnCells(const Id& /*instrumentId*/, int /*cellIndex*/, int /*cellCountToModify*/, const juce::String& /*actionName*/,
                                      std::function<PsgInstrumentCell(int /*iterationIndex*/, const PsgInstrumentCell& cell)> /*actionOnCell*/) noexcept
{
}

void SongControllerNoOp::setCurrentlyShownLine(int /*shownLine*/) noexcept
{
}

int SongControllerNoOp::getCurrentlyShownLine() const noexcept
{
    return 0;
}

void SongControllerNoOp::setSampleInstrumentMetadata(const Id& /*instrumentId*/, OptionalInt /*newLoopStart*/,
                                                     OptionalInt /*newEnd*/, OptionalBool /*newIsLoop*/, OptionalFloat /*newAmplification*/,
                                                     OptionalInt /*newDigiNote*/)
{
}

void SongControllerNoOp::setSample(const Id& /*instrumentId*/, std::unique_ptr<Sample> /*sample*/, int /*sampleFrequencyHz*/, const juce::String& /*originalFileName*/)
{
}

void SongControllerNoOp::setExportedSoundEffects(const std::unordered_set<int>& /*exportedInstrumentIndexes*/)
{
}

void SongControllerNoOp::storeCurrentLocation(const Location& /*location*/) noexcept
{
}

}   // namespace arkostracker
