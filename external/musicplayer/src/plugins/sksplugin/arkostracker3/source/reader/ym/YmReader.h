#pragma once

#include "../StreamedMusicReader.h"
#include "decoder/YmDecoder.h"

namespace arkostracker 
{

/** Reader for a YM. Allows to extract frames from it. */
class YmReader final : public StreamedMusicReader
{
public:
    /**
     * Constructor.
     * @param inputStream the stream to read the YM data from.
     */
    explicit YmReader(juce::InputStream& inputStream) noexcept;

    // StreamedMusicReader method implementations.
    // ===================================================
    bool checkFormat() noexcept override;
    bool prepare() noexcept override;
    int getPsgCount() const noexcept override;
    PsgType getPsgType(int psgIndex) const noexcept override;
    int getIterationCount() const noexcept override;
    int getDigidrumCount() const noexcept override;
    int getLoopIndex() const noexcept override;
    int getPsgFrequencyHz(int psgIndex) const override;
    float getPlayerReplayHz() const override;
    juce::String getTitle() const override;
    juce::String getAuthor() const override;
    juce::String getComments() const override;
    SamplePlayInfo getSamplePlayInfo(int sampleIndex, int sampleReplayFrequencyHz) const override;
    std::pair<PsgRegisters, bool> getRegisters(int psgIndex, int iterationIndex) const noexcept override;
    bool areDigidrumsAllowed() const override;
    std::vector<std::shared_ptr<Sample>> getDigidrums() const noexcept override;

private:
    /**
     * Parses the YM to build data about linking iterations to their hardware envelope. Useful when generating Instruments.
     * Warning, it is probably wiser to do that in a worker thread, since the YM is parsed.
     */
    void buildHardwareEnvelopesForIterations() noexcept;

    /**
     * Returns the latest known hardware envelope related to the given iteration index. The buildHardwareEnvelopesForIterations
     * method MUST have been called before! This is fast, since the building has been done before.
     * @param iterationIndex the index.
     * @return the hardware envelope (0-15) (warning, AT3 Instrument requests envelopes from 8-15) and the retrig state.
     */
    HardwareEnvelopeAndRetrig getHardwareEnvelopeAndRetrig(int iterationIndex) const noexcept;

    std::unique_ptr<YmDecoder> ymDecoder;   // The selected YM Decoder.
    YmMetadata ymMetadata;                  // The YM metadata.

    std::map<int, HardwareEnvelopeAndRetrig> iterationToHardwareEnvelope;     // Links iterations to a change in hardware envelope.
};

}   // namespace arkostracker
