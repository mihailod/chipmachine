#pragma once

#include "../player/PsgRegisters.h"
#include "../player/SamplePlayInfo.h"
#include "../song/psg/PsgType.h"

namespace arkostracker 
{

/** Abstract class to read any "framed" music, such as YM and VGM. */
class StreamedMusicReader
{
public:
    /**
     * Constructor.
     * @param inputStream the stream to read the YM data from.
     */
    explicit StreamedMusicReader(juce::InputStream& inputStream) noexcept;

    /** Destructor. */
    virtual ~StreamedMusicReader() = default;

    /** Indicates whether the format is correct. This MUST be called before anything else. */
    virtual bool checkFormat() noexcept = 0;

    /**
     * Must be called only after if the format is correct (see checkFormat method).
     * May parse the file, or prepare some data to accelerate the parsing later.
     * This implementation is expected to parse the whole file, which might take some time.
     * @return false if an error occurred, the music cannot be played.
     */
    virtual bool prepare() noexcept = 0;

    /** @return the how many PSG there are. */
    virtual int getPsgCount() const noexcept = 0;
    /**
     * @return the PSG type.
     * @param psgIndex the PSG index. Must be valid.
     */
    virtual PsgType getPsgType(int psgIndex) const noexcept = 0;
    /** @return how many iterations are encoded. */
    virtual int getIterationCount() const noexcept = 0;
    /** @return the loop index. */
    virtual int getLoopIndex() const noexcept = 0;
    /**
     * @return the speed of the PSG, in Hz (1000000 for a CPC).
     * @param psgIndex the PSG index. Must be valid.
     */
    virtual int getPsgFrequencyHz(int psgIndex) const = 0;
    /** @return the player replay, in Hz (50 most of the time). */
    virtual float getPlayerReplayHz() const = 0;
    /** @return the title of the music. */
    virtual juce::String getTitle() const = 0;
    /** @return the author of the music. */
    virtual juce::String getAuthor() const = 0;
    /** @return the comments of the music. */
    virtual juce::String getComments() const = 0;

    /** @return how many digidrums there are. */
    virtual int getDigidrumCount() const =0;

    /** @return a possible sample play info from its index. */
    virtual SamplePlayInfo getSamplePlayInfo(int sampleIndex, int sampleReplayFrequencyHz) const = 0;

    /**
     * @return the PSG registers from the given iteration, and a flag indicating if the retrieval was successful.
     * @param psgIndex the PSG index. Must be valid.
     * @param iterationIndex the iteration index. If invalid, returns a failure.
     */
    virtual std::pair<PsgRegisters, bool> getRegisters(int psgIndex, int iterationIndex) const noexcept = 0;

    /** @return true if digidrums are allowed, thanks to specific register handling. */
    virtual bool areDigidrumsAllowed() const = 0;

    /** @return all the digidrums. */
    virtual std::vector<std::shared_ptr<Sample>> getDigidrums() const noexcept = 0;

protected:
    /** @return the input stream to read from. */
    juce::InputStream& getInputStream() const noexcept;

private:
    juce::InputStream& inputStream;
};

}   // namespace arkostracker
