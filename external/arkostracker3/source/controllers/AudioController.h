#pragma once

#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

#include "../player/PsgRegistersProvider.h"
#include "../song/Song.h"

namespace arkostracker 
{

class SongPlayer;
class MidiController;
class OutputMix;

/**
 * Abstract Audio Controller. It also holds the Midi Controller.
 * Holds the AY Generator and player. There should only be once instance of this Controller.
 *
 * This Controller is a PsgRegistersProvider, but it doesn't provide the registers itself: it uses the SongPlayer for that.
 */
class AudioController
{
public:
    /** Destructor. */
    virtual ~AudioController() = default;

    /** Changes the song. This changes the Song stored in the SongPlayer, and stops it. */
    virtual void stopPlayerAndSetSong(std::shared_ptr<Song> newSong) noexcept = 0;

    /**
     * Stops the song from being played. Sounds are stopped, effects are reset.
     * This posts a message for the audio player to stop, so is asynchronous, UNLESS blockTillStopped is true.
     * Called from UI thread.
     * This does not prevent the player from receiving events, this only stops the sounds.
     */
    virtual void stopPlayer() noexcept = 0;

    /** @return the SongPlayer. */
    virtual SongPlayer& getSongPlayer() noexcept = 0;

    /** @return the Midi Controller. */
    virtual MidiController& getMidiController() noexcept = 0;

    /** @return how large the signal buffer size is, for those who want to read and display it. */
    virtual size_t getSignalBufferSize() const noexcept = 0;

    /**
     * Fills the blocks (one per channel) with the signal of each channel.
     * This should be called on the UI thread only, it is useful to draw vu-meters, for example.
     * @param blocksToFill the blocks to fill. They MUST have the right size (see getSignalBufferSize).
     */
    virtual void fillSignal(std::vector<juce::HeapBlock<uint16_t>*>& blocksToFill) noexcept = 0;

    /** @return the Audio Device Manager. */
    virtual juce::AudioDeviceManager& getAudioDeviceManager() noexcept = 0;

    /**
     * Sets the output mix. This must be called from the UI thread.
     * @param outputMix the output mix.
     */
    virtual void setOutputMix(const OutputMix& outputMix) noexcept = 0;
};

}   // namespace arkostracker

