#pragma once

#include "../../../../song/Song.h"
#include "../../../../utils/ErrorReport.h"
#include "SubsongBuilder.h"

namespace arkostracker 
{

/**
 * A handy class to build a Song without having to take care of linearly created Instruments, Expressions, etc.
 * This should be used by all the importers instead of creating a Song directly.
 * It holds an ErrorReport, so the importers don't have to manage one.
 *
 * - At the end, use the build() method to generate the song and the status report. This must only be called once.
 * - Instead of passing SubsongBuilder to specific classes, pass the SongBuilder and use the getCurrentSubsongBuilder() method.
 *
 * Contract:
 * - All the positions must be filled.
 * - There is no need to fill the holes in the indexes between and the Patterns, tracks, special tracks.
 * - No need to optimize the linker or the tracks. This is NOT done by this class, but an optimizer can be ran afterwards.
 * - At least one Subsong, and no holes in the Subsongs.
 * - After having declared the Subsong, adds the PSGs, before adding the Patterns, and they will rely on the PSGs count to check if channel indexes are out of bounds.
 */
class SongBuilder
{
public:
    /**
     * Constructor. One empty Subsong is created.
     * @param isNative true only if AT3, false for any other construction (from import for example). This is only to create one additional data,
     * the "start" marker on the first position.
     */
    explicit SongBuilder(bool isNative = false) noexcept;

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

    /** @return true if there are no critical errors in the report (also in the Subsong reports). */
    bool isOk() const noexcept;
    /** @return true if there are any critical errors in the report. */
    bool areErrors() const noexcept;

    /**
     * Sets the song basic data.
     * @param title the title.
     * @param author the author.
     * @param composer the composer.
     * @param comments the comments.
     * @param creationDate the creation date, in epoch (ms).
     * @param modificationDate the modification date, in epoch (ms).
     */
    void setSongMetaData(juce::String title, juce::String author, juce::String composer, juce::String comments, juce::int64 creationDate = juce::Time::currentTimeMillis(),
                         juce::int64 modificationDate = juce::Time::currentTimeMillis()) noexcept;

    /**
     * Builds the Song from all the previous actions. WARNING, the object becomes unusable after this method is called!
     * @return the Song, and the report. The Song may be null, if the caller didn't fill the parameters correctly.
     */
    std::pair<std::unique_ptr<Song>, std::unique_ptr<ErrorReport>> buildSong() noexcept;


    // Subsongs.
    // ==================================================

    /**
     * Starts the creation of a new Subsong. The slot must be free! Only one Subsong at a time can be created.
     * One of first thing to do is to add a PSG.
     * WARNING! Be sure to get a REFERENCE to the returned object, not a VALUE!
     * @return the SubsongBuilder to call to write on the Subsong.
     */
    SubsongBuilder& startNewSubsong(int subsongIndex) noexcept;
    /** Stores the current Subsong. */
    void finishCurrentSubsong() noexcept;

    /** @return the current Subsong Builder. MUST be called when one is currently open! */
    SubsongBuilder& getCurrentSubsongBuilder() noexcept;


    // Instruments.
    // ===============================================

    /** @return the index of an empty Instrument. */
    int findNextInstrumentIndex() noexcept;

    /** Starts the creation of a new PSG instrument. The slot must be free! Only one Instrument at a time can be created. */
    void startNewPsgInstrument(int instrumentIndex) noexcept;
    /** Starts the creation of a new Sample instrument. The slot must be free! Only one Instrument at a time can be created. */
    void startNewSampleInstrument(int instrumentIndex) noexcept;
    /**
     * Starts the creation of an instrument. The slot must be free! Only one Instrument at a time can be created.
     * @param instrumentIndex the index of the instrument. The slot must be free!
     * @param instrumentType the instrument type.
     */
    void startInstrument(int instrumentIndex, InstrumentType instrumentType) noexcept;

    /** Stores the current instrument. */
    void finishCurrentInstrument() noexcept;

    /**
     * Shortcut to set an Instrument directly, in case the caller already has them.
     * WARNING, this must NOT be a currently started instrument, else the new one will be overwritten when the started one is finished!
     * @param instrumentIndex the Instrument index.
     * @param instrument the Instrument.
     */
    void setInstrument(int instrumentIndex, std::unique_ptr<Instrument> instrument) noexcept;

    /**
     * Shortcut to set several Instrument directly, in case the caller already has them.
     * WARNING, this must NOT be a currently started instrument, else the new one will be overwritten when the started one is finished!
     * @param startIndex the index where to start the insertion (>=0).
     * @param instruments the Instruments. The items are going to be moved!
     */
    void setInstruments(int startIndex, std::vector<std::unique_ptr<Instrument>>& instruments) noexcept;

    /** Sets the name to the current instrument. Must be called AFTER the instrument is started. */
    void setCurrentInstrumentName(const juce::String& name) noexcept;
    /** Sets the color to the current instrument. Must be called AFTER the instrument is started. */
    void setCurrentInstrumentColor(const juce::uint32& argbColor) noexcept;

    /** Initializes the new PSG Instrument Cell with default values. */
    void startNewPsgInstrumentCell() noexcept;
    /** Pushes the current PSG Cell into the current Psg Instrument. The latter MUST have been initialized before! */
    void finishCurrentPsgInstrumentCell() noexcept;

    /** Sets the speed to the current PSG Instrument. Must be called AFTER the instrument is started. */
    void setCurrentPsgInstrumentSpeed(int speed) noexcept;
    /** Sets the exported flag of the sound effect, for the current PSG instrument. */
    void setCurrentPsgInstrumentSoundEffectExported(bool exported) noexcept;

    /** A convenient method, when the generator already has a full Cell, instead of using the more specific methods. */
    void setCurrentPsgInstrumentCell(const PsgInstrumentCell& cell);
    /** Sets the retrig to the current PSG Instrument. */
    void setCurrentPsgInstrumentRetrig(bool retrig) noexcept;
    /** Sets the loop to the current PSG Instrument. */
    void setCurrentPsgInstrumentMainLoop(int startIndex, int endIndex, bool isLooping);
    /** Sets the link to the current PSG cell. */
    void setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink link);
    /** Sets the volume to the current PSG cell. */
    void setCurrentPsgInstrumentCellVolume(int volume);
    /** Sets the noise to the current PSG cell. */
    void setCurrentPsgInstrumentCellNoise(int noise);
    /** Sets the primary period to the current PSG cell. */
    void setCurrentPsgInstrumentCellPrimaryPeriod(int period);
    /** Sets the primary arpeggio to the current PSG cell. */
    void setCurrentPsgInstrumentCellPrimaryArpeggio(int arpeggio);
    /** Sets the primary pitch to the current PSG cell. */
    void setCurrentPsgInstrumentCellPrimaryPitch(int pitch);
    /** Sets the primary period to the current PSG cell. */
    void setCurrentPsgInstrumentCellSecondaryPeriod(int period);
    /** Sets the secondary arpeggio to the current PSG cell. */
    void setCurrentPsgInstrumentCellSecondaryArpeggio(int arpeggio);
    /** Sets the secondary pitch to the current PSG cell. */
    void setCurrentPsgInstrumentCellSecondaryPitch(int pitch);
    /** Sets the ratio to the current PSG cell. */
    void setCurrentPsgInstrumentCellRatio(int ratio);
    /** Sets the hardware envelope to the current PSG cell. */
    void setCurrentPsgInstrumentCellHardwareEnvelope(int envelope);
    /** Sets the retrig to the current PSG cell. */
    void setCurrentPsgInstrumentCellRetrig(bool retrig);

    /** Sets the frequency in Hz to the current sample instrument. */
    void setCurrentSampleInstrumentFrequencyHz(int frequencyHz);
    /** Sets the loop to the current sample instrument. */
    void setCurrentSampleInstrumentLoop(const Loop& loop);
    /** Sets the volume ratio to the current sample instrument. */
    void setCurrentSampleInstrumentVolumeRatio(float volumeRatio);
    /**
     * Sets the sample to the current sample instrument.
     * @param sampleData8BitsUnsignedBase64 the sample data, 8 bits, unsigned, in base-64.
     */
    void setCurrentSampleInstrumentSample(const juce::String& sampleData8BitsUnsignedBase64);
    /**
     * Sets the sample to the current sample instrument.
     * @param sampleData8BitsUnsigned the sample data, 8 bits, unsigned.
     */
    void setCurrentSampleInstrumentSample(const juce::MemoryBlock& sampleData8BitsUnsigned);


    // Expressions.
    // ==================================================

    /** Starts the creation of a new Expression. The slot must be free! Only one Expression at a time can be created. A default name is given. */
    void startNewExpression(bool isArpeggio, int expressionIndex, const juce::String& title) noexcept;
    /** Stores the current Expression. */
    void finishCurrentExpression() noexcept;

    /** Sets the speed to the current Expression, which MUST have been initialized before. */
    void setCurrentExpressionSpeed(int speed) noexcept;
    /** Sets the loop to the current Expression, which MUST have been initialized before. As a convenience, it can be called even before adding the values. */
    void setCurrentExpressionLoop(int startIndex, int endIndex) noexcept;
    /** Adds a value to the current Expression, which MUST have been initialized before. */
    void addCurrentExpressionValue(int value) noexcept;

    /**
     * Shortcut to set several Expressions directly, in case the caller already has them.
     * WARNING, this must NOT be a currently started expression, else the new one will be overwritten when the started one is finished!
     * @param isArpeggio true if Arpeggios, false if Pitches.
     * @param startIndex the index where to start the insertion (>=0).
     * @param expressions the Expressions.
     */
    void setExpressions(bool isArpeggio, int startIndex, const std::vector<std::unique_ptr<Expression>>& expressions) noexcept;

    /**
     * @return true if the expression has been *declared*.
     * @param isArpeggio true if Arpeggios, false if Pitches.
     * @param expressionIndex the index of the expression.
     */
    bool doesExpressionExist(bool isArpeggio, int expressionIndex) const noexcept;

private:
    static const int sampleMaximumSizeInBytes;

    /**
     * Encodes the Subsongs.
     * @param song the current Song.
     * @return true if no critical errors were found.
     */
    bool encodeSubsongs(Song& song) noexcept;

    /**
     * Encodes the Instruments.
     * @param song the current Song.
     * @return true if no critical errors were found.
     */
    bool encodeInstruments(Song& song) noexcept;

    /**
     * Encodes the Expressions (either Arpeggios or Pitches).
     * @param song the current Song.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @return true if no critical errors were found.
     */
    bool encodeExpression(Song& song, bool isArpeggio) noexcept;

    bool isNative;                                          // True only for AT3.

    std::unique_ptr<ErrorReport> errorReport;               // The report.

    int currentSubsongIndex;                                // The Subsong index we're working on.
    int currentInstrumentIndex;                             // The index of the Instrument we're working on.
    int currentExpressionIndex;                             // The index of the Expression we're working on.

    std::unique_ptr<SubsongBuilder> currentSubsongBuilder;

    bool instrumentStarted;                                 // To make sure we don't build several instruments at the same time (or forgot to finish one).
    std::unique_ptr<PsgPart> currentPsgPart;
    InstrumentType currentInstrumentType;
    juce::String currentInstrumentTitle;
    juce::uint32 currentInstrumentArgbColor;
    int currentSampleInstrumentFrequencyHz;
    Loop currentSampleInstrumentLoop;
    float currentSampleInstrumentVolumeRatio;
    std::shared_ptr<Sample> currentSampleInstrumentSample;

    bool currentExpressionIsArpeggio;
    Expression currentExpression;
    Loop currentExpressionLoop;

    juce::String songTitle;
    juce::String author;
    juce::String composer;
    juce::String comments;
    juce::int64 creationDate;
    juce::int64 modificationDate;

    std::map<int, std::unique_ptr<SubsongBuilder>> subsongIndexToBuilder;               // Map linking a Subsong index to the Subsong Builder.
    std::map<int, std::unique_ptr<Instrument>> indexToInstrument;                       // Map linking an index to an Instrument.
    std::map<int, Expression> indexToArpeggio;                                          // Map linking an index to an Arpeggio.
    std::map<int, Expression> indexToPitch;                                             // Map linking an index to an Pitch.

    PsgInstrumentCell psgInstrumentCell;
};


}   // namespace arkostracker

