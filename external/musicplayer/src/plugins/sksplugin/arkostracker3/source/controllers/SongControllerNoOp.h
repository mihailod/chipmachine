#pragma once

#include "../ui/patternViewer/controller/dataItem/Digit.h"
#include "../utils/MainControllerNoOp.h"
#include "SongController.h"

namespace arkostracker 
{

/** A SongController that does nothing. */
class SongControllerNoOp : public SongController
{
public:
    SongControllerNoOp() noexcept;

    void setSong(std::shared_ptr<Song> newSong) override;
    std::shared_ptr<Song> getSong() override;
    std::shared_ptr<const Song> getConstSong() const override;
    MainController& getMainController() override;
    Id getCurrentSubsongId() const noexcept override;
    Location getCurrentViewedLocation() const noexcept override;
    void setCurrentLocation(const Location& location, bool alsoChangeLine) noexcept override;
    void storeCurrentLocation(const Location& location) noexcept override;
    Observers<SongMetadataObserver>& getSongMetadataObservers() noexcept override;
    Observers<SubsongMetadataObserver>& getSubsongMetadataObservers() noexcept override;
    Observers<LinkerObserver>& getLinkerObservers() noexcept override;

    juce::String getTitle() const noexcept override;
    juce::String getAuthor() const noexcept override;
    juce::String getComposer() const noexcept override;
    juce::String getComments() const noexcept override;

    void setSongMetadata(OptionalValue<juce::String> newTitle, OptionalValue<juce::String> newAuthor, OptionalValue<juce::String> newComposer,
                         OptionalValue<juce::String> newComments) noexcept override;

    void performOnConstSubsong(const Id& subsongId, const std::function<void(const Subsong&)>& function) const noexcept override;
    void performOnSubsong(const Id& subsongId, const std::function<void(Subsong&)>& function) const noexcept override;

    bool canUndo() const override;
    bool canRedo() const override;
    void undo() override;
    void redo() override;
    juce::String getUndoDescription() override;
    juce::String getRedoDescription() override;
    void clearUndo() override;
    void markSongAsSaved() noexcept override;
    bool isSongModified() const noexcept override;
    bool performAction(std::unique_ptr<juce::UndoableAction> action, const juce::String& name) noexcept override;

    Observers<ExpressionChangeObserver>& getExpressionObservers(bool isArpeggio) noexcept override;
    Observers<TrackChangeObserver>& getTrackObservers() noexcept override;

    void setCell(const Location& location, int channelIndex, OptionalValue<Note> newNote, OptionalInt newInstrument, OptionalValue<EffectDigit> newEffectDigit,
                 OptionalValue<EffectNumber> newEffectNumber, OptionalValue<Digit> newInstrumentDigit) noexcept override;
    void setSpecialCell(bool isSpeedTrack, const Location& location, Digit digit) noexcept override;
    void pasteCells(const Location& location, const CursorLocation& cursorLocation, const PasteData& pasteData, bool pasteMix, bool pasteContinuously) noexcept override;

    void transpose(TransposeRate transposeRate, const SelectedData& selectedData) noexcept override;
    void toggleTracksReadOnly(Id subsongId, const std::set<int>& trackIndexes, OptionalInt speedTrackIndex, OptionalInt eventTrackIndex) noexcept override;
    void insertCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept override;
    void removeCellAt(const Location& location, const CursorLocation& cursorLocation) noexcept override;
    void clearSelection(const SelectedData& selectedData) noexcept override;

    std::vector<juce::String> getExpressionNames(bool isArpeggio) const noexcept override;
    juce::String getExpressionName(bool isArpeggio, const Id& expressionId) const noexcept override;
    juce::String getExpressionNameFromIndex(bool isArpeggio, int expressionIndex) const noexcept override;
    bool isReadOnlyExpression(bool isArpeggio, const Id& expressionId) const noexcept override;
    void setExpressionName(bool isArpeggio, const Id& expressionId, const juce::String& newName) noexcept override;
    void deleteExpressions(bool isArpeggio, std::set<int> indexesToDelete) noexcept override;
    int getExpressionCount(bool isArpeggio) const noexcept override;
    void insertExpressions(bool isArpeggio, int expressionIndexWhereToInsert, std::vector<std::unique_ptr<Expression>> expressions) noexcept override;
    void moveExpressions(bool isArpeggio, const std::set<int>& indexesToDelete, int destinationIndex) noexcept override;
    void applyOnExpressionCells(const Id& expressionId, bool isArpeggio, int cellIndex, int cellCountToModify, const juce::String& actionName,
            std::function<int(int iterationIndex, int initialValue)> actionOnCell) noexcept override;
    bool isExpressionArpeggio(const Id &expressionId) const noexcept override;

    void setPositionMarker(const Id& subsongId, int positionIndex, const juce::String& newName, juce::Colour newColor) noexcept override;
    void modifyPosition(const Id& subsongId, int positionIndex, const Position& position) noexcept override;
    void movePositionMarker(const Id& subsongId, int sourcePosition, int destinationPosition) noexcept override;
    void duplicatePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int positionAfterWhichToInsert, bool insertAfter,
                            bool alsoDuplicateMarker) noexcept override;
    void createNewPositionAndPattern(const Id& subsongId, int positionAfterWhichToInsert) noexcept override;
    void clonePositions(const Id& subsongId, const std::set<int>& positionsToDuplicate, int positionAfterWhichToInsert, bool insertAfter) noexcept override;
    void deletePositions(const Id& subsongId, const std::set<int>& positionsToDelete, bool deleteOrCut) noexcept override;
    void movePositions(const Id& subsongId, const std::set<int>& positions, int destinationPosition, bool insertAfter) noexcept override;
    void insertPositions(const Id& subsongId, const std::vector<Position>& positions, int destinationPosition, bool insertAfter,
                        bool alsoDuplicateMarker) noexcept override;
    void modifyPositionsData(const Id& subsongId, const std::set<int>& positionIndexes, OptionalInt newHeight, OptionalValue<juce::uint32> newPatternColorArgb) noexcept override;
    void increasePatternIndex(const Id& subsongId, int positionIndex, int increaseStep) noexcept override;
    void modifyPattern(const Id& subsongId, int patternIndex, const Pattern& pattern) noexcept override;
    void createTrackLink(const Id& subsongId, int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) override;
    void unlinkTrack(const Id& subsongId, int positionIndexToUnlink, int channelIndexToUnlink) noexcept override;
    void createSpecialTrackLink(const Id& subsongId, bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) override;
    void unlinkSpecialTrack(const Id& subsongId, bool isSpeedTrack, int positionIndexToUnlink) noexcept override;
    void clearPatterns(const Id& subsongId) noexcept override;
    void rearrangePatterns(const Id& subsongId, bool deleteUnused) noexcept override;

    void setPositionHeight(const Location& location, int newHeight) noexcept override;
    void setPositionTransposition(const Location& location, int channelIndex, int newTransposition) noexcept override;
    void setTrackNameAndColor(const Location& location, int channelIndex, const juce::String& newName, const OptionalValue<juce::Colour>& newColor) noexcept override;
    void setSpecialTrackName(const Location& location, bool isSpeedTrack, const juce::String& newName) noexcept override;

    int getChannelCount(const Id& subsongId) const noexcept override;
    int getChannelCount() const noexcept override;

    void setLoopStartAndEnd(const Id& subsongId, int loopStartPosition, int endPosition) noexcept override;
    std::vector<Psg> getPsgs(const Id& subsongId) const noexcept override;
    int getPsgCount(const Id& subsongId) const noexcept override;

    OptionalId getInstrumentId(int index) const noexcept override;
    Id getLastInstrumentId() const noexcept override;
    OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept override;
    int getInstrumentCount() const noexcept override;
    std::unordered_map<int, juce::uint32> buildInstrumentToColor() const noexcept override;
    juce::String getInstrumentName(const Id& instrumentId) const noexcept override;
    juce::String getInstrumentNameFromIndex(int instrumentIndex) const noexcept override;
    void setInstrumentName(const Id& instrumentId, const juce::String& newName) noexcept override;
    void setInstrumentColor(const Id& instrumentId, juce::Colour newColor) noexcept override;
    void performOnConstInstrument(const Id& instrumentId, const std::function<void(const Instrument&)>& function) const noexcept override;
    void performOnInstrument(const Id& instrumentId, const std::function<void(Instrument&)>& function) const noexcept override;
    void performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) noexcept override;
    bool isReadOnlyInstrument(const Id& instrumentId) const noexcept override;
    Observers<InstrumentChangeObserver>& getInstrumentObservers() noexcept override;
    void deleteInstruments(const std::set<int>& indexesToDelete) noexcept override;
    bool deleteInstrument(const Id& id) noexcept override;
    Expression getExpression(bool isArpeggio, const Id& expressionId) noexcept override;
    ExpressionHandler& getExpressionHandler(bool isArpeggio) noexcept override;
    bool duplicateExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept override;
    bool deleteExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex) noexcept override;

    void setExpressionCell(bool isArpeggio, const Id& expressionId, int cellIndex, int value) noexcept override;

    void setExpressionMetadata(bool isArpeggio, const Id& expressionId, OptionalInt newLoopStart, OptionalInt newEnd,
                               OptionalInt newSpeed, OptionalInt newShift) override;
    OptionalInt getExpressionIndex(bool isArpeggio, const Id& expressionId) const noexcept override;
    OptionalId getExpressionId(bool isArpeggio, int expressionIndex) const noexcept override;
    Id getLastExpressionId(bool isArpeggio) const noexcept override;

    void moveInstruments(const std::set<int>& indexesToDelete, int destinationIndex) noexcept override;
    void insertInstruments(int indexWhereToInsert, std::vector<std::unique_ptr<Instrument>> instruments) noexcept override;
    void addInstrument(std::unique_ptr<Instrument> instrument) noexcept override;
    void setPsgInstrumentCell(const Id& instrumentId, int cellIndex, const PsgInstrumentCell& cell) noexcept override;
    bool duplicateInstrumentCell(const Id& instrumentId, int cellIndex) noexcept override;
    bool deleteInstrumentCell(const Id& instrumentId, int cellIndex) noexcept override;

    void toggleRetrig(const Id& instrumentId, int cellIndex) noexcept override;
    void generateIncreasingVolume(const Id& instrumentId, int cellIndex) noexcept override;
    void generateDecreasingVolume(const Id& instrumentId, int cellIndex) noexcept override;
    void applyOnCells(const Id& instrumentId, int cellIndex, int cellCountToModify, const juce::String& actionName,
                      std::function<PsgInstrumentCell(int iterationIndex, const PsgInstrumentCell& cell)> actionOnCell) noexcept override;

    void setPsgInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop, OptionalBool newIsRetrig,
            OptionalInt newSpeed, std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) override;
    void setSampleInstrumentMetadata(const Id& instrumentId, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop,
                                     OptionalFloat newAmplification, OptionalInt newDigiNote) override;
    void setSample(const Id& instrumentId, std::unique_ptr<Sample> sample, int sampleFrequencyHz, const juce::String& originalFileName) override;

    void createNewSubsong(const std::vector<Psg>& psgs, const Properties& metadata) noexcept override;
    void changeSubsongMetadata(const Id& subsongId, const Properties& metadata) noexcept override;
    void changeSubsongPsgAndMetadata(const Id& subsongId, const std::vector<Psg>& psgs, const Properties& metadata) noexcept override;

    void deleteSubsong(const Id& subsongId) noexcept override;
    int getSubsongCount() const noexcept override;

    void setCurrentlyShownLine(int shownLine) noexcept override;

    int getCurrentlyShownLine() const noexcept override;

    void setExportedSoundEffects(const std::unordered_set<int>& exportedInstrumentIndexes) override;

private:
    MainControllerNoOp mainController;
    Observers<SongMetadataObserver> songMetadataObservers;
    Observers<SubsongMetadataObserver> subsongMetadataObservers;
    Observers<LinkerObserver> linkerObservers;
    Observers<ExpressionChangeObserver> expressionChangeObservers;
    Observers<InstrumentChangeObserver> instrumentChangeObserver;
    Observers<TrackChangeObserver> trackChangeObservers;
    ExpressionHandler expressionHandler;
};

}   // namespace arkostracker

