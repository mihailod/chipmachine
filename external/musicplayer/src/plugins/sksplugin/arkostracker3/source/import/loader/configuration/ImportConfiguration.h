#pragma once

#include "../../mod/ModConfiguration.h"
#include "../../midi/MidiConfiguration.h"

namespace arkostracker 
{

/** A default import configuration. You can pass specific implementation via the constructor, which allows to use them without deriving this class. */
class ImportConfiguration
{
public:
    /**
     * Constructor.
     * @param modConfigurationToUse if present, will use this one (a copy is made). Else, a default one is used.
     * @param midiConfigurationToUse if present, will use this one (a copy is made). Else, a default one is used.
     */
    explicit ImportConfiguration(const ModConfiguration* modConfigurationToUse = nullptr,
                                 const MidiConfiguration* midiConfigurationToUse = nullptr) noexcept;

    /** @return the configuration for MOD import. */
    ModConfiguration getConfigurationForMod() const noexcept;
    /** @return the configuration for MIDI import. */
    MidiConfiguration getConfigurationForMidi() const noexcept;

private:
    ModConfiguration modConfiguration;
    MidiConfiguration midiConfiguration;
};

}   // namespace arkostracker

