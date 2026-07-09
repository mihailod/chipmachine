#pragma once

#include <memory>

#include "../../song/instrument/Instrument.h"

namespace arkostracker 
{

/** Generates generic Instruments (useful when wanting a template sound when creating a sound, or for MIDI import). */
class GenericInstrumentGenerator
{
public:
    enum class TemplateId : uint8_t
    {
        empty,
        singleBeep,
        decreasingVolume,
        attackSustainDecay,
        smallAttackAndLoop,
        hardBass1,
        hardBass2,
        bassdrum,
        snare,
        tom,
        hihat,
        cymbalCrash,
    };

    /** Prevents instantiation. */
    GenericInstrumentGenerator() = delete;

    /** Builds an Instrument with only a single beep. */
    static std::unique_ptr<Instrument> generateInstrumentForSingleBeep() noexcept;
    /** Builds an empty Instrument. */
    static std::unique_ptr<Instrument> generateInstrumentForEmpty() noexcept;
    /**
     * Builds an Instrument with a decreasing volume.
     * @param name a name, or empty to use a default one.
     */
    static std::unique_ptr<Instrument> generateInstrumentForDecreasingVolume(const juce::String& name = juce::String()) noexcept;
    /** Builds an Instrument with a small attack and a loop. */
    static std::unique_ptr<Instrument> generateInstrumentForSmallAttackAndLoop() noexcept;
    /** Builds an Instrument with a hardware bass, wave 8. */
    static std::unique_ptr<Instrument> generateInstrumentForHardBass1() noexcept;
    /** Builds an Instrument with a hardware bass, wave A. */
    static std::unique_ptr<Instrument> generateInstrumentForHardBass2() noexcept;
    /** Builds an Instrument with a small attack and a long sustain, then decay. */
    static std::unique_ptr<Instrument> generateInstrumentForAttackSustainDecay() noexcept;

    /** Builds an Instrument with a hihat. */
    static std::unique_ptr<Instrument> generateInstrumentForHihat() noexcept;
    /** Builds an Instrument with a bassdrum. */
    static std::unique_ptr<Instrument> generateInstrumentForBassDrum() noexcept;
    /** Builds an Instrument with a snare. */
    static std::unique_ptr<Instrument> generateInstrumentForSnare() noexcept;
    /** Builds an Instrument with a crash. */
    static std::unique_ptr<Instrument> generateInstrumentForCrash() noexcept;
    /**
     * Builds an Instrument with a tom.
     * @param pitchOffset an offset to add to the pitch, simulate lower of higher toms.
     * @return the Instrument.
     */
    static std::unique_ptr<Instrument> generateInstrumentForTom(int pitchOffset) noexcept;

    /** @return a map linking an ID of template instrument to its displayable name ("Beep", "Decreasing volume", etc.). */
    static const std::map<TemplateId, juce::String>& getTemplateIdToDisplayableName() noexcept;

    /**
     * @return an Instrument from the given id.
     * @param templateId the ID of the template.
     */
    static std::unique_ptr<Instrument> buildFromTemplateId(TemplateId templateId) noexcept;

private:
    static const juce::String singleBeepInstrumentName;
    static const juce::String decreasingVolumeInstrumentName;
    static const juce::String attackSustainDecayInstrumentName;
    static const juce::String smallAttackAndLoopInstrumentName;
    static const juce::String hardBass1InstrumentName;
    static const juce::String hardBass2InstrumentName;
    static const juce::String bassdrumInstrumentName;
    static const juce::String snareInstrumentName;
    static const juce::String tomInstrumentName;
    static const juce::String hihatInstrumentName;
    static const juce::String cymbalCrashInstrumentName;
};

}   // namespace arkostracker
