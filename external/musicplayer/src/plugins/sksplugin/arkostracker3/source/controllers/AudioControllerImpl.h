#pragma once

#include <memory>
#include <vector>

#include "../audio/sources/PsgStreamGenerator.h"
#include "../audio/sources/PsgsProcessor.h"
#include "../player/SongPlayer.h"
#include "observers/ChannelMuteObserver.h"
#include "observers/SubsongMetadataObserver.h"
#include "observers/SongPlayerObserver.h"
#include "AudioController.h"
#include "MidiController.h"

namespace arkostracker 
{

class MainController;
class SongController;
class PlayerController;

/** Implementation of the AudioController. */
class AudioControllerImpl final : public AudioController,
                                  public ChannelMuteObserver,
                                  public SubsongMetadataObserver,
                                  public SongPlayerObserver
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param song the Song.
     */
    explicit AudioControllerImpl(MainController& mainController, std::shared_ptr<const Song> song) noexcept;

    /** Destructor. */
    ~AudioControllerImpl() override;

    // AudioController method implementations.
    // ===================================================
    void stopPlayerAndSetSong(std::shared_ptr<Song> newSong) noexcept override;
    void stopPlayer() noexcept override;

    SongPlayer& getSongPlayer() noexcept override;
    MidiController& getMidiController() noexcept override;
    size_t getSignalBufferSize() const noexcept override;
    void fillSignal(std::vector<juce::HeapBlock<uint16_t>*>& blocksToFill) noexcept override;
    juce::AudioDeviceManager& getAudioDeviceManager() noexcept override;
    void setOutputMix(const OutputMix& outputMix) noexcept override;

    // ChannelMuteObserver method implementations.
    // ===================================================
    void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) override;

    // SubsongMetadataObserver method implementations.
    // ===================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) override;

    // SongPlayerObserver method implementations.
    // ===================================================
    void onPlayerNewLocations(const Locations& locations) noexcept override;

private:
    /**
     * Builds the PSG Stream Generator to reach the given PSG count.
     * This must be called every time the PSG count changed (PSG added/deleted, or changed on Subsong with another PSG count).
     * @return true if the PSG Stream Generators have been rebuilt.
     */
    bool buildAndRigPsgStreamGeneratorIfNeeded() noexcept;

    MainController& mainController;
    SongController& songController;
    SongPlayer songPlayer;                                                          // Reads the Song and extracts PSG register from it.

    OptionalValue<Location> latestPlayedLocation;                                   // The played location, as returned by the player. At first, unknown.

    juce::AudioDeviceManager audioDeviceManager;                                    // Global manager of the audio devices.
    PsgsProcessor psgsProcessor;                                                    // Mixes all the PSG, applies the effects.
    std::vector<std::unique_ptr<PsgStreamGenerator>> psgStreamGenerators;           // One per PSG.
    juce::AudioSourcePlayer audioSourcePlayer;                                      // Continuously stream audio to an AudioIODevice. Only one is required, as all it mixed.

    std::unique_ptr<MidiController> midiController;
};

}   // namespace arkostracker
