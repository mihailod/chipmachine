#pragma once

#include "../../../song/instrument/sample/Sample.h"
#include "../../../song/psg/PsgFrequency.h"

namespace arkostracker 
{

/** Simple holder of the metadata of the YM. We don't need all the data, so not all are retrieved. */
class YmMetadata
{
public:
    /** Constructor, with default values for Atari ST. */
    YmMetadata() :
            title(juce::String()),
            author(juce::String()),
            comments(juce::String()),
            psgMasterClockHz(static_cast<int>(PsgFrequency::psgFrequencyAtariST)),
            playerReplayHz(static_cast<int>(PsgFrequency::defaultReplayFrequencyHz)),
            iterationCount(0),
            loopIterationIndex(0),
            digidrums()
    {}

    int getDigidrumCount() const noexcept
    {
        return static_cast<int>(digidrums.size());
    }

    juce::String title;
    juce::String author;
    juce::String comments;
    /** The clock of the PSG, in Hz (2000000 for an Atari). */
    int psgMasterClockHz;
    /** The player replay, in Hz (50 most of the time), or -1 if it couldn't be found. */
    int playerReplayHz;
    int iterationCount;
    int loopIterationIndex;
    std::vector<std::shared_ptr<Sample>> digidrums;
};

/**
 * Abstract class to decode an YM, according to its format (YM2...YM6).
 */
class YmDecoder
{
public:
    /** Destructor. */
    virtual ~YmDecoder() = default;

    /**
     * Constructor.
     * @param inputStream the Stream of the YM.
     */
    explicit YmDecoder(juce::InputStream& inputStream) noexcept;

    /** True if the implementation accepts the header, and thus, can parse this file. */
    virtual bool acceptFormatHeader() noexcept = 0;

    /**
     * Parses the header and retrieves metadata. If the data are strange but can be repaired, the implementation must do it.
     * @param successOut set to true if the parsing went fine. False if the file is malformed/can not be read (critical).
     * @return the metadata, considered correct.
     */
    virtual YmMetadata parseHeader(bool& successOut) noexcept = 0;

    /**
     * Returns a value, regardless of the structure of the song.
     * @param iterationIndex the iteration index. Should be valid.
     * @param reg the register (0-15).
     * @param successOut set to true if the extraction went fine.
     * @return the value, or 0 if the extraction failed.
     */
    unsigned char getValue(int iterationIndex, int reg, bool& successOut) const noexcept;

    /** @return how many registers are encoded in a frame. */
    virtual int getEncodedRegisterCountInFrame() const noexcept = 0;

protected:
    /** Checks the tag at the beginning of the stream. */
    bool checkTag(const juce::String& tag) const noexcept;

    /** @return the next 4 bytes, big-endian format. The stream advances automatically. */
    int readBigEndian4Bytes() const noexcept;
    /** @return the next 4 bytes, little-endian format. The stream advances automatically. */
    int readLittleEndian4Bytes() const noexcept;
    /** @return the next 0-terminated String. The stream advances automatically. */
    juce::String readNTString() const noexcept;

    /** @return true if the YM is interleaved. */
    virtual bool isInterleaved() const noexcept = 0;
    /** @return how many bytes there are after the end of the music data. May be 0. */
    virtual int getPostMusicFramesDataSize() const noexcept = 0;
    /** @return the iteration count. */
    virtual int getIterationCount() const noexcept = 0;
    /** @return the index where the first register of the first frame. */
    virtual int getFirstFrameDataIndex() const noexcept = 0;

    juce::InputStream& inputStream;                   // The Stream of the YM.
};

}   // namespace arkostracker
