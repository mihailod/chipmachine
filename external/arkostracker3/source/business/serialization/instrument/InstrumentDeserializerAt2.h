#pragma once

#include "../../../song/instrument/Instrument.h"

namespace arkostracker
{

class InstrumentDeserializerAt2
{
public:

    /**
     * Deserializes an AT2 XML into one Instrument. Only PSG one are parsed, not Sample.
     * @param instrumentRoot the XML root Element of the Instrument ("aks:instrument").
     * @return the Instrument, or empty if an error occurred.
     */
    static std::unique_ptr<Instrument> deserializeInstrument(const juce::XmlElement& instrumentRoot) noexcept;

private:
    /**
     * Adds a cell to the given PSG part.
     * @param psgPart the PSG part to fill.
     * @param cellNode the node to read.
     */
    static void addCell(PsgPart& psgPart, const juce::XmlElement& cellNode) noexcept;

    static const juce::String nodeRoot;
    static const juce::String nodeFmInstrument;
    static const juce::String nodeTitle;
    static const juce::String nodeSpeed;
    static const juce::String nodeIsLooping;
    static const juce::String nodeLoopStartIndex;
    static const juce::String nodeEndIndex;
    static const juce::String nodeIsRetrig;
    static const juce::String nodeFmInstrumentCell;

    static const juce::String nodeLink;
    static const juce::String nodeVolume;
    static const juce::String nodeNoise;
    static const juce::String nodeSoftwarePeriod;
    static const juce::String nodeSoftwareArpeggio;
    static const juce::String nodeSoftwarePitch;
    static const juce::String nodeRatio;
    static const juce::String nodeHardwarePeriod;
    static const juce::String nodeHardwareArpeggio;
    static const juce::String nodeHardwarePitch;
    static const juce::String nodeHardwareEnvelope;
};

}   // namespace arkostracker
