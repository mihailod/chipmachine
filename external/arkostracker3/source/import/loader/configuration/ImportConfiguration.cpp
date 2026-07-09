#include "ImportConfiguration.h"

namespace arkostracker 
{

ImportConfiguration::ImportConfiguration(const ModConfiguration* pModConfigurationToUse, const MidiConfiguration* pMidiConfigurationToUse) noexcept:
        modConfiguration((pModConfigurationToUse == nullptr) ? ModConfiguration() : *pModConfigurationToUse),
        midiConfiguration((pMidiConfigurationToUse == nullptr) ? MidiConfiguration() : *pMidiConfigurationToUse)
{
}

ModConfiguration ImportConfiguration::getConfigurationForMod() const noexcept
{
    return modConfiguration;
}

MidiConfiguration ImportConfiguration::getConfigurationForMidi() const noexcept
{
    return midiConfiguration;
}

}   // namespace arkostracker
