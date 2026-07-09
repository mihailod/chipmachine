#pragma once

#include <memory>
#include <vector>

#include "../../../song/instrument/Instrument.h"

namespace arkostracker 
{

/** Serializes/deserializes Instruments. */
class InstrumentSerializer
{
public:
    /** Prevents instantiation. */
    InstrumentSerializer() = delete;

    /**
     * Serializes to XML the given Instruments.
     * @param instruments the Instruments to serialize.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<const Instrument*>& instruments) noexcept;

    /**
     * Deserializes an XML into Instruments.
     * @param root the XML root Element.
     * @return the Instruments, including the possible samples, or empty if an error occurred. The samples must be put in the SampleManager.
     */
    static std::vector<std::unique_ptr<Instrument>> deserializeInstruments(const juce::XmlElement& root) noexcept;

    /**
     * Serializes to XML the given Instrument.
     * @param instrument the Instrument to serialize.
     * @return the XML node of the Instrument.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const Instrument& instrument) noexcept;

    /**
     * Deserializes an XML into one Instrument.
     * @param instrumentRoot the XML root Element of the Instrument.
     * @return the Instrument, or empty if an error occurred. If it is a Sample Instrument, the Sample MUST be also be put in the SampleManager by the caller.
     */
    static std::unique_ptr<Instrument> deserializeInstrument(const juce::XmlElement& instrumentRoot) noexcept;

private:
    /**
     * Creates the PSG part of the given Instrument.
     * @param instrumentRoot the XML root.
     * @param errorOut set to true if an error occurred.
     */
    static PsgPart createPsgPart(const juce::XmlElement& instrumentRoot, bool& errorOut) noexcept;

    /**
     * Creates the Sample part of the given Instrument.
     * @param instrumentRoot the XML root.
     * @param errorOut set to true if an error occurred.
     * @return the sample part, including the sample, which MUST be also put typically in the SampleManager by the caller.
     */
    static SamplePart createSamplePart(const juce::XmlElement& instrumentRoot, bool& errorOut) noexcept;

    /**
     * Serializes the PSG part.
     * @param instrument the instrument.
     * @param rootXml the root XML.
     */
    static void serializePsgPart(const Instrument& instrument, juce::XmlElement& rootXml) noexcept;

    /**
     * Serializes the sample part.
     * @param instrument the instrument.
     * @param rootXml the root XML.
     */
    static void serializeSamplePart(const Instrument& instrument, juce::XmlElement& rootXml) noexcept;
};

}   // namespace arkostracker
