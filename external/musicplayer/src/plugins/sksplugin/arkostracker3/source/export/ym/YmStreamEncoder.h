#pragma once

#include <memory>

#include "../../player/PsgRegisters.h"
#include "../../song/Song.h"

namespace arkostracker 
{

/**
 * Abstract class to the encode the data of the YM (the registers themselves, not the metadata).
 *
 * It is the responsibility of the subclass to call the isCancelled() method to know if the job must be stopped!
 * However, the "cancelled" state is not stored here, but by a possible enclosing Task.
 *
 * Terminology:
 * - A "digidrum" is a sample, as encoded in the output YM, so it is 0-based and has no holes.
 * - A "sample" is a sample, as stored in an AT3 song. There may be holes in the list.
 */
class YmStreamEncoder
{
public:
    /** Destructor. */
    virtual ~YmStreamEncoder() = default;

    /**
     * Constructor.
     * @param song the Song to read.
     * @param subsongId the ID of the Subsong to read. Must be valid.
     * @param psgIndex the index of the unique PSG to get the data from. Must be valid.
     * @param isYm3 true if YM3, false if YM6.
     * @param targetPsgFrequency the possibly forced target PSG frequency.
     */
    YmStreamEncoder(std::shared_ptr<const Song> song, Id subsongId, int psgIndex, bool isYm3, OptionalInt targetPsgFrequency) noexcept;

    /**
     * Encodes the given PSG registers to the given output stream. As only 14 registers are given, registers 14 and 15 are forged.
     * @param outputStream the stream to write to.
     * @param psgRegisters the registers to encode.
     * @param previousHardwareEnvelope the previous Hardware envelope (0-15). Useful to know if the envelope PSG Register must be encoded because of a change, or not.
     */
    static void encodeRegisters(juce::OutputStream& outputStream, const PsgRegisters& psgRegisters, int previousHardwareEnvelope) noexcept;

    /** Encodes a byte in the given output stream. */
    static void encodeByte(juce::OutputStream& outputStream, int value) noexcept;

    /** @return how many digidrums there are. This must be calked AFTER the music is generated. */
    int getDigidrumCount() const noexcept;

    /**
     * Encodes the Digidrums, if any.
     * @param outputStream the stream to fill.
     */
    void encodeDigidrums(juce::OutputStream& outputStream) const noexcept;

    /**
     * Generates the stream (the YM data only, not the metadata).
     * @param outputStream the stream to fill.
     * @param iterationCount how many iterations there are.
     * @param startLocation the start location to play from.
     * @param pastEndLocation the past-end location to play to.
     * @param digiChannel the digi-channel.
     */
    virtual void generateStream(juce::OutputStream& outputStream, int iterationCount, const Location& startLocation, const Location& pastEndLocation,
                                int digiChannel) noexcept = 0;

protected:
    /**
     * Fills the given psg registers with sample data, if any.
     * @param psgRegisters the registers. May be modified.
     * @param digiChannel the digiChannel (>=0).
     */
    void fillPsgRegistersWithDigidrumData(PsgRegisters& psgRegisters, int digiChannel) noexcept;

    const std::shared_ptr<const Song> song;
    const Id subsongId;
    const int psgIndex;
    const bool isYm3;
    const OptionalInt targetPsgFrequencyHz;

    std::unordered_map<int, int> eventToYmDigidrum;     // Maps an event to a digidrum encoded in the YM.
    std::vector<int> sampleIndexes;                     // The index of the samples to encode.
};

}   // namespace arkostracker
