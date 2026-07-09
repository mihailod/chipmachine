#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../business/model/StartEnd.h"
#include "../utils/Id.h"
#include "ExpressionHandler.h"
#include "instrument/Instrument.h"
#include "subsong/Subsong.h"

namespace arkostracker 
{

/**
 * A Song.
 *
 * It is responsible for all its inner elements (Instruments, Expressions, Subsongs, etc.).
 *
 * It DOES lock its access to allow concurrent access.
 *
 * It does NOT handle observations on changes.
 * It does NOT manage redo/undo itself.
 * See the SongController for that.
 *
 * NOTE: Only the very used methods of set/get on a Subsongs are present. For the others,
 * use the "perform[const]onSubsong".
 */
class Song
{
public:
    friend class SongTestHelper;

    /**
     * Constructor.
     * @param name the name.
     * @param author the author.
     * @param composer the composer (for example "Rob Hubbard" for a cover).
     * @param comments the comments.
     * @param createDefaultData true to create default data (one subsong with a position, one instrument). False to create an invalid song without instrument and Subsong.
     * @param creationDate the creation date, in ms.
     * @param modificationDate the modification date, in ms.
     */
    Song(juce::String name, juce::String author, juce::String composer, juce::String comments,
         bool createDefaultData = false, juce::int64 creationDate = juce::Time::currentTimeMillis(), juce::int64 modificationDate = juce::Time::currentTimeMillis()) noexcept;

    /** Copy constructor. */
    Song(const Song&) noexcept;

    /**
     * @return a default Song, valid, with one Subsong and the empty Instrument.
     * @param addFirstInstrument true to create a "first" instrument (decreasing volume).
     */
    static std::unique_ptr<Song> buildDefaultSong(bool addFirstInstrument = false) noexcept;

    /**
     * @return the object that knows how to handle the Expressions.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    ExpressionHandler& getExpressionHandler(bool isArpeggio) noexcept;

    /**
     * @return the object that knows how to handle the Expressions, in read only.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     */
    const ExpressionHandler& getConstExpressionHandler(bool isArpeggio) const noexcept;

    /** @return the song title. */
    juce::String getTitle() const noexcept;
    /** @return the author. */
    juce::String getAuthor() const noexcept;
    /** @return the composer. */
    juce::String getComposer() const noexcept;
    /** @return the comments. */
    juce::String getComments() const noexcept;

    /**
     * @return the DigiChannel (>=0).
     * @param subsongId the ID of the Subsong. Should be valid.
     */
    int getDigiChannel(const Id& subsongId) const noexcept;

    /** Sets the title. */
    void setTitle(const juce::String& newTitle) noexcept;
    /** Sets the author. */
    void setAuthor(const juce::String& newAuthor) noexcept;
    /** Sets the composer. */
    void setComposer(const juce::String& newComposer) noexcept;
    /** Sets the comments. */
    void setComments(const juce::String& newComments) noexcept;

    /** @return the creation date, in ms. */
    juce::int64 getCreationDateMs() const noexcept;
    /** @return the modification date, in ms. */
    juce::int64 getModificationDateMs() const noexcept;

    /**
     * Forces the creation/modification dates to the given value. This is only really useful for testing.
     * @param creationDateMs the creation date in ms.
     * @param modificationDataMs the modification date in ms, or 0 to use the creation date.
     */
    void resetDatesMs(juce::int64 creationDateMs, juce::int64 modificationDataMs = 0) noexcept;

    /** Updates the modification date to now. */
    void updateModificationDate() noexcept;

    /**
     * @return how many channels there are (multiple of 3, >0).
     * @param subsongId the id of the Subsong. It must exist.
     */
    int getChannelCount(const Id& subsongId) const noexcept;

    /**
     * @return how many PSGs there are (>0).
     * @param subsongId the id of the Subsong. It must exist.
     */
    int getPsgCount(const Id& subsongId) const noexcept;

    /**
     * @return the replay frequency, in hz (50 for example).
     * @param subsongId the id of the Subsong. It must exist.
     */
    float getReplayFrequencyHz(const Id& subsongId) const noexcept;

    /**
     * @return the loop start/end position. The end will point on the last Cell of the end Position.
     * param subsongId the id of the Subsong. It must exist.
     */
    //StartEnd getLoopStartAndEndLocation(const Id& subsongId) const noexcept;

    /** Adds the empty instrument and expressions. Useful when creating the Song from scratch. */
    void addEmptyInstrumentAndExpressions() noexcept;


    // Subsongs.
    // ------------------------------------------------------------

    /** @return the names and ids of the Subsongs, in order. */
    std::vector<std::pair<juce::String, Id>> getSubsongNamesAndIds() const noexcept;

    /**
     * @return the metadata of a Subsong.
     * @param subsongId the Subsong ID. If invalid, default data is returned.
     */
    Subsong::Metadata getSubsongMetadata(const Id& subsongId) const noexcept;

    /**
     * @return the PSGs of a Subsong.
     * @param subsongId the Subsong ID. If invalid, empty data is returned.
     */
    std::vector<Psg> getSubsongPsgs(const Id& subsongId) const noexcept;

    /**
     * @return how many PSGs of a Subsong there are.
     * @param subsongId the Subsong ID. If invalid, 0 is returned.
     */
    int getSubsongPsgCount(const Id& subsongId) const noexcept;

    /** @return how many Subsongs there are (>0). */
    int getSubsongCount() const noexcept;

    /** @return true if the Subsong which ID is given exist. */
    bool doesSubsongIdExist(const Id& subsongId) const noexcept;

    /**
     * Performs an action on a given Subsong. All are locked during the process.
     * @param subsongId the id of the Subsong. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnSubsong(const Id& subsongId, const std::function<void(Subsong&)>& function) noexcept;
    /**
     * Performs an action on a given Subsong, without modifying it. All are locked during the process.
     * @param subsongId the id of the Subsong. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstSubsong(const Id& subsongId, const std::function<void(const Subsong&)>& function) const noexcept;

    /**
     * Adds a Subsong.
     * @param subsong the Subsong.
     */
    void addSubsong(std::unique_ptr<Subsong> subsong) noexcept;

    /**
     * Adds a Subsong.
     * @param subsongIndexToInsertAt where to insert (>=0). Must be valid.
     * @param subsong the Subsong.
     * @return true if success.
     */
    bool insertSubsong(int subsongIndexToInsertAt, std::unique_ptr<Subsong> subsong) noexcept;

    /**
     * Replaces a Subsong. It must exist!
     * @param subsongId the id of the Subsong to replace. Must be valid.
     * @param subsong the Subsong.
     */
    void replaceSubsong(const Id& subsongId, std::unique_ptr<Subsong> subsong) noexcept;

    /**
     * Deletes a Subsong. There SHOULD be at least one after that!
     * @param subsongId the id. Must be valid.
     * @return the deleted Subsong, or nullptr if not found, which shouldn't happen.
     */
    std::unique_ptr<Subsong> deleteSubsong(const Id& subsongId) noexcept;

    /** @return the ID of the first Subsong. Useful for some initialization. */
    Id getFirstSubsongId() const noexcept;

    /**
     * @return the Subsong Id that would be "replacing" if the Subsong, which id is given, would be deleted.
     * Could be the Subsong after, or the one before if it was the last one, or empty if there would be none anymore.
     * @param subsongId the Subsong ID that would be deleted.
     */
    OptionalValue<Id> getSubsongIdIfSubsongDeleted(const Id& subsongId) const noexcept;

    /** @return the Ids of the subsongs, in order. */
    std::vector<Id> getSubsongIds() const noexcept;

    /**
     * @return the Subsong id from its hash, or absent if not found.
     * @param idHash the hash of the id. Note that it is an int (and not a uint64), as it is sometimes used in ComboBox.
     */
    OptionalValue<Id> getSubsongIdFromHash(int idHash) const noexcept;

    /**
     * @return the subsong ID from the index of the Subsong, or absent if not found.
     * @param subsongIndex the Subsong index. Should be valid.
     */
    OptionalValue<Id> getSubsongId(int subsongIndex) const noexcept;

    /**
     * @return true if the the Subsong, which ID is given, exists.
     * @param subsongId the ID of the Subsong.
     */
    bool doesSubsongExist(const Id& subsongId) const noexcept;

    /**
     * @return the index of the Subsong which ID matching the given one. May not be found.
     * @param subsongId the ID of the Subsong. May be invalid.
     */
    OptionalInt getSubsongIndex(const Id& subsongId) const noexcept;

    /**
     * @return the loop start and past-end locations for a Subsong.
     * @param subsongId the id of the Subsong. It should exist.
     */
    std::pair<Location, Location> getLoopStartAndPastEndPositions(const Id& subsongId) const noexcept;


    // Instrument.
    // =======================================

    /** @return how many Instruments there are. */
    int getInstrumentCount() const noexcept;

    /**
     * @return the possible ID of the Instrument which index is given.
     * @param index the index. If invalid, empty is returned.
     */
    OptionalId getInstrumentId(int index) const noexcept;

    /** @return the ID of the first "real" Instrument, that is the "1", else the "0". */
    Id getFirstRealInstrumentId() const noexcept;
    /** @return the ID of the last Instrument. */
    Id getLastInstrumentId() const noexcept;

    /**
     * @return the possible index of the Instrument which ID is given.
     * @param instrumentId the ID. If not found, empty is returned.
     */
    OptionalInt getInstrumentIndex(const Id& instrumentId) const noexcept;

    /**
     * @return the IDs of all the instruments (including the 0th by default).
     * @param skip0th true to skip the 0th, false to get all.
     */
    std::vector<Id> getInstrumentIds(bool skip0th = false) const noexcept;

    /**
     * @return the Instrument ID, if found, from the given event.
     * @param event the event.
     */
    OptionalId findInstrumentIdFromEvent(int event) const noexcept;

    /**
     * @return the name of an instrument.
     * @param id the ID of the Instrument. If invalid, empty is returned.
     */
    juce::String getInstrumentName(const Id& id) const noexcept;

    /**
     * @return the name of an instrument from its index.
     * @param instrumentIndex the index of the instrument. If invalid, empty is returned.
     */
    juce::String getInstrumentNameFromIndex(int instrumentIndex) const noexcept;

    /** Builds a map linking the instrument to their color. */
    std::unordered_map<int, juce::uint32> buildInstrumentToColor() const noexcept;

    /**
     * Performs an action on a given const Instrument. All the Instruments are locked during the process.
     * @param id the ID of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstInstrument(const Id& id, const std::function<void(const Instrument&)>& function) const noexcept;

    /**
     * Performs an action on a given const Instrument. All the Instruments are locked during the process.
     * Even though using an index, this method can still be useful for objects manipulating only indexes, such as the player.
     * @param instrumentIndex the index of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstInstrumentFromIndex(int instrumentIndex, const std::function<void(const Instrument&)>& function) const noexcept;

    /**
     * Performs an action on a given non-const Instrument. All the Instruments are locked during the process.
     * @param id the ID of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnInstrument(const Id& id, const std::function<void(Instrument&)>& function) noexcept;

    /**
     * Performs an action on a given Instrument that we KNOW is a PSG Instrument. All the Instruments are locked during the process.
     * If the Instrument is NOT a PSG Instrument, an assertion is raised and the action is not done.
     * @param id the ID of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstPsgInstrument(const Id& id, const std::function<void(const Instrument&, const PsgPart& psgPart)>& function) const noexcept;

    /**
     * Performs an action on a given Instrument that we KNOW is a PSG Instrument. All the Instruments are locked during the process.
     * If the Instrument is NOT a PSG Instrument, an assertion is raised and the action is not done.
     * Even though using an index, this method can still be useful for objects manipulating only indexes, such as the player.
     * @param instrumentIndex the index of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstPsgInstrumentFromIndex(int instrumentIndex, const std::function<void(const Instrument&, const PsgPart& psgPart)>& function) const noexcept;

    /**
     * Performs an action on a given Instrument that we KNOW is a Sample Instrument. All the Instruments are locked during the process.
     * If the Instrument is NOT a Sample Instrument, an assertion is raised and the action is not done.
     * @param id the ID of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstSampleInstrument(const Id& id, const std::function<void(const Instrument&, const SamplePart& samplePart)>& function) const noexcept;

    /**
     * Performs an action on a given Instrument that we KNOW is a Sample Instrument. All the Instruments are locked during the process.
     * If the Instrument is NOT a Sample Instrument, an assertion is raised and the action is not done.
     * Even though using an index, this method can still be useful for objects manipulating only indexes, such as the player.
     * @param instrumentIndex the index of the Instrument. It must exist, but if not, nothing is done.
     * @param function the function to call.
     */
    void performOnConstSampleInstrumentFromIndex(int instrumentIndex, const std::function<void(const Instrument&, const SamplePart& samplePart)>& function) const noexcept;

    /**
     * Allows to perform an operation on the Instrument list. A lock is used during the whole operation.
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) noexcept;

    /**
     * Allows to perform an operation on the Instrument list. A lock is used during the whole operation.
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnConstInstruments(const std::function<void(const std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) const noexcept;

    /**
     * @return true if the instrument is read-only (i.e. it is the first one). If not found, returns true.
     * @param instrumentId the ID of the Instrument. May not exist.
     */
    bool isReadOnlyInstrument(const Id& instrumentId) const noexcept;

    /**
     * Adds an Instrument.
     * @param instrument the Instrument.
     * @return the index of the newly added Instrument.
     */
    int addInstrument(std::unique_ptr<Instrument> instrument) noexcept;

    /**
     * Removes an Instrument.
     * @param instrumentId the Instrument ID. Should be valid.
     */
    void removeInstrument(const Id& instrumentId) noexcept;


    // Cells.
    // =======================================

    /**
     * @return a Cell. This method is safe. If any element doesn't exist, an empty Cell is returned.
     * @param location the location. May be invalid.
     * @param channelIndex the channel index (may be >2).
     */
    Cell getCell(const Location& location, int channelIndex) const noexcept;

    /**
     * @return a SpecialCell. This method is safe. If any element doesn't exist, an empty Cell is returned.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param location the location. May be invalid.
     */
    SpecialCell getSpecialCell(bool speedTrack, const Location& location) const noexcept;


private:
    /**
     * @return the Subsong from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param subsongId the ID.
     */
    Subsong* findSubsong_noLock(const Id& subsongId) noexcept;

    /**
     * @return the Subsong from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param subsongId the ID.
     */
    const Subsong* findSubsong_noLock(const Id& subsongId) const noexcept;

    /**
     * @return the index of the Subsong, or absent if not found. This does NOT lock, so the caller MUST do it!
     * Long is used because it is the std type for iterator arithmetics.
     * @param subsongId the ID. May be absent.
     */
    OptionalLong findSubsongIndex_noLock(const Id& subsongId) const noexcept;

    /**
     * @return the Instrument from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param id the ID of the Instrument.
     */
    Instrument* findInstrument_noLock(const Id& id) noexcept;

    /**
     * @return the Instrument from its index, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param instrumentIndex the index of the Instrument.
     */
    const Instrument* findInstrumentFromIndex_noLock(int instrumentIndex) const noexcept;

    /**
     * @return the Instrument from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param id the ID of the Instrument.
     */
    const Instrument* findInstrument(const Id& id) const noexcept;

    mutable std::mutex songMutex;                               // Protects EACH access to all the Song members.
    juce::String name;
    juce::String author;
    juce::String composer;
    juce::String comments;
    juce::int64 creationDate;                                   // In epoch (ms).
    juce::int64 modificationDate;                               // In epoch (ms).

    ExpressionHandler arpeggios;                                // No need of a mutex, doesn't change. Locks its internal variable by itself.
    ExpressionHandler pitches;

    std::vector<std::unique_ptr<Subsong>> subsongs;
    mutable std::mutex subsongsMutex;                           // Protects EACH access to ALL the Subsongs.

    std::vector<std::unique_ptr<Instrument>> instruments;
    mutable std::mutex instrumentsMutex;                        // Protects EACH access to ALL the Instruments.

    JUCE_LEAK_DETECTOR (Song)

    // WARNING! The COPY CONSTRUCTOR must be updated if one field is added!
};

}   // namespace arkostracker
