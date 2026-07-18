#pragma once

#include "../../../../song/subsong/Subsong.h"
#include "../../../../utils/ErrorReport.h"

namespace arkostracker 
{

/** Builder of a Subsong, used by the SongBuilder. It has its own ErrorReport, which should be added to the SongBuilder (by itself). */
class SubsongBuilder
{
public:

    /**
     * Constructor.
     * @param isNative true only if AT3, false for any other construction (from import for example). This is only to create one additional data,
     * the "start" marker on the first position.
     */
    explicit SubsongBuilder(bool isNative) noexcept;

    /**
     * Sets the metadata of the Subsong.
     * @param title the title of the Subsong.
     * @param initialSpeed the initial speed (>0).
     * @param replayFrequencyHz the replay frequency in hz (50 for example).
     * @param digiChannel the digichannel (>=0).
     * @param highlightSpacing the highlight spacing.
     * @param secondaryHighlight the secondary highlight.
     */
    void setMetadata(juce::String title, int initialSpeed = 6, float replayFrequencyHz = 50.0F, int digiChannel = 1, int highlightSpacing = 4, int secondaryHighlight = 4) noexcept;

    /** Sets the loop. */
    void setLoop(int startIndex, int endIndex) noexcept;

    /**
     * Adds a PSG.
     * @param psg the PSG.
     */
    void addPsg(const Psg& psg) noexcept;

    /** Sets the SID player capabilities. */
    void setSidPlayerCapability(const SidPlayerCapability& sidPlayerCapability) noexcept;

    /**
     * Builds the Subsong from all the previous actions.
     * @return the Song. It may be null, if the caller didn't fill the parameters correctly.
     */
    std::unique_ptr<Subsong> buildSubsong() noexcept;

    /** @return the Error Report. */
    const ErrorReport& getErrorReport() const noexcept;

    /**
     * Adds a warning line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addWarning(const juce::String& text, OptionalInt lineNumber = { }) noexcept;

    /**
     * Adds an error line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addError(const juce::String& text, OptionalInt lineNumber = { }) noexcept;

    /** @return true if there are no critical errors in the report. */
    bool isOk() const noexcept;
    /** @return true if there are any critical errors in the report. */
    bool areErrors() const noexcept;


    // Positions.
    // ==============================================

    /** Starts a new Position. */
    void startNewPosition() noexcept;
    /** Sets the height of the current Position. */
    void setCurrentPositionHeight(int newHeight) noexcept;
    /**
     * Sets the transposition to a channel of the current position.
     * @param channelIndex the channel index (>0).
     * @param transposition the transposition.
     */
    void setCurrentPositionTransposition(int channelIndex, int transposition) noexcept;
    /** Sets the Pattern index to the current position. It may not have been created before. */
    void setCurrentPositionPatternIndex(int patternIndex) noexcept;
    /** Stores the current Position. */
    void finishCurrentPosition() noexcept;

    /**
     * A shortcut to add positions directly. This is useful for importers which has their Positions directly, or use another way of reading them.
     * @param positions the positions.
     */
    void addPositions(const std::vector<Position>& positions) noexcept;


    // Patterns.
    // ==============================================

    /**
     * Starts a new pattern, which index is given. Warning! Check first if the checkRawOriginalPatternAndStoreIfDifferent is relevant.
     * If yes, make sure the startNewPattern ALWAYS uses the pattern index returned by this check method, and that no pattern index is "hand made".
     */
    void startNewPattern(int patternIndex) noexcept;
    /** Sets the track track index to a channel of the current Pattern. The track may not exist yet. */
    void setCurrentPatternTrackIndex(int channelIndex, int trackIndex) noexcept;
    /** Sets the speed track index in the current Pattern. The track may not exist yet. */
    void setCurrentPatternSpeedTrackIndex(int speedTrackIndex) noexcept;
    /** Sets the event track index in the current Pattern. The track may not exist yet. */
    void setCurrentPatternEventTrackIndex(int eventTrackIndex) noexcept;
    /** Stores the current Pattern. The channels must have been all filled. */
    void finishCurrentPattern() noexcept;

    /**
     * A shortcut to add Patterns directly. This is useful for importers which has their Patterns directly, or use another way of reading them.
     * @param patterns the Patterns.
     */
    void addPatterns(const std::vector<Pattern>& patterns) noexcept;

    /**
     * Sets the speed track index to an *already added* Pattern (start/finish). Useful to some importers that knows the Speed Track afterwards.
     * @param patternIndex the pattern index. It MUST exist.
     * @param speedTrackIndex the index of the Speed Track.
     */
    void setPatternSpeedTrackIndex(int patternIndex, int speedTrackIndex) noexcept;

    /**
     * This is useful when using formats that don't manage pattern at all (STarKos, AT1, AT2). When creating a Pattern, it is more user friendly
     * to avoid creating duplicates Patterns. Before calling StartNewPattern, create a "fake" pattern with the original data, and it will return
     * a pattern index to use.
     * This method stores the pattern in a separate set to know which patterns have been given to this method.
     * @param pattern the pattern.
     * @return true if new, false if old, plus the pattern index (new or old).
     */
    std::pair<bool, int> checkRawOriginalPatternAndStoreIfDifferent(const Pattern& pattern) noexcept;


    // Tracks.
    // ==============================================

    /** Starts a new Track. It must NOT be present already! Only one Track can be created at a time. */
    void startNewTrack(int trackIndex) noexcept;
    /** Stores the current Track. */
    void finishCurrentTrack() noexcept;

    /**
     * Short-cut for setting a Cell on a Track. It may not exist yet. It must NOT be the currently selected Track, else
     * bugs will appear!! It must NOT be used on a Track that WILL be created afterwards!
     * This should mostly be used for imports that are not linear (such as MIDI), or for native.
     * @param trackIndex the track index.
     * @param cellIndex the cell index.
     * @param cell the Cell.
     */
    void setCellOnTrack(int trackIndex, int cellIndex, const Cell& cell) noexcept;

    /**
     * Shortcut to set a read-only flag and name to a track. It may not exist yet. It must NOT be the currently selected Track.
     * @param trackIndex the track index.
     * @param readOnly true if read-only.
     * @param name the name.
     * @param argbColor the ARGB color, or 0 if there is none.
     */
    void setTrackMetadata(int trackIndex, bool readOnly, const juce::String& name, juce::uint32 argbColor) noexcept;


    // Special Tracks.
    // ==============================================

    /** Starts a new Special Track. It must NOT be present already. Only one SpecialTrack of its kind can be created at a time. */
    void startNewSpecialTrack(bool isSpeedTrack, int specialTrackIndex) noexcept;
    /** Sets the speed to a cell in the current Special Track. */
    void setCurrentSpecialTrackValue(bool isSpeedTrack, int cellIndex, int value) noexcept;
    /** Stores the current SpecialTrack. */
    void finishCurrentSpecialTrack(bool isSpeedTrack) noexcept;

    /**
     * Short-cut for setting a SpecialCell on a SpecialTrack. It may not exist yet. It must NOT be the currently selected SpecialTrack, else
     * bugs will appear!! It must NOT be used on a SpecialTrack that WILL be created afterwards!
     * This should mostly be used for imports that are not linear (such as MIDI), or for native.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the Special Track index.
     * @param cellIndex the cell index.
     * @param specialCell the SpecialCell.
     */
    void setSpecialCellOnTrack(bool isSpeedTrack, int specialTrackIndex, int cellIndex, const SpecialCell& specialCell) noexcept;

    /**
     * Shortcut to set a read-only flag and name to a special track. It MUST already exist!
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialTrackIndex the Special Track index.
     * @param readOnly true if read-only.
     * @param name the name.
     * @param argbColor the ARGB color, or 0 if there is none.
     */
    void setSpecialTrackMetadata(bool isSpeedTrack, int specialTrackIndex, bool readOnly, const juce::String& name, juce::uint32 argbColor) noexcept;


    // Speed Tracks.
    // ==============================================

    /** Starts a new SpeedTrack. It must NOT be present already! Only one SpeedTrack can be created at a time. */
    void startNewSpeedTrack(int speedTrackIndex) noexcept;
    /** Sets the speed to a cell in the current Speed Track. */
    void setCurrentSpeedTrackValue(int cellIndex, int speed) noexcept;
    /** Stores the current SpeedTrack. */
    void finishCurrentSpeedTrack() noexcept;

    /**
     * Sets a speed to a Speed Track, existing or not. Handy short-cut when dealing with non-linear format.
     * This must NOT be used when the same SpeedTrack is started!
     * @param speedTrackIndex the speed track index.
     * @param cellIndex the cell index.
     * @param speed the speed.
     */
    void setSpeedToTrack(int speedTrackIndex, int cellIndex, int speed) noexcept;


    // Event Tracks.
    // ==============================================

    /** Starts a new EventTrack. It must NOT be present already! Only one EventTrack can be created at a time. */
    void startNewEventTrack(int eventTrackIndex) noexcept;
    /** Sets the event to a cell in the current Event Track. */
    void setCurrentEventTrackValue(int cellIndex, int event) noexcept;
    /** Stores the current EventTrack. */
    void finishCurrentEventTrack() noexcept;


    // Cells.
    // ==============================================

    /** Starts a new Cell. Only one Cell should be created at at time. */
    void startNewCell(int cellIndex) noexcept;
    /** Stores the current Cell. */
    void finishCurrentCell() noexcept;

    /** Sets the note in the current Cell. */
    void setCurrentCellNote(int note) noexcept;
    /** Sets the instrument in the current Cell. It may not exist yet. */
    void setCurrentCellInstrument(int instrument) noexcept;
    /** Removes the instrument in the current Cell. */
    void removeCurrentCellInstrument() noexcept;
    /** Sets an RST in the current Cell. */
    void setCurrentCellRst() noexcept;
    /**
     * Adds an effect to the current Cell.
     * @param effect the Effect.
     * @param logicalValue the logical value (no need to shift).
     */
    void addCurrentCellEffect(Effect effect, int logicalValue) noexcept;

    /**
     * Sets an effect to the current Cell.
     * @param effectIndex the index where to put the effect.
     * @param effect the Effect.
     * @param rawValue the raw value.
     */
    void setCurrentCellEffectRawValue(int effectIndex, Effect effect, int rawValue) noexcept;

private:
    /**
     * Encodes all the Tracks. Holes will be filled. This must be called only when all the Tracks data is filled.
     * @param subsong the current Subsong.
     * @return true if no critical error was found.
     */
    bool encodeTracks(Subsong& subsong) noexcept;
    /**
     * Encodes all the Special Tracks. Holes will be filled
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsong the current Subsong.
     * @return true if no critical error was found.
     */
    bool encodeSpecialTracks(bool isSpeedTrack, Subsong& subsong) noexcept;

    /**
     * Encodes the Patterns. Holes will be filled. But Tracks/Special Tracks MUST have been declared in this Builder already.
     * @param subsong the current Subsong.
     * @return true if no critical error was found.
     */
    bool encodePatterns(Subsong& subsong) noexcept;

    /**
     * Encodes the Positions, and the loop. The related Patterns MUST have been declared in this Builder already.
     * @param subsong the current Subsong.
     * @return true if no critical error was found.
     */
    bool encodePositions(Subsong& subsong) noexcept;

    /** Checks that the Pattern list does not have any "hole". It it does, they are filled with any other Pattern. */
    void checkAndRepairHolesInPatterns() noexcept;
    /** Checks that all the Tracks/SpecialTracks that are in the Patterns actually exist, else create them with blank Tracks. */
    void createReferencedInPatternsButNotPresentTracks() noexcept;
    /**
     * Creates and adds a Special Track if it is not present in the given Pattern.
     * @param pattern the Pattern.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     */
    void createSpecialTrackIfNotPresentInPattern(const Pattern& pattern, bool isSpeedTrack) noexcept;

    /**
     * @return true if the given track exist (unlink/link if present).
     * @param trackIndexes the unlinked/linked (if present) indexes.
     * @param patternIndex the pattern index, for display.
     * @param typeName "Track", "Speed track", "Event Track".
     */
    bool checkTrackExist(const Pattern::TrackIndexAndLinkedTrackIndex& trackIndexes, int patternIndex, const juce::String& typeName) noexcept;

    /**
     * Sets the metadata of a Track?
     * @param track the Track.
     * @param readOnly true if read only.
     * @param name the name, or empty.
     * @param argbColor the color, or 0 if none.
     */
    static void fillTrackMetadata(Track& track, bool readOnly, const juce::String& name, juce::uint32 argbColor) noexcept;

    bool isNative;                                          // True if AT3.

    ErrorReport errorReport;

    juce::String title;
    int initialSpeed;                                       // The speed when starting the Subsong from the beginning (>0).
    float replayFrequencyHz;                                // Frequency in hz the song must be played (12.5, 25, 50...).
    int digiChannel;                                        // Channel number on which the drums should be played (>= 0).
    int highlightSpacing;                                   // The highlight spacing (>=1).
    int secondaryHighlight;                                 // The secondary highlight (>1), in unit of highlight.

    int loopStartIndex;
    int endIndex;

    std::vector<Psg> psgs;
    SidPlayerCapability sidPlayerCapability;

    int currentTrackIndex;
    Track currentTrack;
    std::map<int, Track> indexToTrack;                      // The stored Tracks.

    int currentSpeedTrackIndex;
    SpecialTrack currentSpeedTrack;
    std::map<int, SpecialTrack> indexToSpeedTrack;          // The stored SpeedTracks.

    int currentEventTrackIndex;
    SpecialTrack currentEventTrack;
    std::map<int, SpecialTrack> indexToEventTrack;          // The stored EventTracks.

    std::vector<Position> positions;                        // We consider they are added one by one.
    int currentPositionHeight;
    int currentPositionPatternIndex;
    juce::String currentPositionMarkerName;
    juce::uint32 currentPositionMarkerColor;
    std::unordered_map<int, int> currentPositionChannelToTransposition;     // Transposition to 0 are not encoded.

    int currentPatternIndex;
    std::map<int, int> currentPatternChannelIndexToTrack;   // They should all be set.
    int currentPatternSpeedIndex;
    int currentPatternEventIndex;
    juce::uint32 currentPatternColor;
    std::map<int, Pattern> indexToPattern;                  // The stored Patterns.
    std::map<Pattern, int> browsedRawOriginalPatternToIndex;

    int currentCellIndex;
    Cell currentCell;
};

}   // namespace arkostracker
