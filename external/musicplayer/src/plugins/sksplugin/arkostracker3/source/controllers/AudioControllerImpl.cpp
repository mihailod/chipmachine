#include "AudioControllerImpl.h"

#include "../app/preferences/PreferencesManager.h"
#include "../utils/NumberUtil.h"
#include "../utils/PsgValues.h"
#include "MainController.h"
#include "MidiControllerImpl.h"

namespace arkostracker 
{

AudioControllerImpl::AudioControllerImpl(MainController& pMainController, std::shared_ptr<const Song> pSong) noexcept:
        mainController(pMainController),
        songController(pMainController.getSongController()),
        songPlayer(std::move(pSong)),

        latestPlayedLocation(),

        audioDeviceManager(),
        psgsProcessor(),
        psgStreamGenerators(),
        audioSourcePlayer(),

        midiController(std::make_unique<MidiControllerImpl>(audioDeviceManager))
{
    // Setups the output effects from the possibly stored Preferences.
    const auto outputMix = PreferencesManager::getInstance().getOutputMix().toOutputMix();
    psgsProcessor.setOutputMix(outputMix);

    // Some observations.
    songController.getSubsongMetadataObservers().addObserver(this);                // PSG count and replay frequency changed in this subsong.
    songPlayer.getSongPlayerObservers().addObserver(this);
    mainController.observers().getChannelMuteStateObservers().addObserver(this);

    // Sets-up the AudioDevicesManager.
    // Reads the possibly saved audio preference. The result may be nullptr if it doesn't exist or is invalid. Not a problem, the audioDeviceManager can handle it.
    const auto xmlElement(PreferencesManager::getInstance().getAudioProperties());

    const auto errorString = audioDeviceManager.initialise(0, 2, xmlElement.get(), true);
    const auto success = (errorString == juce::String());

    if (!success) {
        jassertfalse;
        DBG("Initializing the audio device failed!");       // This actually never happens, even when the sound card is off...
        return;
    }

    // The chain is as follows:
    // - Multiple PSG generator (one per PSG, obviously).
    // - One MixerAudioSource (psgsProcessor) which mixes all the PSGs into one signal,
    //   and process the buffers to apply effects to the mixed signal (filter, stereo separation, etc.).
    // - One Audio Source Player, a JUCE component that "pulls" the data from the chain, and is the only element that can communicate with the Audio Device Manager.
    // - One Audio Device Manager, that plays the whole.
    //const auto psgCount = songController.getPsgCount(songController.getCurrentSubsongIndex());
    buildAndRigPsgStreamGeneratorIfNeeded();

    // PsgProcessor to AudioSourcePlayer.
    audioSourcePlayer.setSource(&psgsProcessor);

    // Links the AudioDeviceManager to AudioSourcePlayer.
    audioDeviceManager.addAudioCallback(&audioSourcePlayer);

    // Enables midi in.
    midiController->scanMidiDevicesAndRegister();
}

AudioControllerImpl::~AudioControllerImpl()
{
    mainController.observers().getChannelMuteStateObservers().removeObserver(this);
    songPlayer.getSongPlayerObservers().removeObserver(this);
    songController.getSubsongMetadataObservers().removeObserver(this);

    audioDeviceManager.removeAudioCallback(&audioSourcePlayer);

    audioSourcePlayer.setSource(nullptr);
    psgsProcessor.removeAllInputs();
}

bool AudioControllerImpl::buildAndRigPsgStreamGeneratorIfNeeded() noexcept
{
    DBG("    buildAndRigPsgStreamGeneratorIfNeeded");

    const auto subsongId = songController.getCurrentSubsongId();
    const auto& song = songController.getSong();
    const auto metadata = song->getSubsongMetadata(subsongId);
    const auto psgs = song->getSubsongPsgs(subsongId);
    const auto newReplayFrequency = metadata.getReplayFrequencyHz();
    const auto sidPlayerCapability = metadata.getSidPlayerCapability();

    const auto newPsgCount = psgs.size();
    const auto currentPsgCount = psgStreamGenerators.size();

    // We consider the first generator has the right replay frequency. There must never be any differences between PSGs.
    // On app startup, there are no generators, so it must be tested.
    const auto currentReplayFrequency = psgStreamGenerators.empty() ? PsgFrequency::defaultReplayFrequencyHz : psgStreamGenerators.at(0)->getReplayFrequencyHz();
#ifdef JUCE_DEBUG
    for (const auto& psgStreamGenerator : psgStreamGenerators) {
        jassert(juce::exactlyEqual(psgStreamGenerator->getReplayFrequencyHz(), currentReplayFrequency));      // All PSGs must have the same replay frequency!
    }
#endif

    // Maybe slow to get? Shouldn't be a problem though.
    const auto outputMix = PreferencesManager::getInstance().getOutputMix().toOutputMix();
    const auto channelAVolumeMix = static_cast<double>(outputMix.getChannelAVolume()) / 100.0;
    const auto channelBVolumeMix = static_cast<double>(outputMix.getChannelBVolume()) / 100.0;
    const auto channelCVolumeMix = static_cast<double>(outputMix.getChannelCVolume()) / 100.0;

    // Any PSG data/replay frequency changed? If yes, recreates them all.
    auto mustRecreate = (newPsgCount != currentPsgCount) || !juce::exactlyEqual(newReplayFrequency, currentReplayFrequency);
    if (!mustRecreate) {
        // Same count, but have the data changed?
        size_t psgIndex = 0U;
        for (const auto& psgStreamGenerator : psgStreamGenerators) {
            const auto& psg = psgs.at(psgIndex);
            mustRecreate = !psgStreamGenerator->isPsgMetadataEqual(psg.getType(), psg.getPsgFrequency(), newReplayFrequency,
                                                                   psg.getSamplePlayerFrequency(),
                                                                   channelAVolumeMix, channelBVolumeMix, channelCVolumeMix,
                                                                   psg.getPsgMixingOutput(),
                                                                   sidPlayerCapability);
            if (mustRecreate) {
                break;
            }
            ++psgIndex;
        }
    }

    DBG("Must recreate PSGs: " + (mustRecreate ? juce::String("true") : "false"));

    if (!mustRecreate) {
        return false;
    }

    DBG("Recreating all the PSG Stream Generators. New count = " + juce::String(newPsgCount));

    for (const auto& psgStreamGenerator : psgStreamGenerators) {
        psgsProcessor.removeInputSource(psgStreamGenerator.get());
    }
    psgStreamGenerators.clear();

    // Must create PSGs.
    for (size_t psgIndex = 0U; psgIndex < newPsgCount; ++psgIndex) {
        // What are the PSG metadata?
        const auto& psg = psgs.at(psgIndex);

        // Creates a PSG Stream Generator, links it to the PSG Mixer.
        auto psgStreamGenerator = std::make_unique<PsgStreamGenerator>(songPlayer,
                                                                       psg.getType(), static_cast<int>(psgIndex),
                                                                       newReplayFrequency,
                                                                       psg.getPsgFrequency(),
                                                                       psg.getSamplePlayerFrequency(),
                                                                       psg.getPsgMixingOutput(),
                                                                       channelAVolumeMix,
                                                                       channelBVolumeMix,
                                                                       channelCVolumeMix,
                                                                       sidPlayerCapability
        );
        psgsProcessor.addInputSource(psgStreamGenerator.get(), false);

        psgStreamGenerators.push_back(std::move(psgStreamGenerator));
    }

    return true;
}


// AudioController method implementations.
// ===================================================

SongPlayer& AudioControllerImpl::getSongPlayer() noexcept
{
    return songPlayer;
}

MidiController& AudioControllerImpl::getMidiController() noexcept
{
    return *midiController;
}

void AudioControllerImpl::fillSignal(std::vector<juce::HeapBlock<uint16_t>*>& blocksToFill) noexcept
{
    // UI thread!

    const auto blockCount = blocksToFill.size();
    const auto psgStreamCount = psgStreamGenerators.size() * static_cast<size_t>(PsgValues::channelCountPerPsg);     // 3 channels per generator.

    jassert(blockCount == psgStreamCount); (void)psgStreamCount;      // May raise if the song PSG count changes, but should happen during a very short period!

    size_t generatedChannelIndex = 0U;
    // Fills every given Blocks with the data of every PSG Stream Generator.
    for (const auto& psgStreamGenerator : psgStreamGenerators) {
        for (auto i = 0; i < PsgValues::channelCountPerPsg; ++i) {
            if (generatedChannelIndex >= blockCount) { // Security!
                return;
            }

            const auto* blockToFill = blocksToFill.at(generatedChannelIndex);
            psgStreamGenerator->fillSignal(i, *blockToFill);
            ++generatedChannelIndex;
        }
    }
}

size_t AudioControllerImpl::getSignalBufferSize() const noexcept
{
    return PsgStreamGenerator::signalReadBufferSize;
}

juce::AudioDeviceManager& AudioControllerImpl::getAudioDeviceManager() noexcept
{
    return audioDeviceManager;
}

void AudioControllerImpl::setOutputMix(const OutputMix& outputMix) noexcept
{
    // Applies the global volume/stereo separation.
    psgsProcessor.setOutputMix(outputMix);
    // Applies the channel volume mix.
    const auto rebuilt = buildAndRigPsgStreamGeneratorIfNeeded();

    // A side effect of reconstruction is that the mute states are lost in the generators.
    // Instead of doing a complicated reapplication of the mute states (which may have to be corrected because a PSG may be gone),
    // and may be incorrect anyway (which PSG is removed?), it is simpler to unmute all so that the UI matches the PSG generators state well.
    if (rebuilt) {
        mainController.unmuteAll(); // This WILL be notified here, but will have no consequence, except setting unmute once again.
    }
}


// ChannelMuteObserver method implementations.
// ===================================================

void AudioControllerImpl::onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexesInSong)
{
    auto psgIndex = 0;
    for (const auto& psgStreamGenerator : psgStreamGenerators) {
        // Keeps only the channels from the current PSG, and remaps them from 0 to 2.
        const auto firstAndLastChannelsInSong = PsgValues::getFirstAndLastChannels(psgIndex);
        const auto firstChannelOfPsgInSong = firstAndLastChannelsInSong.first;
        const auto lastChannelOfPsgInSong = firstAndLastChannelsInSong.second;
        std::unordered_set<int> mutedChannelIndexes;

        // Can't use copy_if because we also have to remap... Use a not-so-elegant code.
        for (const auto inputChannel : mutedChannelIndexesInSong) {
            if ((inputChannel >= firstChannelOfPsgInSong) && (inputChannel <= lastChannelOfPsgInSong)) {
                const auto storedChannel = inputChannel - firstChannelOfPsgInSong;
                mutedChannelIndexes.insert(storedChannel);         // Stores the PSG, but from 0-2.

                jassert((storedChannel >= 0) && (storedChannel < PsgValues::channelCountPerPsg));
            }
        }

        psgStreamGenerator->setMutedChannelIndexes(mutedChannelIndexes);

        ++psgIndex;
    }
}


// SubsongMetadataObserver method implementations.
// ===================================================

void AudioControllerImpl::onSubsongMetadataChanged(const Id& subsongId, const unsigned int what)
{
    // The Subsong id must match the played location, else it is not interesting to us.
    // If not present yet (because Song not played yet), continues.
    // Note than on loading a new song, the onPlayerNewLocations is called, fixing the latestPlayedLocation.
    if (latestPlayedLocation.isPresent() && (subsongId != latestPlayedLocation.getValueRef().getSubsongId())) {
        return;
    }

    // PSG count changed or replay frequency?
    if (!NumberUtil::isBitPresent(what, psgsData) && !NumberUtil::isBitPresent(what, subsongMetadata)) {
        return;
    }

    buildAndRigPsgStreamGeneratorIfNeeded();
}


// SongPlayerObserver method implementations.
// ===================================================

void AudioControllerImpl::onPlayerNewLocations(const Locations& locations) noexcept
{
    // This is called at every change of line, so make it quick. It is useful when changing Subsong (when loading a new Song too).
    const auto isNewSubsong = (latestPlayedLocation.isAbsent() || (latestPlayedLocation.getValueRef().getSubsongId() != locations.playedLocation.getSubsongId()));

    latestPlayedLocation = locations.playedLocation;

    if (isNewSubsong) {
        buildAndRigPsgStreamGeneratorIfNeeded();
    }
}


// ===================================================

void AudioControllerImpl::stopPlayerAndSetSong(const std::shared_ptr<Song> newSong) noexcept
{
    songPlayer.setSong(newSong);

    // Sets the playing location to the first position. What matters is that the locations points on a valid Subsong.
    const auto subsongId = newSong->getFirstSubsongId();
    const Location startLocation(subsongId, 0);
    const Location pastEndLocation(subsongId, 1);
    songPlayer.setCurrentlyPlayedLocations(startLocation, startLocation, pastEndLocation);
}

void AudioControllerImpl::stopPlayer() noexcept
{
    songPlayer.stop();
}

}   // namespace arkostracker
