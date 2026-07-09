#pragma once

#include "../common/ChannelMixerKeeper.h"

namespace arkostracker 
{

/** Holds the configuration for a MIDI import. */
class MidiConfiguration
{
public:
    /** Default constructor. */
    MidiConfiguration() noexcept;

    /**
     * Constructor.
     * @param ppq the "parts per quarter" to use (24, 120, etc.) to regroup events into Cells. The lowest, the more precise, but also more lines.
     * @param drumChannel the channel where the drums are (10 most of the time, or 16).
     * @param importVelocities true to import note velocities.
     * @param channelMixerKeeper holds what channels to mix and keep.
     */
    MidiConfiguration(int ppq, int drumChannel, bool importVelocities, ChannelMixerKeeper channelMixerKeeper) noexcept;

    int getPpq() const noexcept;
    int getDrumChannel() const noexcept;
    bool isImportVelocities() const noexcept;
    const ChannelMixerKeeper& getChannelMixerKeeper() const noexcept;

private:
    int ppq;
    int drumChannel;
    bool importVelocities;
    ChannelMixerKeeper channelMixerKeeper;
};



}   // namespace arkostracker

