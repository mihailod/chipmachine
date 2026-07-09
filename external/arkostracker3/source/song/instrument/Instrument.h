#pragma once

#include <juce_core/juce_core.h>

#include "InstrumentType.h"
#include "psg/PsgPart.h"
#include "sample/SamplePart.h"

namespace arkostracker 
{

/**
 * An Instrument. It can be a PSG or Sample Instrument (or maybe later SID-Sample, etc.).
 *
 * Contrary to AT2 design, I decided to use composition to hold the various parts.
 * Even though an Instrument has only one type, it is technically possible to have PSG and Sample data.
 * No need for polymorphism, cast or visitor to handle this...
 * It also makes copy of an Instrument trivial.
 * Since there shouldn't be many types of Instrument (2 most likely), it doesn't matter that parts of it are unused/irrelevant.
 *
 * This class does NOT lock anything. It is NOT thread safe.
 */
class Instrument
{
public:
    static const juce::uint32 defaultColor;
    static const juce::String emptyInstrumentName;

    /**
     * @return a PSG Instrument.
     * @param name the name.
     * @param psgPart the PsgPart that accompanies it. A copy will be made, so it must be ready when given.
     * @param color the color.
     */
    static std::unique_ptr<Instrument> buildPsgInstrument(const juce::String& name, const PsgPart& psgPart, juce::uint32 color = defaultColor) noexcept;
    /** @return a "mute" PSG Instrument (loops on a unique empty Cell). Its speed is the slowest. */
    static std::unique_ptr<Instrument> buildEmptyPsgInstrument() noexcept;

    /**
     * @return a Sample Instrument. Only a raw constructor FOR NOW.
     * @param name the name.
     * @param samplePart the SamplePart that accompanies it.
     * @param color the color.
     */
    static std::unique_ptr<Instrument> buildSampleInstrument(const juce::String& name, const SamplePart& samplePart, juce::uint32 color = defaultColor) noexcept;

    /**
     * Constructor. Use the static constructor instead.
     * @param name the name of the Instrument.
     * @param instrumentType the type of the Instrument.
     * @param psgPart the PSG part, if relevant.
     * @param samplePart the sample part, if relevant.
     * @param color the color.
     */
    explicit Instrument(juce::String name, InstrumentType instrumentType, PsgPart psgPart, SamplePart samplePart, juce::uint32 color = defaultColor) noexcept;

    /** @return the unique ID of the Instrument. Do not serialize it. */
    Id getId() const noexcept;

    /** @return the name of the Instrument. */
    juce::String getName() const noexcept;
    /**
     * Sets the name to the Instrument.
     * @param name the name.
     */
    void setName(const juce::String& name) noexcept;

    /** @return the type of the instrument. */
    InstrumentType getType() const noexcept;

    /** @return the color. */
    juce::uint32 getArgbColor() const noexcept;
    /** @param newArgbColor the color. */
    void setColor(juce::uint32 newArgbColor) noexcept;

    /** @return the PSG Part. If not a PSG instrument, an assertion is raised. */
    const PsgPart& getConstPsgPart() const noexcept;
    /** @return the PSG Part. If not a PSG instrument, an assertion is raised. */
    PsgPart& getPsgPart() noexcept;
    /** @return the Sample Part. If not a Sample instrument, an assertion is raised. */
    const SamplePart& getConstSamplePart() const noexcept;
    /** @return the Sample Part. If not a Sample instrument, an assertion is raised. */
    SamplePart& getSamplePart() noexcept;

    bool operator==(const Instrument& other) const noexcept;
    bool operator!=(const Instrument& other) const noexcept;

    /** Compares the Instrument with another, skipping metadata like the name and color. */
    bool isMusicallyEqualTo(const Instrument& other) const noexcept;

private:
    const Id id;                    // Uniquely identify the Instrument for this session. Not serialized.

    juce::String name;              // Name of the item.

    InstrumentType instrumentType;

    juce::uint32 color;             // A color for this instrument.

    PsgPart psgPart;                // The PSG part of an Instrument. Irrelevant if not used.
    SamplePart samplePart;          // The Sample part of an Instrument. Irrelevant if not used.

    JUCE_LEAK_DETECTOR (Instrument)
};

}   // namespace arkostracker
