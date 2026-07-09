#pragma once

#include "../StreamedMusicReader.h"
#include "../../utils/OccurrenceMap.h"

namespace arkostracker 
{

class VgmReader : public StreamedMusicReader
{
public:
    /**
     * Constructor.
     * @param inputStream the stream to read the YM data from.
     */
    explicit VgmReader(juce::InputStream& inputStream) noexcept;

    // StreamedMusicReader method implementations.
    // ===================================================
    bool checkFormat() noexcept override;
    bool prepare() noexcept override;
    int getPsgCount() const noexcept override;
    PsgType getPsgType(int psgIndex) const noexcept override;
    int getIterationCount() const noexcept override;
    int getLoopIndex() const noexcept override;
    int getPsgFrequencyHz(int psgIndex) const override;
    float getPlayerReplayHz() const override;
    juce::String getTitle() const override;
    juce::String getAuthor() const override;
    SamplePlayInfo getSamplePlayInfo(int sampleIndex, int sampleReplayFrequencyHz) const override;
    std::pair<PsgRegisters, bool> getRegisters(int psgIndex, int iterationIndex) const noexcept override;
    bool areDigidrumsAllowed() const override;
    int getDigidrumCount() const override;
    juce::String getComments() const override;
    std::vector<std::shared_ptr<Sample>> getDigidrums() const noexcept override;

private:
    static const int maximumPsgCount;

    /**
     * @return the version of the VGM (BCD decoding). This changes the position of the Stream. Returns 0 if fails.
     * @param inputStream the stream to read.
     */
    static int readVersion(juce::InputStream& inputStream) noexcept;

    /**
     * Parses the whole VGM data.
     * @return true if everything went fine.
     */
    bool parseVgmData() noexcept;

    /**
     * Called when a PSG register is found.
     * @param reg the register.
     * @param value the value.
     * @param isFirstPsg true if first PSG, false if second.
     */
    void onPsgRegisterFound(int reg, unsigned char value, bool isFirstPsg) noexcept;

    /**
     * Called when a "wait" is found.
     * @param sampleCount the sample count, in division of 44100hz.
     */
    void onWaitFound(unsigned int sampleCount) noexcept;

    /** Parses the possible GD3 chunk and fills the internal data. */
    void parseGd3() noexcept;

    int psgCount;
    int psgFrequencyHz;
    PsgType psgType;
    float playerReplayHz;
    int loopIndex;
    juce::String title;
    juce::String author;

    int vgmDataPosition;
    int vgmLoopPosition;                                    // Position in the VGM data where the loop starts.

    std::vector<PsgRegisters> currentFramePsgRegisters;     // PSG 1 and possibly 2.
    OccurrenceMap<unsigned int> waitSampleToOccurrence;     // Stores how many waits there are to determine the most probably replay rate in Hz.

    std::vector<std::vector<PsgRegisters>> psgToPsgRegisters;   // Links a PSG to the PSG frame.
    std::vector<OptionalValue<unsigned char>> psgToPreviousHardwareEnvelope;   // Links a PSG to the previous hardware envelope, useful to know if retrig or not.
};


}   // namespace arkostracker

