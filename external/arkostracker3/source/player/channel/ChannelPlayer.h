#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "../../song/Expression.h"
#include "../../song/cells/Cell.h"
#include "../../utils/FpFloat.h"
#include "../CellToPlay.h"
#include "../SongDataProvider.h"
#include "ChannelPlayerResults.h"

namespace arkostracker 
{

class SidPlayerCapability;
class SongDataProvider;

// TODO Continue TU the player!
/**
 * Takes care of generating the PSG stream of *one* Channel (out of the three a PSG has), with a Cell as a base data.
 *
 * To enforce decoupling, the Song itself is not known at all. This class asks for data from a provider class.
 *
 * In order to manage multi-threading, a Cell is "posted" in a queue. This is needed so that fastly sent Cells don't overwrite each other.
 * All the Cells in the queue is read when the stream is played (i.e. there is no delay between Cells).
 *
 * However, this class does not manage multi-threading at all.
 */
class ChannelPlayer
{
public:
    /**
     * Constructor.
     * @param songDataProvider to get Song data without knowing anything about it.
     * @param channelIndex the channel index in the Song (0, 1, 2, or more).
     */
    ChannelPlayer(SongDataProvider& songDataProvider, int channelIndex) noexcept;

    /**
     * Posts a Cell in the queue. It will be played when the stream is read, after all the possible queued Cells are read.
     * This is called from a method called from the Audio Thread! May be called from the UI thread in a non-threaded environment
     * (YM/WAV generation for example).
     * @param cellToPlay the Cell to play, also includes a location to force the getting of the right pitch/arp/volume when the Cell is played (for the "Effect context" feature).
     */
    void postCell(const CellToPlay& cellToPlay) noexcept;

    /**
     * Posts a "stop" sound (an RST, with R effect).
     * This is called from a method called from the Audio Thread! May be called from the UI thread in a non-threaded environment
     * (YM/WAV generation for example).
     */
    void postStopSound() noexcept;

    /**
     * Plays the stream and modifies the internal states of this Channel, and returns PSG registers for the channel.
     * This reads the new posted Cells, if there are.
     * The tick parameters are useful to:
     * - Know if effects must be read (only on first tick).
     * - Know if effects based on time (pitch slide, volume slide) must continue (if tick inferior or equal to speed).
     * This is useful when listening to one line (we want these effects to be compliant to the real song, even if the
     * user stays 10 seconds on a line).
     * This is called from a method called from the Audio Thread! May be called from the UI thread in a non-threaded environment
     * (YM/WAV generation for example).
     * @param isFirstTick true if we are on the first tick of the line.
     * @param stillWithinLine true if the current tick is still inside the line (line-based effects can still evolve).
     * @return the registers.
     */
    std::unique_ptr<ChannelPlayerResults> playStream(bool isFirstTick, bool stillWithinLine) noexcept;

    /**
     * @return the results of this ChannelPlayer. Especially useful for generators.
     * Call this after having played one tick to get new values.
     */
    std::unique_ptr<ChannelPlayerResults> getResults() const noexcept;

private:
    /**
     * Reads the given Cell and sets the internal states of this Channel.
     * @param cell the Cell to read.
     */
    void readCellAndSetStates(const Cell& cell) noexcept;

    /**
     * Manages the effects, such as arpeggio, pitches, glide, volume, etc.
     * @param stillWithinLine true if the current tick is still inside the line (line-based effects can still evolve).
     */
    void manageTrailingEffects(bool stillWithinLine) noexcept;

    /** Calculates the next values for the "trailing effect" pitch. Updates the internal variables. */
    void manageTrailingPitch() noexcept;
    /** Calculates the next values for the "trailing effect" volume. Updates the internal variables. */
    void manageTrailingVolume() noexcept;

    /** Plays a frame of the current instrument. */
    void playInstrumentFrame() noexcept;

    /**
     * Reads the given effects and updates the internal values.
     * This should be called only when a new Cell is received.
     * @param effects the effects of the cells. They SHOULD have been normalized and corrected before.
     */
    void manageNewCellEffects(const CellEffects& effects) noexcept;

    /**
     * Manages the "set volume" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectSetVolume(int effectValue) noexcept;

    /**
     * Manages the "volume in" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectVolumeIn(int effectValue) noexcept;

    /**
     * Manages the "volume out" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectVolumeOut(int effectValue) noexcept;

    /**
     * Manages the "pitch up" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectPitchUp(int effectValue) noexcept;
    /**
     * Manages the "fast pitch up" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectFastPitchUp(int effectValue) noexcept;

    /**
     * Manages the "pitch down" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectPitchDown(int effectValue) noexcept;
    /**
     * Manages the "fast pitch down" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectFastPitchDown(int effectValue) noexcept;

    /**
     * Manages the "pitch table" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectPitchTable(int effectValue) noexcept;

    /**
     * Manages the "reset" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectReset(int effectValue) noexcept;

    /**
     * Manages the "force Instrument speed" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectForceInstrumentSpeed(int effectValue) noexcept;

    /**
     * Manages the "force Arpeggio speed" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectForceArpeggioSpeed(int effectValue) noexcept;

    /**
     * Manages the "force Pitch speed" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectForcePitchTableSpeed(int effectValue) noexcept;

    /**
     * Manages the "Arpeggio 3 notes" effect (only two are shown). Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectArpeggio3Notes(int effectValue) noexcept;

    /**
     * Manages the "Arpeggio 4 notes" effect (only three are shown). Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectArpeggio4Notes(int effectValue) noexcept;

    /**
     * Manages the "Arpeggio table" effect. Updates the internal variables.
     * @param effectValue the value.
     */
    void manageEffectArpeggioTable(int effectValue) noexcept;

    /** Plays a frame of the current PSG instrument. */
    void playPsgInstrumentFrame() noexcept;

    /** Plays a frame of the current Sample instrument. Warning, this is called from within an Instrument lock, so be as quickly as possible. */
    void playSampleInstrumentFrame() noexcept;

    /**
     * Resets the volume, arpeggios, pitch, etc. Useful when playing a pattern from start for example, or when a Reset effect occurs.
     * @param invertedVolume the new volume. Warning, it is inverted (0 is full, 15 is silence!).
     */
    void resetEffects(int invertedVolume = 0) noexcept;

    /** Fully resets the direct Arpeggio (not only the data (one empty element is still present), but also the speed). */
    void resetInlineArpeggio() noexcept;

    /** Makes the Arpeggio being played from the start. */
    void resetPositionOnArpeggio() noexcept;

    /** Uses an empty Pitch Table, so that no Pitch Table is heard. */
    void resetPitchTable() noexcept;

    /** Makes the Pitch Table being played from the start. */
    void resetPositionOnPitchTable() noexcept;

    /**
     * Updates the internal variables about the current Arpeggio, whether it is Inline or Table, from the Song.
     * This should be done on every advance, to take the possible changes in account.
     * @param resetSpeed if true, possible forced speed is reset, so that the "normal" speed is used.
     * This can be set to true when a new sound is started, for example.
     */
    void updateArpeggioMetadata(bool resetSpeed = false) noexcept;

    /**
     * Updates the internal variables about the current Pitch, from the Song.
     * This should be done on every advance, to take the possible changes in account.
     * @param resetSpeed if true, possible forced speed is reset, so that the "normal" speed is used.
     * This can be set to true when a new sound is started, for example.
     */
    void updatePitchMetadata(bool resetSpeed = false) noexcept;

    /**
     * Reads the current Arpeggio and returns the note to add to the base note.
     * Besides that, advances inside the Arpeggio.
     * @return the Arpeggio value.
     */
    int readArpeggioAndAdvance() noexcept;
    /** @return the current Arpeggio value. The Arpeggio can be inline or not. */
    int readArpeggioValue() const noexcept;

    /**
     * Reads the current Pitch and returns the pitch to add to the base pitch.
     * Besides that, advances inside the Pitch.
     * @return the Pitch value.
     */
    int readPitchTableAndAdvance() noexcept;
    /** Reads the current Pitch value. */
    int readPitchValue() const noexcept;

    /**
     * Browses the (already normalized and optimized) effects, looking for a Glide. If one is found,
     * the Glide speed is set. Note that the Glide goal period is NOT managed here (a note must be read, plus a glide
     * may be found without note).
     * @param effects the effects. They SHOULD be normalized and optimized.
     * @return true if a Glide effect has been found.
     */
    bool browseEffectsForGlideAndSetGlideSpeed(const CellEffects& effects) noexcept;

    /** Calculates the next values for the Glide. Updates the internal variables. */
    void manageTrailingGlide() noexcept;

    /** Must be called when the Glide is over: this will modify the internal variables to reflect this. */
    void setGlideToComplete() noexcept;

    /**
     * Restricts the given software period (like an AND).
     * @param period the period. Will be modified.
     */
    static void restrictSoftwarePeriod(int& period) noexcept;

    /**
     * Restricts the given hardware period (like an AND).
     * @param period the period. Will be modified.
     */
    static void restrictHardwarePeriod(int& period) noexcept;

    /** @return the SamplePlayInfo for the current sample (if any). */
    SamplePlayInfo generateSamplePlayInfo() const noexcept;

    void applyEffectContext(const LineContext& lineContext) noexcept;

    /** @return a SpecialEffect from the current values. */
    SpecialEffect buildSpecialEffect() const noexcept;

    /**
     * Calculates the periods of a SID.
     * @param cell the cell to read from.
     * @param period the period of the sound.
     * @param psgFrequency the PSG frequency.
     * @param sidPlayerCapability the SID player capability.
     * @return the high-shelf and low-shelf periods of the SID.
     */
    static std::pair<int, int> calculateSidPeriods(const LowLevelPsgInstrumentCell& cell, int period, int psgFrequency, const SidPlayerCapability& sidPlayerCapability) noexcept;

    SongDataProvider& songDataProvider;                             // To get Song data without knowing anything about it.
    const int channelIndex;                                         // The channel index in the Song (0, 1, 2, or more).

    // -----------------------------------------------------
    std::mutex postedCellMutex;                                     // Mutex to make sure the Cell is accessed atomically, as it is posted by a non-audio thread.
    bool postedMustStopUiThread;                                    // Must be accessed via mutex. True to stop all sounds and clear the related variables.
                                                                    // Supersedes the posted Cells. Set to false once consumed.
    std::vector<CellToPlay> postedCellsUiThread;                    // The posted Cells. Must be accessed via mutex. Must be treated in order.

    // All the following data are only accessed in the Audio Thread.
    // -----------------------------------------------------

    OptionalInt newPlayedNote;          // Present if a new note has just been played.
    CellEffects newCellEffects;         // The effects declared in the read line.
    std::vector<Cell> newPostedCells;   // The played cells (should be only one in offline, possibly several in real time).

    std::shared_ptr<Sample> currentSample;  // Null if no sample is being played.
    bool isSampleRestarted;             // True if a sample is started from the beginning, when played. Should be immediately set to false when read.
    float currentSampleAmplification;
    Loop currentSampleLoop;
    int currentSampleFrequencyHz;
    int currentSampleVolume4Bits;       // The volume of the sample, from 0 (no sound), to 15. The track volume has been applied to it.
    float currentSamplePitchHz;         // The sample pitch, in Hz (how to play it). Takes the reference frequency in account, and the pitch/glide.
    float currentSampleReferenceFrequency;  // The reference frequency in Hz (440 for example).

    int baseNote;                       // The base note, as given by the Cell (with transposition). Only changes when a new note comes.
    int trackNote;                      // The track note, which derives from the base note, plus the arpeggio.

    int noise;                          // The noise, from 0 to 31 (0 means no noise).
    bool soundOpen;                     // True if the sound channel is open (heard).

    // The current track pitch period. It is added to the current base note period for pitch effects. Signed. Warning! The lower, the higher the note is.
    FpFloat trackPitch;
    FpFloat trackPitchSlide;            // Increases/decreases as pitch effects are used. Signed. Warning! The lower, the higher the note is.
    FpFloat trackVolume;                // The current volume (0-15) of the track.
    FpFloat trackVolumeSlide;           // How much the volume can be going up or down every tick. Signed.
    bool trackGlideActivated;           // True if the glide effect is still looking for a goal period. False on a new note, or when the goal is reached.
    uint16_t trackGlideGoalPeriod;           // The period the glide effect wants to reach.
    uint16_t trackGlideFinalPitchValue;      // The final pitch of the glide effect. Used once the final pitch has been reached.
    uint16_t trackGlideInitialPeriod;        // The period at the beginning of the glide effect.
    bool isTrackGlidePeriodIncreasing;  // Indicates the direction of the glide. True if the period increases (lower sound).
    int pitchFromPitchTable;            // The pitch read from the Pitch Table.

    OptionalId currentInstrumentId;
    int instrumentCellVolumeOrHardwareVolume;   // The volume read by the instrument (0-15, or 16 if the instrument uses hardware volume). Decreased by the Track volume.
    int instrumentCurrentLine;          // The line being read on the instrument.
    int instrumentCurrentTick;          // The tick on the current line on the current instrument.

    OptionalInt forcedInstrumentSpeed;  // If present, the speed is forced to this speed.

    int hardwareEnvelope;               // The current hardware envelope (0-15).
    bool hardwareRetrig;                // If true, the hardware envelope is re-triggered.
    int hardwarePeriod;                 // The current hardware period.
    int softwarePeriod;                 // The current software period. It derives from the baseNote BUT changes with the Arpeggios and Pitches.

    // The current inline Arpeggio. For simplicity, it is always used and emptied when not used.
    // Note that its speed is NOT used (except on importing the Arpeggio when reading a new effect), because it can be forced.
    Expression currentInlineArpeggio;
    bool useInlineArpeggio;             // True to use the Direct Arpeggio, false to use a Table Arpeggio.

    OptionalId currentTableArpeggioId;  // The ID of the used Table Arpeggio, if any.
    int currentArpeggioStepIndex;       // Index on the Arpeggio index being played. Note that it is not only used for direct Arpeggio, but for all!
    int currentArpeggioTick;            // Tick to know if the next step in the Arpeggio (direct or not) must be read.

    // The following are values that ARE actually used, because they can be from either the Direct Arpeggio or the Table Arpeggio.
    int currentArpeggioSpeed;           // Speed of the current Arpeggio, direct or not. It may be replaced by the forced speed!
    int currentArpeggioEndIndex;        // The end index of the current Arpeggio, direct or not.
    int currentArpeggioLoopStartIndex;  // The loop start index of the current Arpeggio, direct or not.
    int forcedArpeggioSpeed;            // If >=0, the speed is forced to this speed.

    OptionalId currentPitchId;          // The current ID of the used Pitch Table, if any.
    int currentPitchStepIndex;          // Index on the Pitch Table index being played.
    int currentPitchTick;               // Tick to know if the next step in the Pitch Table must be read.
    int currentPitchSpeed;              // The speed of the current Pitch Table. It may be replaced by the forced speed!
    int currentPitchEndIndex;           // The end index of the current Pitch Table.
    int currentPitchLoopStartIndex;     // The loop start index of the current Pitch Table.
    int forcedPitchSpeed;               // If >=0, the speed is forced to this speed.

    bool currentIsSidActivated;
    int currentSidHighShelfPeriod;
    int currentSidLowShelfPeriod;
    int currentSidLowShelfVolume;                  // From 0 to 15.
};

}   // namespace arkostracker
