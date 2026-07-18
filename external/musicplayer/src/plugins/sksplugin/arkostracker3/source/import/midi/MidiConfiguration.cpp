#include "MidiConfiguration.h"

#include <utility>

namespace arkostracker 
{

MidiConfiguration::MidiConfiguration() noexcept :
        ppq(120),
        drumChannel(10),
        importVelocities(true),
        channelMixerKeeper()
{
    channelMixerKeeper.addTracksToKeep({ 0, 1, 2 });        // By default, only retrieve three tracks.
}

MidiConfiguration::MidiConfiguration(int pPpq, int pDrumChannel, bool pImportVelocities, ChannelMixerKeeper pChannelMixerKeeper) noexcept :
        ppq(pPpq),
        drumChannel(pDrumChannel),
        importVelocities(pImportVelocities),
        channelMixerKeeper(std::move(pChannelMixerKeeper))
{
}

int MidiConfiguration::getPpq() const noexcept
{
    return ppq;
}

int MidiConfiguration::getDrumChannel() const noexcept
{
    return drumChannel;
}

bool MidiConfiguration::isImportVelocities() const noexcept
{
    return importVelocities;
}

const ChannelMixerKeeper& MidiConfiguration::getChannelMixerKeeper() const noexcept
{
    return channelMixerKeeper;
}


}   // namespace arkostracker

