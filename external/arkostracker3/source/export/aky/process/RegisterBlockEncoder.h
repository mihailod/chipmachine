#pragma once

#include <juce_core/juce_core.h>

#include "../../../player/SpecialEffect.h"
#include "../../../song/sid/SidPlayerCapability.h"

namespace arkostracker
{

class ChannelOutputRegisters;
class EncodedLine;
class RegisterBlock;
class RegisterBlockLabelProvider;
class RegisterBlockPool;
class SourceGenerator;

/**
    Class that encodes a Register Block as source.
*/
class RegisterBlockEncoder
{
public:
    /**
     * Constructor.
     * @param sourceGenerator the source generator.
     * @param registerBlockPool the RegisterBlock Pool, used for some optimizations.
     * @param registerBlockLabelProvider provides labels for the lines of RegisterBlocks.
     * @param littleEndian true to encode as little endian.
     * @param sidPlayerCapability the SID player capability.
     */
    RegisterBlockEncoder(SourceGenerator& sourceGenerator, RegisterBlockPool& registerBlockPool, RegisterBlockLabelProvider& registerBlockLabelProvider,
        bool littleEndian, const SidPlayerCapability& sidPlayerCapability) noexcept;

    /**
     * Encodes the given RegisterBlock as raw bytes, internally.
     * This is a mandatory step before being able to generate the source.
     * @param registerBlock the Block to encode.
     * @param label the Label to declare.
     * @param relativeToLabel a possible label to encode the first label relative to (<label> - <relativeToLabel>). If empty, not used.
     */
    void encode(RegisterBlock& registerBlock, const juce::String& label, const juce::String& relativeToLabel) noexcept;

    /**
     * Generates the source of this RegisterBlock. It must have been encoded before.
     * @param registerBlock the Block to encode.
     * @param label the Label to declare.
     * @param baseLabel the song base label. Only useful if wanting to encode the label relative to the base label. Empty if not interested.
     */
    void generateSource(const RegisterBlock& registerBlock, const juce::String& label, const juce::String& baseLabel) const noexcept;

private:
    static const juce::String loopLabelSuffix;            // The String to add to the labels, to mark the loop.

    /** Clears the current registers. Useful to be called between each encoding. */
    void clearCurrentRegisters();

    /**
     * Encodes the given ChannelRegisters, as an initial line. This sets-up the initial values.
     * @param registerBlock the RegisterBlock, for it to be populated with the EncodedLines.
     * @param channelRegisters the ChannelRegisters to encode.
     */
    void encodeInitialLine(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an initial line, for a "no software no hardware" sound. */
    void encodeInitialLineNoSoftwareNoHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an initial line, for a "software only" sound. */
    void encodeInitialLineSoftwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an initial line, for a "hardware only" sound. */
    void encodeInitialLineHardwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an initial line, for a "software and hardware" sound. */
    void encodeInitialLineSoftwareAndHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;

    /** Encodes the given software period, as a full Period or an index. */
    //void encodeSoftwarePeriodOrIndex(int period) noexcept;

    /**
     * Encodes the given ChannelRegisters, as an non-initial line.
     * @param registerBlock the RegisterBlock, for it to be populated with the EncodedLines.
     * @param channelRegisters the ChannelRegisters to encode.
     */
    void encodeNonInitialLine(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an non-initial line, for a "no software no hardware" sound. */
    void encodeNonInitialLineNoSoftwareNoHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) const noexcept;
    /** Encodes the given ChannelRegisters as an non-initial line, for a "software only" sound. */
    void encodeNonInitialLineSoftwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an non-initial line, for a "hardware only" sound. */
    void encodeNonInitialLineHardwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the given ChannelRegisters as an non-initial line, for a "software and hardware" sound. */
    void encodeNonInitialLineSoftwareAndHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;

    /**
     * Encodes the "Noise and/or Retrig" byte, if needed. This is shared with the "hardware only" and "software and hardware".
     * Nothing happens if the isNoiseOrRetrig is false.
     * @param encodedLine the encoded line to which add the noise sequence.
     * @param isNoiseOrRetrig true if there is the retrig and/or the noise (with a new value or no).
     * @param retrig true if retrig.
     * @param isNoise true if there is noise (noise channel to open). The value of the noise may have not changed!
     * @param isNewNoise true if the noise value has changed. Only relevant of the isNoise is 1.
     * @param noiseValue the noise value, if relevant.
     */
    static void encodeNoiseOrRetrigIfNeeded(EncodedLine& encodedLine, bool isNoiseOrRetrig, bool retrig, bool isNoise, bool isNewNoise, int noiseValue) noexcept;

    /** Encodes the possible SID data, for an initial line, if needed. */
    void encodeInitialLineSidIfNeeded(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the possible SID data, for an non-initial line, if needed. */
    void encodeNonInitialLineSidIfNeeded(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept;
    /** Encodes the possible SID periods. Used by IS and NIS (AFTER the low-shelf volume has been encoded). */
    void encodeSidPeriodsAndStoreThem(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters, EncodedLine& sidEncodedLine) noexcept;

    SourceGenerator& sourceGenerator;                       // The source generator.
    RegisterBlockPool& registerBlockPool;                   // The RegisterBlock Pool, used for some optimizations.
    RegisterBlockLabelProvider& registerBlockLabelProvider; // Provides labels for the lines of RegisterBlocks.
    bool littleEndian;                                      // True to encode as little endian.

    int currentVolume;                                      // The current volume (0-16), or -1 if undefined.
    int currentNoise;                                       // The current noise (0 is not, 1-31), or -1 if undefined.
    //int currentSound;                                       // The sound channel (0 for off, 1 for on, -1 if undefined).
    int currentSoftwarePeriod;                              // The software period (0-0xfff, -1 if undefined).
    int currentHardwarePeriod;                              // The hardware period (0-0xffff, -1 if undefined).
    int currentHardwareCurve;                               // The hardware curve (0-0x1f, -1 if undefined).

    SpecialEffect currentSpecialEffect;
    int currentSidHighShelfDuration;
    int currentSidLowShelfDuration;
    int currentSidLowShelfVolume;

    SidPlayerCapability sidPlayerCapability;
};

}   // namespace arkostracker
