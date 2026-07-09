#pragma once

#include <set>
#include <vector>

#include "../../business/model/Loop.h"
#include "../../utils/Id.h"
#include "../SpecialTrackLocation.h"
#include "../TrackLocation.h"
#include "../psg/Psg.h"
#include "../sid/SidPlayerCapability.h"
#include "../tracks/SpecialTrack.h"
#include "../tracks/Track.h"
#include "LinkState.h"
#include "Pattern.h"
#include "Position.h"

namespace arkostracker 
{

class CellLocationInPosition;

/**
 * A Subsong.
 *
 * It does NOT lock its access to allow concurrent access: this is done by the Song.
 * It does NOT handle observations on changes.
 * It does NOT manage redo/undo itself.
 * See the SongController for that.
 */
class Subsong
{
public:
    /**
     * A simple holder of the Subsong metadata. This class is ONLY a convenience class to GET the metadata without having to get the fields
     * one by one. It is NOT meant to be set.
     */
    class Metadata
    {
    public:
        Metadata(juce::String pName, int initialSpeed, float replayFrequencyHz, int digiChannel,
                 int highlightSpacing, int secondaryHighlight, int loopStartPosition, int endPosition,
                 const SidPlayerCapability& sidPlayerCapability) noexcept;

        /** @return a default Metadata object. */
        static Metadata buildDefault() noexcept;

        bool operator==(const Metadata& rhs) const;
        bool operator!=(const Metadata& rhs) const;

        const juce::String& getName() const;
        int getInitialSpeed() const;
        float getReplayFrequencyHz() const;
        int getDigiChannel() const;
        int getHighlightSpacing() const;
        int getSecondaryHighlight() const;
        int getLoopStartPosition() const;
        int getEndPosition() const;
        SidPlayerCapability getSidPlayerCapability() const;

    private:
        const juce::String name;
        const int initialSpeed;
        const float replayFrequencyHz;
        const int digiChannel;
        const int highlightSpacing;
        const int secondaryHighlight;
        const int loopStartPosition;
        const int endPosition;
        const SidPlayerCapability sidPlayerCapability;
    };

     /**
      * Constructor of an empty Subsong. It is invalid, because it has no pattern, no Tracks! Use this to create one from
      * scratch.
      * @param name the name.
      * @param initialSpeed the initial speed (>0).
      * @param replayFrequencyHz frequency in hz the song must be played (12.5, 25, 50...).
      * @param digiChannel channel number on which the drums should be played (>= 0).
      * @param highlightSpacing the highlight spacing (>=1).
      * @param secondaryHighlight the secondary highlight (>1), in unit of highlight.
      * @param loopStartPosition the index of the pattern when looping.
      * @param endPosition the index of the last pattern to play.
      * @param psgs the PSGs of the Subsong.
      * @param sidPlayerCapability the SID player capabilities.
      * @param createDefaultData true to create first tracks and a position. If false, the Subsong is invalid.
      */
    Subsong(juce::String name, int initialSpeed, float replayFrequencyHz, int digiChannel,
            int highlightSpacing, int secondaryHighlight,
            int loopStartPosition, int endPosition, std::vector<Psg> psgs,
            const SidPlayerCapability& sidPlayerCapability,
            bool createDefaultData = false) noexcept;

    /** @return the name. */
    const juce::String& getName() const noexcept;

    /**
     * Sets the name.
     * @param name the name.
     */
    void setName(const juce::String& name) noexcept;

    /** @return how many positions there are (without taking the loop end in account). */
    int getLength() const noexcept;

    /** @return how many PSG there are (>0). */
    int getPsgCount() const noexcept;
    /** @return how many channels there are (>=3). */
    int getChannelCount() const noexcept;

    /** @return the replay frequency, in hz (50 for example). */
    float getReplayFrequencyHz() const noexcept;

    /** @return the DigiChannel (>=0). */
    int getDigiChannel() const noexcept;
    /**
     * Sets the DigiChannel.
     * @param digiChannel the DigiChannel (>=0).
     */
    void setDigiChannel(int digiChannel) noexcept;

    /**
     * Sets both highlight spacings.
     * @param primary the primary highlight spacing.
     * @param secondary the secondary highlight spacing.
     */
    void setHighlightSpacings(int primary, int secondary) noexcept;

    /**
     * Sets the initial speed.
     * @param initialSpeed the initial speed (>0).
     */
    void setInitialSpeed(int initialSpeed) noexcept;

    /** @return the loop start position. */
    int getLoopStartPosition() const noexcept;
    /** @return the end position. */
    int getEndPosition() const noexcept;
    /** @return the loop. */
    Loop getLoop() const noexcept;

    /** @return how many Tracks there are. */
    int getTrackCount() const noexcept;

    /** @return the PSG metadata for this Subsong. */
    std::vector<Psg> getPsgs() const noexcept;
    /** @return the PSG metadata, as a reference, for the Subsong. */
    const std::vector<Psg>& getPsgRefs() const noexcept;
    /**
     * Sets the PSGs. Warning, this is the ONLY thing it does! The related channels in the Patterns should have the same count.
     * @param psgs the PSGs. There must be at least one!
     */
    void setPsgs(std::vector<Psg> psgs) noexcept;

    /** @return the Sid player capabilities. */
    SidPlayerCapability getSidPlayerCapability() const noexcept;
    /** Sets the Sid player capabilities. */
    void setSidPlayerCapability(const SidPlayerCapability& sidPlayerCapability) noexcept;

    /**
     * Sets the loop start and end, or only one of them. If invalid, they are corrected.
     * @param loopStartPosition the loop start position, or not present.
     * @param endPosition the end position, or not present.
     */
    void setLoopAndEndStartPosition(OptionalInt loopStartPosition, OptionalInt endPosition) noexcept;

    /** Sets the loop. If invalid, it is corrected. */
    void setLoop(const Loop& loop) noexcept;

    /** Sets the replay frequency. If invalid, it is corrected. */
    void setReplayFrequency(float replayFrequencyHz) noexcept;

    /** @return the start/end position.*/
    std::pair<int, int> getLoopStartAndEndPosition() const noexcept;

    /** @return the metadata. Handy when wanting several data, one call is enough. */
    Metadata getMetadata() const noexcept;

    /** @return the unique ID of the Subsong. Do not serialize it. */
    Id getId() const noexcept;

    /** This only compares the IDs. */
    bool operator==(const Subsong& rhs) const;
    bool operator!=(const Subsong& rhs) const;

    /** @return true if the musical content is equal (name is skipped). */
    bool isMusicallyEqual(const Subsong& other) const;

    // Positions.
    // =======================================

    /**
     * Replaces the data of a Position with new one.
     * @param positionIndex the index of the Position. Must be valid.
     * @param positionData the new data.
     */
    void setPosition(int positionIndex, const Position& positionData) noexcept;

    /**
     * Adds a Position at the end. This may update the loop start/end.
     * @param position the position (a copy is made).
     * @return the index of the added Position.
     */
    int addPosition(const Position& position) noexcept;

    /**
     * Adds a Position. This may update the loop start/end.
     * @param insertionIndex where to insert (>=0).
     * @param insertAfter true to insert after the given index. This is useful to "push" the start/end loop.
     * @param position the position (a copy is made).
     */
    void addPosition(int insertionIndex, bool insertAfter, const Position& position) noexcept;

    /**
     * Adds Positions. This may update the loop start/end.
     * @param insertionIndex where to insert (>=0).
     * @param insertAfter true to insert after the given index. This is useful to "push" the start/end loop.
     * @param positions the positions (copies are made).
     */
    void addPositions(int insertionIndex, bool insertAfter, const std::vector<Position>& positions) noexcept;

    /**
     * Removes Positions.
     * @param index the first position to remove.
     * @param deleteAfter true to remove after the given index. This is useful to "pop" the start/end loop.
     * @param positionCountToRemove how many positions to remove.
     */
    void removePositions(int index, bool deleteAfter, int positionCountToRemove) noexcept;

    /**
     * Completely replaces the Positions with the given ones! Use with cautious!
     * Useful on batch Actions. Watch out for the loop validity, which is not modified.
     * @param positions the new Positions.
     */
    void setPositions(const std::vector<Position>& positions) noexcept;

    /**
     * Removes the last Patterns. This does NOT delete the related Tracks/Special Tracks.
     * @param count how many Patterns.
     */
    void removeLastPatterns(int count) noexcept;

    /**
     * Removes the last Tracks.
     * @param count how many Tracks.
     */
    void removeLastTracks(int count) noexcept;

    /**
     * Removes the last Special Tracks.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param count how many Special Tracks.
     */
    void removeLastSpecialTracks(bool speedTrack, int count) noexcept;

    /**
     * Removes Positions. This does not update the loop.
     * @param positions the positions to delete. Must be valid.
     */
    void removePositions(const std::set<int>& positions) noexcept;

    /**
     * @return the position from the given index.
     * @param positionIndex the index. Must be valid.
     */
    Position getPosition(int positionIndex) const noexcept;

    /**
     * @return the Positions. Quite useful for exports, or optimizers.
     * @param untilLoopEnd true to keep the Positions only up the loop end, false to keep up the full end.
     */
    std::vector<Position> getPositions(bool untilLoopEnd) const noexcept;
    /** @return all the Patterns. Quite useful for exports, or optimizers. */
    std::vector<Pattern> getPatterns() const noexcept;
    /** @return all the Tracks. Quite useful for exports, or optimizers. */
    std::vector<Track> getTracks() const noexcept;
    /**
     * @return all the Special Tracks. Quite useful for exports, or optimizers.
     * @param isSpeedTrack true to get the Speed Tracks, false for the Event Tracks.
     */
    std::vector<SpecialTrack> getSpecialTracks(bool isSpeedTrack) const noexcept;

    /**
     * @return the Position data.
     * This returns a ref, so make sure all the treatment is still perform in a locked block!
     * @param position the position index. Must be valid.
     */
    const Position& getPositionRef(int position) const noexcept;

    /**
     * @return true if the position exists.
     * @param positionIndex the position index.
     */
    bool doesPositionExist(int positionIndex) const noexcept;

    /**
     * @return the height of a Position.
     * @param positionIndex the index of the Position. If invalid, asserts and returns a default value.
     */
    int getPositionHeight(int positionIndex) const noexcept;

    /**
     * Sets the marker data of an existing position.
     * @param positionIndex the position. Must be valid.
     * @param text the marker name.
     * @param color the marker color.
     */
    void setPositionMarkerData(int positionIndex, const juce::String& text, juce::uint32 color) noexcept;

    /**
     * @returns the data of the marker of a Position.
     * @param positionIndex the index of the Position. Must be valid.
     */
    std::pair<juce::String, juce::uint32> getPositionMarker(int positionIndex) const noexcept;

    /**
     * @return a reference to a Track from a Position. Should be used in a transaction!
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the channel index. Must be valid.
     */
    const Track& getConstTrackRefFromPosition(int positionIndex, int channelIndex) const noexcept;

    /**
     * @return a reference to a Track from a Position. Should be used in a transaction!
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the channel index. Must be valid.
     */
    Track& getTrackRefFromPosition(int positionIndex, int channelIndex) noexcept;

    /**
     * @return a reference to a Track, from its index. Should be used in a transaction!
     * @param trackIndex the position index. Must be valid.
     */
    const Track& getTrackRefFromIndex(int trackIndex) const noexcept;

    /**
     * @return a reference to a Special Track, from the position index. Should be used in a transaction!
     * @param positionIndex the position index. Must be valid.
     * @param speedTrack true to get the Speed Track, false for Event Track.
     */
    SpecialTrack& getSpecialTrackRefFromPosition(int positionIndex, bool speedTrack) noexcept;

    /**
     * @return a reference to a Special Track, from the position index. Should be used in a transaction!
     * @param positionIndex the position index. Must be valid.
     * @param speedTrack true to get the Speed Track, false for Event Track.
     */
    const SpecialTrack& getSpecialTrackRefFromPosition(int positionIndex, bool speedTrack) const noexcept;

    /**
     * @return a reference to a Special Track, from its index. Should be used in a transaction!
     * @param specialTrackIndex the Special Track index. Must be valid.
     * @param speedTrack true to get the Speed Track, false for Event Track.
     */
    const SpecialTrack& getSpecialTrackRefFromIndex(int specialTrackIndex, bool speedTrack) const noexcept;

    /**
     * @return a reference to a Special Track, from its index. Should be used in a transaction!
     * @param specialTrackIndex the Special Track index. Must be valid.
     * @param speedTrack true to get the Speed Track, false for Event Track.
     */
    SpecialTrack& getSpecialTrackRefFromIndex(int specialTrackIndex, bool speedTrack) noexcept;

    /**
     * @return how many special tracks there are.
     * @param speedTrack true to get the Speed Track, false for Event Track.
     */
    int getSpecialTrackCount(bool speedTrack) const noexcept;


    // Patterns.
    // ==========================================================

    /**
     * @return the Pattern data related to the given Position.
     * This returns a ref, so make sure all the treatment is still perform in a locked block!
     * @param position the position index. Must be valid.
     */
    const Pattern& getPatternRef(int position) const noexcept;

    /**
     * @return the Pattern from its index.
     * @param patternIndex the pattern index. Must be valid.
     */
    Pattern getPatternFromIndex(int patternIndex) const noexcept;

    /**
     * @return the color of a Pattern.
     * @param patternIndex the index of the Pattern. Must be valid.
     *
     */
    juce::uint32 getPatternColor(int patternIndex) const noexcept;          // FIXME Fishy, like the methods below. I suspect the client is more interested in the Position index??

    /**
     * @return true if the Pattern exist.
     * @param patternIndex the index of the Pattern.
     */
    bool doesPatternExist(int patternIndex) const noexcept;

    /** @return how many Patterns there are. */
    int getPatternCount() const noexcept;

    /**
     * Adds a Pattern.
     * @param pattern the Pattern.
     * @return the index of the new Pattern.
     */
    int addPattern(const Pattern& pattern) noexcept;

    /**
     * Replaces a Pattern. It must exist!
     * @param patternIndex the pattern index. Must be valid.
     * @param pattern the new Pattern.
     */
    void setPattern(int patternIndex, const Pattern& pattern) noexcept;

    /**
     * Completely replaces the Patterns with the given ones! Use with cautious!
     * Useful on batch Actions.
     * @param patterns the new Patterns.
     */
    void setPatterns(const std::vector<Pattern>& patterns) noexcept;


    // Tracks.
    // ==========================================================

    /**
     * @return a reference to a Track, for modification. The Song should be locked!
     * @param trackIndex the index of the Track. Must be valid.
     */
    Track& getTrackRefFromIndex(int trackIndex) noexcept;

    /**
     * Adds a Track.
     * @param track the Track.
     * @return the index of the newly added Track.
     */
    int addTrack(const Track& track) noexcept;

    /**
     * Inserts an empty Cell in a Track.
     * @param trackIndex the Track index. It must exist.
     * @param cellIndex the cell index.
     * @param cellToInsert the cell to insert. Can be an empty Cell.
     * @return the Cell that has gone beyond the the track capacity.
     */
    Cell insertCellAt(int trackIndex, int cellIndex, const Cell& cellToInsert) noexcept;

    /**
     * Removes a Cell in a Track, shifting "up" all the Cells after.
     * @param trackIndex the Track index. It must exist.
     * @param cellIndex the cell index.
     * @param lastCell the Cell to write at the very end.
     * @return the removed Cell.
     */
    Cell removeCellAt(int trackIndex, int cellIndex, const Cell& lastCell) noexcept;

    /**
     * Sets the name of a track.
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the position index. Must be valid.
     * @param newName the new name.
     */
    void setTrackName(int positionIndex, int channelIndex, const juce::String& newName) noexcept;

    /**
     * Sets the name of a track.
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the position index. Must be valid.
     * @param newColor the new color, or absent if it has none.
     */
    void setTrackColor(int positionIndex, int channelIndex, const OptionalValue<juce::uint32>& newColor) noexcept;

    /**
     * @return the link state, that is, if this Track is linked or linked to, or none.
     * Note that this browses all the Positions. You may not want to do that too often, even though it shouldn't be a problem most of the time.
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the position index. Must be valid.
     */
    LinkState getTrackLinkState(int positionIndex, int channelIndex) const noexcept;

    /**
     * @return where a Track is used.
     * @param trackIndex the track index.
     * @param linkType what to look for.
     */
    std::vector<TrackLocation> findTrackUse(int trackIndex, LinkType linkType) const noexcept;


    // Special Tracks.
    // ==========================================================

    /**
     * Adds a Special Track.
     * @param isSpeedTrack true is speed track, false if event track.
     * @param specialTrack the Special Track.
     * @return the index of the newly added Special Track.
     */
    int addSpecialTrack(bool isSpeedTrack, const SpecialTrack& specialTrack) noexcept;

    /**
     * Stores a Special Cell.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the index of the Special Track. Must be valid.
     * @param lineIndex  the line index in the Track.
     * @param cell the Special Cell.
     */
    void setCellFromSpecialTrackIndex(bool isSpeedTrack, int specialTrackIndex, int lineIndex, const SpecialCell& cell) noexcept;

    /**
     * @return a SpecialCell. This call is safe: if the position isn't valid, an empty Cell is returned.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param position the position. May be invalid.
     * @param lineIndex the line.
     */
    SpecialCell getSpecialCell(bool speedTrack, int position, int lineIndex) const noexcept;

    /**
     * Sets the name of a Special Track.
     * @param positionIndex the position index. Must be valid.
     * @param speedTrack true is speed track, false if event track.
     * @param newName the new name.
     */
    void setSpecialTrackName(int positionIndex, bool speedTrack, const juce::String& newName) noexcept;

    /**
     * Inserts a Cell in a SpecialTrack.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the Special Track index. It must exist.
     * @param cellIndex the cell index.
     * @param specialCellToInsert the cell to insert. Can be an empty SpecialCell.
     * @return the Cell that has gone beyond the the track capacity.
     */
    SpecialCell insertSpecialCellAt(bool speedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCellToInsert) noexcept;

    /**
     * Removes a SpecialCell in a SpecialTrack, shifting "up" all the Cells after.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the Track index. It must exist.
     * @param cellIndex the cell index.
     * @param lastSpecialCell the SpecialCell to write at the very end.
     * @return the removed SpecialCell.
     */
    SpecialCell removeSpecialCellAt(bool speedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& lastSpecialCell) noexcept;

    /**
     * @return the link state, that is, if this Special Track is linked or linked to, or none.
     * Note that this browses all the Positions. You may not want to do that too often, even though it shouldn't be a problem most of the time.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param positionIndex the position index. Must be valid.
     */
    LinkState getSpecialTrackLinkState(bool speedTrack, int positionIndex) const noexcept;

    /**
     * @return where a Special Track is used.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the Special Track index.
     * @param linkType what to look for.
     */
    std::vector<SpecialTrackLocation> findSpecialTrackUse(bool speedTrack, int specialTrackIndex, LinkType linkType) const noexcept;


    // Cells.
    // ==========================================================

    /**
     * @return a Cell. This call is safe: if the position isn't valid, an empty Cell is returned.
     * @param position the position. May be invalid.
     * @param line the line.
     * @param channelIndex the channel index (may be >2). May be invalid.
     */
    Cell getCell(int position, int line, int channelIndex) const noexcept;

    /**
     * Stores a Cell. Notes that the target track may be a linked track.
     * @param position the position. May be invalid.
     * @param line the line.
     * @param channelIndex the channel index (may be >2). May be invalid.
     * @param cell the Cell to store.
     */
    void setCell(int position, int line, int channelIndex, const Cell& cell) noexcept;

    /**
     * Stores a Cell. Notes that the target track may be a linked track.
     * @param trackIndex the track index. Must be valid.
     * @param lineIndex the line index in the Track. Must be valid.
     * @param cell the Cell.
     */
    void setCellFromTrackIndex(int trackIndex, int lineIndex, const Cell& cell) noexcept;


    // Special Cells.
    // ==========================================================

    /**
     * Stores a cell. If invalid, nothing happens.
     * @param isSpeedTrack true if speed track, false if EventTrack.
     * @param position the position. May be invalid.
     * @param lineIndex the line. May be invalid.
     * @param specialCell the special cell to write.
     */
    void setSpecialCell(bool isSpeedTrack, int position, int lineIndex, const SpecialCell& specialCell) noexcept;

    /**
     * Stores a cell. If invalid, nothing happens.
     * @param isSpeedTrack true if speed track, false if EventTrack.
     * @param specialTrackIndex the speed track index. Must be valid.
     * @param lineIndex the line index in the Track. Must be valid.
     * @param specialCell the special cell to write.
     */
    void setSpecialCellFromTrackIndex(bool isSpeedTrack, int specialTrackIndex, int lineIndex, const SpecialCell& specialCell) noexcept;

private:

    /**
     * Shifts the loop start and/or end, according to where an insertion/deletion is done.
     * @param index where the insertion/deletion is done.
     * @param shiftCount how many positions are added/deleted. Positive if added, Negative if deleted.
     */
    void shiftStartLoopAndEnd(int index, int shiftCount) noexcept;

    const Id id;                                            // Uniquely identify the Subsong for this session. Not serialized.

    juce::String name;                                      // The name of the Subsong.
    std::vector<Psg> psgs;                                  // The PSG data (frequency, outputs, etc.).
    SidPlayerCapability sidPlayerCapability;                // The SID player capabilities (frequency, min/max period).

    int initialSpeed;                                       // The speed when starting the Subsong from the beginning (>0).
    float replayFrequencyHz;                                // Frequency in hz the song must be played (12.5, 25, 50...).
    int digiChannel;                                        // Channel index on which the drums should be played (>= 0).
    int highlightSpacing;                                   // The highlight spacing (>=1).
    int secondaryHighlight;                                 // The secondary highlight (>1), in unit of highlight.

    int loopStartPosition;                                  // The index of the pattern when looping.
    int endPosition;                                        // The index of the last pattern to play.

    std::vector<Pattern> patterns;                          // The Patterns. There is always at least one.
    std::vector<Position> positions;                        // The Positions. There is always at least one.
    std::vector<Track> tracks;                              // All the Tracks of the subsongs, used or not.
    std::vector<SpecialTrack> speedTracks;                  // All the SpeedTracks of the subsongs, used or not.
    std::vector<SpecialTrack> eventTracks;                  // All the EventTracks of the subsongs, used or not.
};

}   // namespace arkostracker

// Specialization of the std::hash for it to use ours.
// More help here: http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
template<>
struct std::hash<arkostracker::Subsong>
{
    std::size_t operator()(const arkostracker::Subsong& key) const noexcept
    {
        return static_cast<std::size_t>(key.getId().hash());
    }
};
