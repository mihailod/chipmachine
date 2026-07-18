#pragma once

#include <array>
#include <unordered_map>
#include <unordered_set>

#include "../../song/cells/Effect.h"
#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

class SongBuilder;
class SubsongBuilder;
class TrailingEffectContext;

/** Class to import the Tracks from the .128. */
class SubsongImporter
{
public:
    /**
     * Constructor.
     * @param songBuilder the builder of the Song. Only useful to create the Instruments here.
     * @param subsongBuilder the builder of the Subsong, to send the events to. It has already been created.
     * @param musicData the music data to read from.
     * @param soundtrakkerInstrumentWithSFlagIndexes indexes (>0) of Soundtrakker Instruments with a "S" flag (full volume).
     */
    SubsongImporter(SongBuilder& songBuilder, SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData,
                    std::unordered_set<int> soundtrakkerInstrumentWithSFlagIndexes) noexcept;

    /** Starts parsing the music data. It will fill its own ErrorReport. */
    void parse() noexcept;

private:
    static constexpr auto channelCount = 3;

    //static const int maximumPosition = 92;                  // There are 92 positions at maximum.
    static constexpr auto minimumHardwarePeriodToPretendToBeARatio = 0x11;   // Lower, and we don't try to find a ratio.

    static constexpr auto offsetPatternsList = 0x0a30;           // Offset of the Linker (Pattern list).
    static constexpr auto offsetPatternTranspositions = 0x0b18;  // Offset of the transpositions for every Pattern.

    static constexpr auto offsetInitialSpeed = 0xb7b;            // Offset where the initial speed is encoded.
    static constexpr auto offsetPatternLoop = 0xb7c;             // Offset where the pattern loop is encoded is encoded.
    static constexpr auto offsetPatternLength = 0xb7d;           // Offset where the length of the patterns is encoded (64, 32, etc.).
    static constexpr auto offsetSongLength = 0xb7f;              // Offset where the length of the music is encoded (how many patterns).

    static constexpr auto patternLineSize = 3 * 3;               // Size of one line of Pattern data (3 tracks, 3 bytes for each).

    static constexpr auto hardwareRatioCount = 8;                // How many Ratios available.

    static constexpr auto volumeSlideMultiplier = 0x8;           // Number to multiply to the "volume up/down" effect before encoding it to AT format. The higher, the faster.
    static constexpr auto pitchSlideMultiplier = 0x18; // 0x30;  // Number to multiply to the "pitch up/down" effect before encoding it to AT format. The higher, the faster.

    /** Reads the Patterns and adds them, along with the related Tracks, if necessary. */
    void readAndCreatePatternsAndTracks() noexcept;

    /**
     * Extracts the transposition for the given position. If error, a warning is raised only. If outside of boundary, the transposition is limited.
     * @param position the position in the song, where to read the Pattern from.
     * @return the transposition.
     */
    int extractPatternTransposition(int position) const noexcept;

    /**
     * Reads and encodes a Track. As they are read, a SpeedTrack may be built using the current SpeedTrack index.
     * @param patternIndex the pattern index in the song, to be able to reach the right data.
     * @param channelIndex the channel index (0, 1, 2).
     * @param trackIndex the track index of the Track to create.
     * @param patternHeight set on input, may be modified if a "stop pattern" effect is found.
     * @return true if no critical error has been found.
     */
    bool readAndCreateTrack(int patternIndex, int channelIndex, int trackIndex, int& patternHeight) noexcept;

    /**
     * Manages the special case of an instrument with a forced volume ("S" flag). May add a volume effect, unless another volume effect
     * is used. Nothing happens if the note and instrument are not present.
     * @param noteAndOctave the note and octave, from 0 to 95, or -1 if not present.
     * @param soundtrakkerInstrumentIndex the instrument index, or -1 if not present. Warning, this is Soundtrakker index (0-15), not AT!
     * @param effectCommand the possible effect command. It may be present or not, or even invalid or unknown.
     * @param trailingEffectContext the context of the effects for this Track.
     * @param hasVolumeBeenForcedOut output value. If true, the volume has been forced, so the current volume should be encoded in the next note.
     */
    void manageForcedInstrumentVolumeEffect(int noteAndOctave, int soundtrakkerInstrumentIndex, int effectCommand, TrailingEffectContext& trailingEffectContext,
                                            bool& hasVolumeBeenForcedOut) const noexcept;

    /**
     * Indicates whether a Soundtrakker Instrument has a "S" flag (forced volume to full).
     * This can be called ONLY after all the Instruments has been parsed.
     * @param instrumentIndex the index of the Soundtrakker Instrument (0-15). NOT the AT index!
     * @return true if the Instrument has a "S" flag.
     */
    bool doesInstrumentForcesVolume(int instrumentIndex) const noexcept;

    /**
     * Manages a possible effect command. It may be present or not, or even invalid or unknown.
     * @param noteAndOctave the note and octave, from 0 to 95, or -1 if not present.
     * @param effectCommand the possible effect command. It may be present or not, or even invalid or unknown.
     * @param effectParameter the possible 8-bit effect parameter.
     * @param trackIndex the Track index. Only useful to deliver accurate error messages.
     * @param lineIndex the line index. Only useful to deliver accurate error messages.
     * @param wasPitchPresentOut true if a pitch effect (up/down) was present in the previous Cell. This is a bit of a hack to manage the stopping of
     *        the pitches if needed. The value is modified!
     * @param wasHardSoundOut true if the previous note was a hard sound. Modified if needed to update.
     * @param hasVolumeBeenForced true if the volume has been previously forced by a "S" Instrument. We should try to encode the "real" current volume.
     * @param patternHeight the current pattern height. May be modified if a Break pattern is found.
     * @param trailingEffectContext holds a context for this Track.
     */
    void manageReadEffect(int noteAndOctave, int effectCommand, int effectParameter,
                          int trackIndex, int lineIndex, bool& wasPitchPresentOut, bool& wasHardSoundOut, bool hasVolumeBeenForced,
                          int& patternHeight, TrailingEffectContext& trailingEffectContext) noexcept;

    /**
     * Manages the volume effect (0xb).
     * @param effectParameter the 8-bit effect parameter.
     * @param trailingEffectContext the context of this track.
     */
    void manageVolumeEffect(int effectParameter, TrailingEffectContext& trailingEffectContext) const noexcept;

    /**
     * Adds the effect which number is given. It is possible to discard the effect
     * if the value is zero.
     * @param effect the effect.
     * @param logicalValue the value. No need to pad or shift.
     * @param dontEncodeIfZero if true, don't encode the effect if the value is zero (after truncating).
     */
    void addEffect(Effect effect, int logicalValue, bool dontEncodeIfZero = false) const noexcept;

    /**
     * Manages the reset effect (0x1). Also includes an inverted volume.
     * @param effectParameter the 8-bit effect parameter.
     */
    void manageResetEffect(int effectParameter) const noexcept;

    /**
     * Manages the "pitch down" effect (0x2).
     * @param effectParameter the 8-bit effect parameter.
     */
    void managePitchDownEffect(int effectParameter) const noexcept;

    /**
     * Manages the "pitch up" effect (0x3).
     * @param effectParameter the 8-bit effect parameter.
     */
    void managePitchUpEffect(int effectParameter) const noexcept;

    /**
     * Manages the "volume down" effect (0x4).
     * @param effectParameter the 8-bit effect parameter.
     */
    void manageVolumeDownEffect(int effectParameter) const noexcept;

    /**
     * Manages the "volume up" effect (0x5).
     * @param effectParameter the 8-bit effect parameter.
     */
    void manageVolumeUpEffect(int effectParameter) const noexcept;

    /**
     * Manages the "arpeggio with volume" effect (0x6).
     * @param effectParameter the 8-bit effect parameter.
     * @param trailingEffectContext the context of the effects of this Track.
     */
    void manageArpeggioWithVolumeEffect(int effectParameter, TrailingEffectContext& trailingEffectContext) const noexcept;

    /**
     * Encodes a generic Arpeggio, from its number.
     * @param arpeggioNumber the arpeggio number, as in the Soundtrakker song (0-0xf). Must be valid.
     */
    void encodeArpeggio(int arpeggioNumber) const noexcept;

    /**
     * Manages the arpeggio effect (0xf).
     * @param effectParameter the 8-bit effect parameter.
     */
    void manageArpeggioEffect(int effectParameter) const noexcept;

    /**
     * Manages an inline arpeggio effect (0xe).
     * @param effectParameter the 8-bit effect parameter.
     */
    void manageDirectArpeggio(int effectParameter) const noexcept;

    /**
     * Manages the break effect/instrument delay (0x9).
     * @param effectParameter the 8-bit effect parameter.
     * @param lineIndex the line index where the effect happened.
     * @param patternHeight the current height. May be modified by this method.
     */
    void manageBreakOrInsDelayEffect(int effectParameter, int lineIndex, int& patternHeight) const noexcept;
    /**
     * Manages the Speed (delay) effect (0xd).
     * @param effectParameter the 8-bit effect parameter.
     * @param lineIndex the line index where the effect happened.
     */
    void manageSpeedEffect(int effectParameter, int lineIndex) noexcept;

    /**
     * Manages a hard effect command (0x8/0xa).
     * @param noteAndOctave the note and octave, from 0 to 95, or -1 if not present.
     * @param isCurve8 true if the curve is the 0x8. 0xA otherwise.
     * @param effectParameter the 8-bit effect parameter (in this case, the hardware period).
     * @param trackIndex the Track index. Only useful to deliver accurate error messages.
     * @param lineIndex the line index. Only useful to deliver accurate error messages.
     * @param wasHardSound true if the previous note was a hard sound.
     */
    void manageHardEffect(int noteAndOctave, bool isCurve8, int effectParameter, int trackIndex, int lineIndex, bool wasHardSound) noexcept;

    /**
     * Creates one empty Speed and Event Tracks in the current Subsong.
     */
    void createEmptySpecialTracks() const noexcept;

    SongBuilder& songBuilder;
    SubsongBuilder& builder;
    const juce::MemoryBlock& musicData;

    int positionsHeight;
    int songEndIndex;
    bool hasSpeedBeenFoundInPattern;                // True if a Speed has been found in the current Pattern. Useful to know if a SpeedTrack must be created.
    int nextSpeedTrackIndex;                        // The next or current Speed Track index. Starts at 1, as the 0 (empty) is created by default.

    std::array<int, static_cast<unsigned int>(hardwareRatioCount)> hardwareRatioToInstrumentIndexWithHard8; // Indicates whether an instrument exist (>0) for the given ratio, using the hardware curve 0x8.
    std::array<int, static_cast<unsigned int>(hardwareRatioCount)> hardwareRatioToInstrumentIndexWithHardA; // Indicates whether an instrument exist (>0) for the given ratio, using the hardware curve 0xa.
    std::unordered_map<int, int> hardwarePeriodToInstrumentIndexWithHard8;  // Indicates whether an instrument exist for the given hardware period, using the hardware curve 0x8.
    std::unordered_map<int, int> hardwarePeriodToInstrumentIndexWithHardA;  // Indicates whether an instrument exist for the given hardware period, using the hardware curve 0xa.

    std::unordered_set<int> soundtrakkerInstrumentWithSFlagIndexes;   // Indexes of Soundtrakker Instruments with a "S" flag (full volume).
};

}   // namespace arkostracker
