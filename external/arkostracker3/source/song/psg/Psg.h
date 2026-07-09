#pragma once

#include "PsgFrequency.h"
#include "PsgMixingOutput.h"
#include "PsgType.h"

namespace arkostracker 
{

/**
 * Holder the PSG metadata, such as a PSG frequency (1000000hz, for example), a reference frequency (440hz most of the time),
 * and a sample player frequency (12000hz for example).
 *
 * This class is immutable.
 */
class Psg
{
public:
    /**
     * Constructor.
     * @param psgType the type of the PSG (AY/YM).
     * @param psgFrequency the PSG frequency (1000000hz for a CPC), in Hz.
     * @param referenceFrequency the reference frequency, in Hz. 440hz most of the time.
     * @param samplePlayerFrequency the frequency at which the sample player plays the sample (12000hz for example).
     * @param psgMixingOutput how the PSG mixes its channels (ABC, BCA, etc.).
     */
    explicit Psg(PsgType psgType = PsgType::ay, int psgFrequency = PsgFrequency::psgFrequencyCPC, float referenceFrequency = PsgFrequency::defaultReferenceFrequencyHz,
                 int samplePlayerFrequency = PsgFrequency::defaultSamplePlayerFrequencyHz, const PsgMixingOutput& psgMixingOutput = PsgMixingOutput::ABC) noexcept;

    /** @return another instance with a changed PSG frequency. */
    Psg withPsgFrequency(int psgFrequency) const noexcept;

    /** @return another instance with a changed sample player frequency. */
    Psg withSamplePlayerFrequency(int samplePlayerFrequency) const noexcept;

    static Psg buildForCpc() noexcept;
    static Psg buildForPlaycity() noexcept;
    static Psg buildForPcw() noexcept;
    static Psg buildForAtariSt() noexcept;
    static Psg buildForAtariXeXl() noexcept;
    static Psg buildForApple2() noexcept;
    static Psg buildForOric() noexcept;
    static Psg buildForSpectrum() noexcept;
    static Psg buildForSpecNext() noexcept;
    static Psg buildForPentagon() noexcept;
    static Psg buildForMsx() noexcept;
    static Psg buildForSvi3x8() noexcept;
    static Psg buildForVectrex() noexcept;
    static Psg buildForSharpMz700() noexcept;

    /** @return the type of the PSG (AY/YM). */
    PsgType getType() const noexcept;
    /** Returns the PSG frequency (1000000hz for a CPC), in Hz. */
    int getPsgFrequency() const noexcept;
    /** Returns the reference frequency, in Hz. 440hz most of the time. */
    float getReferenceFrequency() const noexcept;
    /** Returns the frequency at which the sample player plays the sample (12000hz for example). */
    int getSamplePlayerFrequency() const noexcept;
    /**
     * @return how the PSG mixes its channels (ABC, BCA, etc.).
     */
    PsgMixingOutput getPsgMixingOutput() const noexcept;

    /** Equality operator, used for maps. */
    bool operator==(const Psg& other) const noexcept; // NOLINT(fuchsia-overloaded-operator)
    /** Inequality operator. */
    bool operator!=(const Psg& other) const noexcept; // NOLINT(fuchsia-overloaded-operator)
    /** For sets. */
    bool operator<(const Psg& rhs) const noexcept;

private:
    PsgType psgType;                    // The type of the PSG (AY/YM).
    int psgFrequency;                   // The PSG frequency (1000000hz for a CPC), in Hz.
    float referenceFrequency;           // The reference frequency, in Hz. 440hz most of the time.
    int samplePlayerFrequency;          // The frequency at which the sample player plays the sample (12000hz for example).
    PsgMixingOutput psgMixingOutput;    // How the PSG mixes its channels (ABC, BCA, etc.).
};

/** Comparator for PSG metadata. Only the PSG frequency and reference frequency are compared! */
class PsgMetadataPsgAndReferenceFrequencyComparator
{
public:
    bool operator()(const Psg& left, const Psg& right) const noexcept; // NOLINT(fuchsia-overloaded-operator)
};

}   // namespace arkostracker
