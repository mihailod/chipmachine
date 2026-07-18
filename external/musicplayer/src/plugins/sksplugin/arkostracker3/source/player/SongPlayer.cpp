#include "SongPlayer.h"

#include "../business/song/tool/context/EffectContext.h"
#include "../business/song/tool/speed/DetermineSpeed.h"
#include "../song/CellLocationInPosition.h"
#include "../utils/PsgValues.h"
#include "../utils/message/MessageWithOneParameter.h"
#include "InstrumentPlayedInfo.h"
#include "TemperedScaleUtil.h"

namespace arkostracker 
{

SongPlayer::SongPlayer(std::shared_ptr<const Song> paramSong) noexcept :
        song(std::move(paramSong)),
        songPlayerObservers(),

        dataFromUiThreadMutex(),
        queueNewDataFromUiThread_ProtectedByMutex(),

        cellsToPlayThreadMutex(),
        cellsToPlay_ProtectedByMutex(),

        listenerNewPlayerLocationFromWorkerThread(*this),
        listenerNewPsgRegistersFromWorkerThread(*this),
        listenerNewSpeedFromWorkerThread(*this),
        listenerInstrumentPlayedFromWorkerThread(*this),

        playedLocation_AudioThread(song->getFirstSubsongId(), 0),
        playStartLocation_AudioThread(song->getFirstSubsongId(), 0),
        loopStartLocation_AudioThread(song->getFirstSubsongId(), 0),
        pastEndLocation_AudioThread(song->getFirstSubsongId(), 1),
        currentTick_AudioThread(0),
        currentSpeed_AudioThread(6),
        canReadSongLine_AudioThread(false),
        canMoveToNextLine_AudioThread(false),
        channelPlayers_AudioThread(),
        psgIndexToPendingRegistersAndSamples_AudioThread(),
        outputPsgRegisters_AudioThread(),
        isFullSongPlayed_AudioThread(),
        mustClearQueues_AudioThread(),

        psgRegisterFastObserverMutex(),
        psgRegisterFastObserver_ProtectedByMutex(),

        playing(false),
        canReadSongLine(false),
        canMoveToNextLine(false),

        offlineSongEndedCount(),
        offlineSongEndCountBeforeMuting(-1),
        offlineCurrentTick(),

        effectContext()
{
}

void SongPlayer::setSong(std::shared_ptr<const Song> newSong) noexcept
{
    stop();
    song = std::move(newSong);      // TODO Improve, shouldn't the Song be a "pending data" so that the audio thread can set it when it can? This would correct some assertions.
}

bool SongPlayer::isPlaying() const noexcept
{
    return playing;
}

void SongPlayer::play(const OptionalValue<Location>& newStartLocation, const Location& newLoopStartLocation, const Location& newPastEndLocation, const bool stopSounds,
                      const bool resetTick, const bool isFullSongPlayed) noexcept
{
    // For now simply creates an object that will be read by the Audio Thread.
    auto dataFromUiThread = std::make_unique<DataFromUiThread>(newStartLocation, newLoopStartLocation,
                                                               newPastEndLocation, stopSounds, resetTick,
                                                               true, true, isFullSongPlayed);
    postDataFromUiThread(std::move(dataFromUiThread));
}

void SongPlayer::postDataFromUiThread(std::unique_ptr<DataFromUiThread> dataFromUiThread) noexcept
{
    // Gets the new "playing?" flag. It will be used by the UI.
    // It is important to get it right, because actions refer to it to know if they must be performed. PlayLine, and "goto other pattern" for example.
    bool newIsPlaying;      // If any of the two flags is absent, gets them from the protected variables.           // NOLINT(*-init-variables)
    if (dataFromUiThread->canReadSongLine.isAbsent() || dataFromUiThread->canMoveToNextLine.isAbsent()) {
        newIsPlaying = canReadSongLine && canMoveToNextLine;
    } else {
        newIsPlaying = (dataFromUiThread->canReadSongLine.getValue() && dataFromUiThread->canMoveToNextLine.getValue());
    }

    playing = newIsPlaying;             // This is synchronous, and maybe a design flaw (see LinkerControllerImpl::onUserChangedSubsong)...
    //DBG("postDataFromUiThread is playing " + (newIsPlaying ? juce::String("true") : juce::String("false")));

    // Posts the object for the audio thread to read it.
    const std::lock_guard lock(dataFromUiThreadMutex);         // Lock!
    queueNewDataFromUiThread_ProtectedByMutex.push(*dataFromUiThread);
    jassert(queueNewDataFromUiThread_ProtectedByMutex.size() < 10U);        // A lot of things enqueued!
}

void SongPlayer::playCell(const Cell& cell, int channelIndex, const OptionalValue<CellLocationInPosition>& location) noexcept
{
    // Pushes a cell to play.
    auto cellToPlay = std::make_unique<CellToPlay>(cell, channelIndex, location);

    const std::lock_guard lock(cellsToPlayThreadMutex);                    // Lock!
    cellsToPlay_ProtectedByMutex.push_back(std::move(cellToPlay));
}

void SongPlayer::playLine(const Location& location) noexcept
{
    // For now simply creates an object that will be read by the Audio Thread.
    auto dataFromUiThread = std::make_unique<DataFromUiThread>(location, OptionalValue<Location>(), OptionalValue<Location>(),
            false, true, true, false, false);
    postDataFromUiThread(std::move(dataFromUiThread));
}

void SongPlayer::setCurrentlyPlayedLocations(const Location& newStartLocation, const Location& newLoopStartLocation, const Location& newPastEndLocation) noexcept
{
    //DBG("setCurrentlyPlayedLocations method");

    auto dataFromUiThread = std::make_unique<DataFromUiThread>(newStartLocation, newLoopStartLocation, newPastEndLocation, false, false,
                                                               OptionalBool(), OptionalBool(),
                                                               OptionalBool());

    // Posts the object for the audio thread to read it.
    postDataFromUiThread(std::move(dataFromUiThread));
}

Location SongPlayer::getCurrentLocationInSongOffline() const noexcept
{
    // Non-real-time only!!
    return playedLocation_AudioThread;
}

std::vector<std::unique_ptr<ChannelPlayerResults>> SongPlayer::getResultsOffline() const noexcept
{
    // Non-real-time only!!
    std::vector<std::unique_ptr<ChannelPlayerResults>> results;

    // Extracts the data of each ChannelPlayer.
    for (const auto& channelPlayer : channelPlayers_AudioThread) {
        auto channelPlayerResults = channelPlayer->getResults();
        // As a quick hack, the speed and tick is set on each item.
        channelPlayerResults->setSpeed(currentSpeed_AudioThread);
        channelPlayerResults->setTick(offlineCurrentTick);

        results.push_back(std::move(channelPlayerResults));
    }

    return results;
}

void SongPlayer::setPlayedLocation(const Location& location) noexcept
{
    // Only changes the current location.
    auto dataFromUiThread = std::make_unique<DataFromUiThread>(location, OptionalValue<Location>(), OptionalValue<Location>(),
                                                               false, false, true, true,
                                                               OptionalBool());
    // Posts the object for the audio thread to read it.
    postDataFromUiThread(std::move(dataFromUiThread));
}

void SongPlayer::stop() noexcept
{
    auto dataFromUiThread = std::make_unique<DataFromUiThread>(OptionalValue<Location>(), OptionalValue<Location>(),
                                                               OptionalValue<Location>(), true, true,
                                                                       false, false, false);
    postDataFromUiThread(std::move(dataFromUiThread));
}

Observers<SongPlayerObserver>& SongPlayer::getSongPlayerObservers() noexcept
{
    return songPlayerObservers;
}

void SongPlayer::setPsgRegistersObserverAudioThread(PsgRegistersObserverAudioThread* observer) noexcept
{
    const std::lock_guard lock(psgRegisterFastObserverMutex);       // Lock!
    // If new observer is not null, must not assert, else it means an observer has not unregistered!
    jassert((observer == nullptr) || ((observer != nullptr) && (psgRegisterFastObserver_ProtectedByMutex == nullptr)));
    psgRegisterFastObserver_ProtectedByMutex = observer;
}

void SongPlayer::setOfflineSongEndCountBeforeMuting(const int loopCount) noexcept
{
    jassert(loopCount > 0);
    offlineSongEndedCount = 0;
    offlineSongEndCountBeforeMuting = loopCount;
}

bool SongPlayer::hasOfflineSongEndCountReached() const noexcept
{
    return (offlineSongEndedCount >= offlineSongEndCountBeforeMuting);
}

void SongPlayer::setEffectContext(EffectContext* newEffectContext) noexcept
{
    effectContext = newEffectContext;
}

// --------------------------------------------------------
// The following methods must be called only from Audio Thread!


void SongPlayer::resizeChannelPlayersIfNeeded() noexcept
{
    const auto expectedChannelCount = song->getChannelCount(playedLocation_AudioThread.getSubsongId());
    auto channelPlayerCount = static_cast<int>(channelPlayers_AudioThread.size());
    if (channelPlayerCount == expectedChannelCount) {
        return;
    }

    if (channelPlayerCount > expectedChannelCount) {
        // Too many channel players! Removes them.
        channelPlayers_AudioThread.resize(static_cast<const size_t>(expectedChannelCount));
    } else {
        // Not enough channel players.
        while (channelPlayerCount < expectedChannelCount) {
            const auto channelIndexInSong = channelPlayerCount;

            auto channelPlayer = std::make_unique<ChannelPlayer>(*this, channelIndexInSong);
            channelPlayers_AudioThread.push_back(std::move(channelPlayer));

            ++channelPlayerCount;
        }
    }

    jassert(expectedChannelCount == static_cast<int>(channelPlayers_AudioThread.size()));
}


// PsgRegistersProvider method implementations.
// ======================================================

std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> SongPlayer::getNextRegisters(const int psgIndexToReturnDataTo) noexcept
{
    // This is called from an Audio Thread!
    // ---------------------------------------------

    auto localSong = song;          // To be sure it won't be GCed during this method.
    auto hasSpeedChanged = false;

    // Any new data from the UI thread?

    //DBG("getNextRegisters, PSG " + juce::String(psgIndexToReturnDataTo));

    auto mustStopSounds = false;
    OptionalValue<Location> locationIfEffectContext;
    {
        const std::lock_guard lock(dataFromUiThreadMutex);
        if (!queueNewDataFromUiThread_ProtectedByMutex.empty()) {
            const auto dataFromUi = queueNewDataFromUiThread_ProtectedByMutex.front();
            queueNewDataFromUiThread_ProtectedByMutex.pop();

            // Sets the played location, if given.
            if (dataFromUi.newStartLocation.isPresent()) {
                const auto newPlayedLocation_AudioThread = dataFromUi.newStartLocation.getValue();
                playedLocation_AudioThread = newPlayedLocation_AudioThread;
                playStartLocation_AudioThread = playedLocation_AudioThread;
                locationIfEffectContext = playedLocation_AudioThread;
            }
            if (dataFromUi.newLoopStartLocation.isPresent()) {
                loopStartLocation_AudioThread = dataFromUi.newLoopStartLocation.getValue();
            }
            if (dataFromUi.newPastEndLocation.isPresent()) {
                pastEndLocation_AudioThread = dataFromUi.newPastEndLocation.getValue();
            }
            if (const auto isFullSongPlayed = dataFromUi.isFullSongPlayed; dataFromUi.isFullSongPlayed.isPresent()) {
                isFullSongPlayed_AudioThread = isFullSongPlayed.getValue();
            }

            // Stops the sounds?
            mustStopSounds = dataFromUi.stopSounds;
            if (mustStopSounds) {
                {
                    const std::lock_guard cellsLock(cellsToPlayThreadMutex);
                    cellsToPlay_ProtectedByMutex.clear();
                }
                for (auto& channelPlayer : channelPlayers_AudioThread) {
                    channelPlayer->postStopSound();
                }
                outputPsgRegisters_AudioThread.clear();

                // Corrects the desync between 15 chans 200hz to 9 chan 12hz (with psg 1 at 2mhz).
                // If not enough, put this on every action.
                mustClearQueues_AudioThread = true;
            }
            if (dataFromUi.resetTick) {
                currentTick_AudioThread = 0;
            }

            if (dataFromUi.canReadSongLine.isPresent()) {
                canReadSongLine_AudioThread = dataFromUi.canReadSongLine.getValue();
            }
            if (dataFromUi.canMoveToNextLine.isPresent()) {
                canMoveToNextLine_AudioThread = dataFromUi.canMoveToNextLine.getValue();
            }

            // Determines the Speed automatically from the current Location.
            const auto newSpeed = DetermineSpeed::determineSpeed(*localSong, playedLocation_AudioThread);
            if (currentSpeed_AudioThread != newSpeed) {
                currentSpeed_AudioThread = newSpeed;
                hasSpeedChanged = true;
            }
        }

        // Sets the variables for the UI.
        canReadSongLine = canReadSongLine_AudioThread;
        canMoveToNextLine = canMoveToNextLine_AudioThread;
    }

    // The Subsong may not exist anymore (load new song).
    const auto currentSubsongId = playedLocation_AudioThread.getSubsongId();
    if (!localSong->doesSubsongIdExist(currentSubsongId)) {
        // Small hack to correct a bug when switching to another song with different initial speed, the new speed
        // was not sent because not considered changed.
        currentSpeed_AudioThread = 0;
        return { std::make_unique<PsgRegisters>(), std::make_unique<SampleData>() };
    }

    // Enough Channel Players? The Subsong PSG might have been resized.
    resizeChannelPlayersIfNeeded();

    const auto currentTick = currentTick_AudioThread;
    auto currentSpeed = currentSpeed_AudioThread;
    const auto isFirstTick = (currentTick == 0);
    const auto stillWithinLine = (currentTick < currentSpeed);

    SamplePlayInfo digidrumSamplePlayInfo;      // Will be filled if a digidrum is found.
    const auto digiChannel = localSong->getDigiChannel(currentSubsongId);
    const auto channelCount = static_cast<int>(channelPlayers_AudioThread.size());
    jassert(digiChannel < channelCount);
    const auto psgCount = PsgValues::getPsgCount(channelCount);

    // Security.
    if (psgIndexToReturnDataTo >= psgCount) {
        jassertfalse;       // May happen when removing a PSG.
        return { std::make_unique<PsgRegisters>(), std::make_unique<SampleData>() };
    }

    // To take care of all possible desync, if the PSG count has changed, deletes all the queues.
    if (psgCount != static_cast<int>(psgIndexToPendingRegistersAndSamples_AudioThread.size())) {
        mustClearQueues_AudioThread = true;
    }

    if ((psgIndexToReturnDataTo == 0) && mustClearQueues_AudioThread) {
        // Also, when going back to the first PSG, only once thanks to the flag (on high replay rate this method is called several times for the same PSG), deletes them too.
        // This takes care of the big desync bug when going from 9-channel-200hz to 15-channel-12hz music (with the PSG >0 at 2Mhz for ex).
        psgIndexToPendingRegistersAndSamples_AudioThread.clear();
        mustClearQueues_AudioThread = false;
        //DBG("CLEARING THE QUEUES");
    }

    // Then, as a convenience, creates the queues if they don't exist...
    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        if (psgIndexToPendingRegistersAndSamples_AudioThread.find(psgIndex) == psgIndexToPendingRegistersAndSamples_AudioThread.cend()) {
            std::queue<std::pair<PsgRegisters, std::unique_ptr<SampleData>>> queue;
            psgIndexToPendingRegistersAndSamples_AudioThread.insert({ psgIndex, std::move(queue) });
        }
    }

    // Looped?
    if (isFirstTick && (playedLocation_AudioThread == loopStartLocation_AudioThread)) {
        // Simple mechanism for stopping the sound if offline generation. After a certain loop of the song is reached, don't produce sound anymore.
        if ((offlineSongEndCountBeforeMuting > 0) && (offlineSongEndedCount >= offlineSongEndCountBeforeMuting)) {
            return { std::make_unique<PsgRegisters>(), std::make_unique<SampleData>() };
        }

        // We must retrig the effect context (if not already set, which is done only when a new instruction is given above).
        if (locationIfEffectContext.isAbsent() && playing && !isFullSongPlayed_AudioThread) {
            locationIfEffectContext = playedLocation_AudioThread;
        }
    }

    // If the queue for the is empty, generates the registers for ALL the PSGs.
    // This allows to synchronize all the PSGs between them, because the current method will be called
    // at different moment, possibly several times in a row for a PSG (more so if the replay frequency is high).
    // And possibly a PSG can be called twice (and the another one called twice too to compensate), so the trick to test for psgIndex 0 does not work.
    // (test case with 2 PSGs, the first at 1 mHz, the second at 2 mHz).
    auto& targetPsgQueue = psgIndexToPendingRegistersAndSamples_AudioThread.find(psgIndexToReturnDataTo)->second; // It has been filled before, it is never absent.
    if (targetPsgQueue.empty()) {
        auto event = 0;             // Externalized, because used below.

        // First, reads the notes and posts them (but only if we are on the first tick).
        if (isFirstTick && canReadSongLine_AudioThread) {

            // Speed cell?
            const auto speedCell = localSong->getSpecialCell(true, playedLocation_AudioThread);
            if (!speedCell.isEmpty()) {
                currentSpeed = speedCell.getValue();
                if (currentSpeed_AudioThread != currentSpeed) {
                    currentSpeed_AudioThread = currentSpeed;
                    hasSpeedChanged = true;
                }
            }

            // Reads the notes of each channel.
            for (auto channelIndexInSong = 0; channelIndexInSong < channelCount; ++channelIndexInSong) {
                const auto cell = localSong->getCell(playedLocation_AudioThread, channelIndexInSong);

                // Enqueues the Cell, if it has any data. See the comments just below about this condition.
                const auto useEffectContext = (locationIfEffectContext.isPresent() && cell.isNoteAndInstrument());
                if (!cell.isEmpty() || useEffectContext) {
                    auto& channelPlayer = channelPlayers_AudioThread.at(static_cast<size_t>(channelIndexInSong));
                    // Creates a CellToPlay. By default, no location, else an effect context will be done every time on every note.
                    // However, can be set if actually wanting to have an effect context, such as when playing a line.
                    // The note and instrument presence is needed, else the arps and pitches will be retrigged every time
                    // a Play Line is done.
                    OptionalValue<CellLocationInPosition> cellLocationInPositionIfEffectContext;
                    if (useEffectContext) {
                        const auto& location = locationIfEffectContext.getValueRef();
                        cellLocationInPositionIfEffectContext = CellLocationInPosition(currentSubsongId, location.getPosition(),
                            location.getLine(), channelIndexInSong);
                    }

                    const CellToPlay cellToPlay(cell, channelIndexInSong, cellLocationInPositionIfEffectContext);
                    channelPlayer->postCell(cellToPlay);
                }
            }

            // Event cell?
            const auto eventCell = localSong->getSpecialCell(false, playedLocation_AudioThread);
            if (!eventCell.isEmpty()) {
                // A new event is found. Is there a sample of the same number?
                event = eventCell.getValue();
                const auto psgIndex = PsgValues::getPsgIndex(digiChannel);
                // Gets the reference frequency.
                auto referenceFrequencyHz = PsgFrequency::defaultReferenceFrequencyHz;  // Default in case of failure.
                localSong->performOnConstSubsong(currentSubsongId, [&](const Subsong& subsong) {
                    if (psgIndex < subsong.getPsgCount()) {
                        const auto& psg = subsong.getPsgRefs().at(static_cast<size_t>(psgIndex));
                        referenceFrequencyHz = psg.getReferenceFrequency();
                    } else {
                        jassertfalse;       // Psg index is invalid! Digichannel is out of bounds!
                    }
                });
                // Nothing is done if the instrument doesn't exist.
                const auto instrumentIdOptional = localSong->findInstrumentIdFromEvent(event);
                if (instrumentIdOptional.isPresent()) {
                    localSong->performOnConstInstrument(instrumentIdOptional.getValue(), [&](const Instrument& instrument) noexcept {
                        if (instrument.getType() == InstrumentType::sampleInstrument) {
                            const auto& samplePart = instrument.getConstSamplePart();
                            const auto digidrumPitchHz = TemperedScaleUtil::getSampleFrequency(samplePart.getDigidrumNote(), referenceFrequencyHz);

                            // Fills the Digisample with a high-priority sample, full volume.
                            digidrumSamplePlayInfo = SamplePlayInfo(
                                    samplePart.getSample(), true, samplePart.getAmplificationRatio(),
                                    samplePart.getLoop().withLooping(false),
                                    samplePart.getFrequencyHz(),
                                    digidrumPitchHz,
                                    referenceFrequencyHz,
                                    15, true);
                        }
                    });
                }
            }

            // Notifies the new position to the listener on the UI thread (thanks to a JUCE message).
            const SongPlayerObserver::Locations locations(playStartLocation_AudioThread, loopStartLocation_AudioThread, pastEndLocation_AudioThread, playedLocation_AudioThread);
            auto* message = new MessageWithOneParameter(locations);      // Ref-counted by JUCE.
            listenerNewPlayerLocationFromWorkerThread.postMessage(message);
            //DBG("New cell on position " + juce::String(playedLocation_AudioThread.getPosition()) + ", line " + juce::String(playedLocation_AudioThread.getLine()));
        }

        // Gets the possible posted Cells from the user, and play them. They are "tick-agnostic" (can be played at any time - played by the user via a keyboard).
        {
            const std::lock_guard lock(cellsToPlayThreadMutex);
            for (const auto& cellToPlay : cellsToPlay_ProtectedByMutex) {
                const auto channelIndexInSong = cellToPlay->getChannelIndex();
                if (channelIndexInSong >= channelCount) {
                    jassertfalse;           // Should never happen! Trying to play a Cell outside the channel count limit!
                    continue;
                }

                // Enqueues the Cell. It may contain a Location for the Effect Context feature.
                auto& channelPlayer = channelPlayers_AudioThread.at(static_cast<const size_t>(channelIndexInSong));
                channelPlayer->postCell(*cellToPlay);
            }

            cellsToPlay_ProtectedByMutex.clear();       // Cleared, now that they are played.
        }

        // Plays each PSG.
        std::unordered_map<int, std::pair<OptionalId, int>> channelIndexToInstrumentAndPlayedIndex;
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            // Plays the streams of the channel of the PSG, and agglomerates the result.
            //auto outputPsgRegisters = std::make_unique<PsgRegisters>();     // Empty registers at first, will be filled just below.
            auto outputSampleDataToPlay = std::make_unique<SampleData>();

            const auto firstChannelIndex = PsgValues::getChannelIndex(0, psgIndex);     // There can be several PSGs!
            const auto pastLastChannelIndex = firstChannelIndex + PsgValues::channelCountPerPsg;
            auto channelIndexInPsg = 0;
            for (auto channelIndexAbsolute = firstChannelIndex; channelIndexAbsolute < pastLastChannelIndex; ++channelIndexAbsolute) {
                // Plays the stream.
                auto& channelPlayer = channelPlayers_AudioThread.at(static_cast<size_t>(channelIndexAbsolute));
                const auto channelResult = channelPlayer->playStream(isFirstTick, stillWithinLine);
                jassert(channelResult != nullptr);

                // Adds the instruments being played in a map.
                if (const auto& instrumentId = channelResult->getInstrumentId(); instrumentId.isPresent()) {
                    channelIndexToInstrumentAndPlayedIndex.insert({
                        channelIndexAbsolute, {
                            instrumentId.getValueRef(), channelResult->getPlayedIndexInInstrument()
                        }
                    });
                }

                // Mixes the channel received registers to the PSG Registers.
                fillWithChannelResult(channelIndexInPsg, outputPsgRegisters_AudioThread, *outputSampleDataToPlay, *channelResult, event);

                ++channelIndexInPsg;
            }

            // Stores the PSGs in the queue.
            auto& psgQueue = psgIndexToPendingRegistersAndSamples_AudioThread.find(psgIndex)->second;     // Always present, checked before.
            if (psgQueue.size() < 50U) {            // Security.
                psgQueue.emplace(outputPsgRegisters_AudioThread, std::move(outputSampleDataToPlay));
                jassert(psgQueue.size() < 20U);     // Log in case of hiccups. Did you read the data from ALL the PSGs?
            } else {
                jassertfalse;                       // Wow, shouldn't happen!
            }
        }

        // Sends what instrument is being played. Useful for the IE to highlight the played index.
        const InstrumentPlayedInfo messageInstrumentPlayed(channelIndexToInstrumentAndPlayedIndex);
        notifyListenersForInstrumentPlayed(messageInstrumentPlayed);

        // Moves forward in the Song.
        advanceInSong(localSong);

        // Notifies the new PSG registers (of all the PSGs) to the possible listeners on the UI thread (thanks to a JUCE message).
        notifyListenersFromNewPsgRegisters();
    }

    // Notifies for the new speed, if any.
    if (hasSpeedChanged) {
        auto* message = new MessageWithOneParameter(currentSpeed);      // Ref-counted by JUCE.
        listenerNewSpeedFromWorkerThread.postMessage(message);
    }

    // Only returns the PSG registers of the targeted PSG.
    if (targetPsgQueue.empty()) {
        DBG("STARVATION for PSG " + juce::String(psgIndexToReturnDataTo));
        jassertfalse;   // Should NEVER happen!!! It means we either never produced values, or read too much of it! Did you read the data from ALL the PSGs?
                        // Ok... May happen when changing song.
        mustClearQueues_AudioThread = true;
        return { std::make_unique<PsgRegisters>(), std::make_unique<SampleData>() };
    }

    // Gets the registers, clears the entry.
    auto& [innerOutputPsgRegisters, innerOutputSampleDataToPlay] = targetPsgQueue.front();
    auto outputPsgRegisters = std::make_unique<PsgRegisters>(innerOutputPsgRegisters);
    auto outputSampleDataToPlay = std::move(innerOutputSampleDataToPlay);
    targetPsgQueue.pop();

    // Adds the Event sample, if any, overwriting the possibly stored sample.
    outputSampleDataToPlay->addSamplePlayInfoIfNotEmpty(digiChannel, digidrumSamplePlayInfo);

    // If the sounds must be stopped, fills the unused slots with a stop. We must not overwrite anything because
    // the "stop sound" is also set when start playing (to stop the already existing sound).
    if (mustStopSounds) {
        outputSampleDataToPlay->fillUnusedWithStops(channelCount);
    }

    return { std::move(outputPsgRegisters), std::move(outputSampleDataToPlay) };
}

void SongPlayer::fillWithChannelResult(const int targetChannel, PsgRegisters& psgRegistersToFill, SampleData& sampleDataToFill,
                                       const ChannelPlayerResults& inputChannelPlayerResults, const int event) noexcept
{
    jassert((targetChannel >= 0) && (targetChannel <= 2));

    psgRegistersToFill.setEventNumber(event);

    // Manages the possible sample.
    const auto& inputSampleData = inputChannelPlayerResults.getSamplePlayInfo();
    sampleDataToFill.addSamplePlayInfoIfNotEmpty(targetChannel, inputSampleData);     // Nothing is added if there is no data.

    // Manages the PSG result.
    const auto& inputChannelRegisters = inputChannelPlayerResults.getChannelOutputRegisters();

    // The software frequency.
    psgRegistersToFill.setSoftwarePeriod(targetChannel, inputChannelRegisters.getSoftwarePeriod());

    // Noise?
    const auto noise = inputChannelRegisters.getNoise();
    const auto isNoise = (noise > 0);
    if (isNoise) {
        psgRegistersToFill.setNoise(noise);             // Sets the noise, as this Channel uses it. Last one entering this method gets priority!
    }
    psgRegistersToFill.setMixerNoiseState(targetChannel, isNoise);

    // Sound?
    const auto soundType = inputChannelRegisters.getSoundType();
    bool isSound;    // NOLINT(*-init-variables)
    switch (soundType) {
        default:
            jassertfalse;               // Unhandled case??
        case SoundType::noSoftwareNoHardware:
            [[fallthrough]];
        case SoundType::hardwareOnly:
            isSound = false;
            break;
        case SoundType::softwareOnly:
            [[fallthrough]];
        case SoundType::softwareAndHardware:
            isSound = true;
            break;
    }
    psgRegistersToFill.setMixerSoundState(targetChannel, isSound);

    // The volume.
    const auto volume0To16 = inputChannelRegisters.getVolume();
    psgRegistersToFill.setVolume(targetChannel, volume0To16);

    // Any hardware volume?
    switch (soundType) {
        case SoundType::hardwareOnly: [[fallthrough]];
        case SoundType::softwareAndHardware:
        {
            const auto hardwarePeriod = inputChannelRegisters.getHardwarePeriod();
            const auto hardwareEnvelope = inputChannelRegisters.getHardwareEnvelope();
            const auto retrig = inputChannelRegisters.isRetrig();
            psgRegistersToFill.setHardwarePeriod(hardwarePeriod);
            psgRegistersToFill.setHardwareEnvelopeAndRetrig(hardwareEnvelope, retrig);
            break;
        }
        case SoundType::noSoftwareNoHardware: [[fallthrough]];
        case SoundType::softwareOnly: [[fallthrough]];
        default:
            // Nothing to do.
            break;
    }

    // SID.
    const auto specialEffect = inputChannelRegisters.getSpecialEffect();
    psgRegistersToFill.setSpecialEffect(targetChannel, specialEffect);
}

void SongPlayer::advanceInSong(const std::shared_ptr<const Song>& localSong) noexcept
{
    // This is called from a method called from the Audio Thread!
    // ----------------------------------------------------------

    const auto subsongId = playedLocation_AudioThread.getSubsongId();
    auto currentPatternHeight = 0;
    localSong->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        currentPatternHeight = subsong.getPositionHeight(playedLocation_AudioThread.getPosition());
    });

    auto currentPosition = playedLocation_AudioThread.getPosition();
    auto currentLine = playedLocation_AudioThread.getLine();
    offlineCurrentTick = currentTick_AudioThread;       // Else, we retrieve an incremented value.

    if (++currentTick_AudioThread >= currentSpeed_AudioThread) {              // The stored speed is from 1-255.
        if (!canMoveToNextLine_AudioThread) {
            // We should move to the next line, but we don't have the right. So, don't do anything.
            currentTick_AudioThread = 0xffff;                               // A large number that the speed can't reach, just for it not to grow uselessly.
        } else {
            // Next line.
            currentTick_AudioThread = 0;

            // End of pattern?
            if (++currentLine >= currentPatternHeight) {
                // Goes to the next one, if possible.
                currentLine = 0;
                ++currentPosition;
            }

            // Still a valid position?
            if (const auto newCurrentLocation = Location(subsongId, currentPosition, currentLine); newCurrentLocation < pastEndLocation_AudioThread) {
                playedLocation_AudioThread = newCurrentLocation;
            } else {
                // The end is reached. Must loop!
                playedLocation_AudioThread = loopStartLocation_AudioThread;
                ++offlineSongEndedCount;
            }
        }
    }
}



// SongDataProvider method implementations.
// ======================================================

// ALL THESE METHODS are called from the Audio Thread!
// ----------------------------------------------------------

OptionalId SongPlayer::getInstrumentIdFromAudioThread(const int instrumentIndex) const noexcept
{
    OptionalId id;
    song->performOnConstInstrumentFromIndex(instrumentIndex, [&](const Instrument& instrument) noexcept {
        id = instrument.getId();
    });

    return id;
}

InstrumentType SongPlayer::getInstrumentTypeFromAudioThread(const OptionalId& instrumentId) const noexcept
{
    auto type = InstrumentType::psgInstrument;
    if (instrumentId.isPresent()) {
        song->performOnConstInstrument(instrumentId.getValueRef(), [&](const Instrument& instrument) noexcept {
            type = instrument.getType();
        });
    }

    return type;
}


OptionalId SongPlayer::getExpressionIdFromAudioThread(const bool isArpeggio, const int expressionIndex) const noexcept
{
    const auto& expressionHandler = song->getConstExpressionHandler(isArpeggio);
    return expressionHandler.find(expressionIndex);
}

SongDataProvider::ExpressionMetadata SongPlayer::getExpressionMetadataFromAudioThread(const bool isArpeggio, const OptionalId& expressionId) const noexcept
{
    auto endIndex = 0;
    auto loopStartIndex = 0;
    auto speed = 0;
    const auto& expressionHandler = song->getConstExpressionHandler(isArpeggio);
    if (expressionId.isPresent()) {
        expressionHandler.performOnConstExpression(expressionId.getValueRef(), [&](const Expression& expression) noexcept {
            loopStartIndex = expression.getLoopStart(true);
            endIndex = expression.getEnd(true);
            speed = expression.getSpeed();
        });
    }

    return { speed, loopStartIndex, endIndex };
}

SongDataProvider::PsgInstrumentFrameData SongPlayer::getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, const int cellIndex) const noexcept
{
    // Initializes values in case of failure.
    Loop loop;
    auto speed = 0;
    auto isInstrumentRetrig = false;
    LowLevelPsgInstrumentCell cell;
    if (instrumentId.isPresent()) {
        song->performOnConstPsgInstrument(instrumentId.getValueRef(), [&](const Instrument& /*instrument*/, const PsgPart& psgPart) noexcept {
            speed = psgPart.getSpeed();
            loop = psgPart.getMainLoop();
            isInstrumentRetrig = psgPart.isInstrumentRetrig();
            cell = psgPart.buildLowLevelCell(cellIndex);
        });
    }

    return { loop, speed, isInstrumentRetrig, cell };
}

SongDataProvider::SampleInstrumentFrameData SongPlayer::getSampleInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId) const noexcept
{
    // Initializes values in case of failure.
    Loop loop;
    auto frequencyHz = PsgFrequency::defaultSampleFrequencyHz;
    auto amplificationRatio = 1.0F;
    std::shared_ptr<Sample> sample = nullptr;

    if (instrumentId.isPresent()) {
        song->performOnConstSampleInstrument(instrumentId.getValueRef(), [&](const Instrument& /*instrument*/, const SamplePart& samplePart) noexcept {
            loop = samplePart.getLoop();
            frequencyHz = samplePart.getFrequencyHz();
            amplificationRatio = samplePart.getAmplificationRatio();
            sample = samplePart.getSample();
        });
    }

    return { loop, frequencyHz, amplificationRatio, sample };
}

std::pair<int, float> SongPlayer::getPsgFrequencyFromChannelFromAudioThread(const int channelIndexInSong) const noexcept
{
    std::pair psgAndRefFrequencies = { PsgFrequency::psgFrequencyCPC, PsgFrequency::defaultReferenceFrequencyHz};   // Default if failure.
    song->performOnConstSubsong(playedLocation_AudioThread.getSubsongId(), [&] (const Subsong& subsong) {
        const auto& psgs = subsong.getPsgRefs();

        const auto psgIndex = static_cast<size_t>(PsgValues::getPsgIndex(channelIndexInSong));
        // Finds the PSG if possible, else the default values will be used.
        if (psgIndex < psgs.size()) {
            const auto& psg = psgs.at(psgIndex);
            psgAndRefFrequencies = { psg.getPsgFrequency(), psg.getReferenceFrequency() };
        }
    });

    return psgAndRefFrequencies;
}

SidPlayerCapability SongPlayer::getSidPlayerCapability() const noexcept
{
    auto sidPlayerCapability = SidPlayerCapability::buildDefault();
    song->performOnConstSubsong(playedLocation_AudioThread.getSubsongId(), [&] (const Subsong& subsong) {
        sidPlayerCapability = subsong.getSidPlayerCapability();
    });

    return sidPlayerCapability;
}

int SongPlayer::getTranspositionFromAudioThread(const int channelIndexInSong) const noexcept
{
    auto transposition = 0;         // Default if failure.

    const auto subsongId = playedLocation_AudioThread.getSubsongId();
    const auto position = playedLocation_AudioThread.getPosition();
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        transposition = subsong.getPositionRef(position).getTransposition(channelIndexInSong);
    });

    return transposition;
}

bool SongPlayer::isEffectContextEnabled() const noexcept
{
    return (effectContext != nullptr);
}

LineContext SongPlayer::determineEffectContextFromAudioThread(const CellLocationInPosition location) const noexcept
{
    jassert(isEffectContextEnabled());      // Method shouldn't be called!

    if (effectContext == nullptr) {
        return { };
    }

    return effectContext->determineContext(location);
}

LineContext SongPlayer::determineEffectContextFromAudioThread(const int channelIndexInSong) const noexcept
{
    jassert(isEffectContextEnabled());      // Method shouldn't be called!

    const auto subsongId = playedLocation_AudioThread.getSubsongId();
    const auto position = playedLocation_AudioThread.getPosition();
    const auto lineIndex = playedLocation_AudioThread.getLine();

    const CellLocationInPosition location(subsongId, position, lineIndex, channelIndexInSong);
    return determineEffectContextFromAudioThread(location);
}


// ========================================================

void SongPlayer::notifyListenersFromNewPsgRegisters() const noexcept
{
    // Audio thread.

    // Copies the original map (gets rid of the unique_ptr).
    std::unordered_map<int, std::pair<PsgRegisters, SampleData>> psgIndexToRegistersAndSamples; // The map to fill.

    for (const auto& entry : psgIndexToPendingRegistersAndSamples_AudioThread) {
        const auto psgIndex = entry.first;
        if (const auto& queue = entry.second; !queue.empty()) {
            const auto& [psgRegisters, sampleData] = queue.front();
            psgIndexToRegistersAndSamples.insert(std::make_pair(psgIndex, std::make_pair(psgRegisters, *sampleData)));
        }
    }

    // Fast observer?
    PsgRegistersObserverAudioThread* fastObserver;          // NOLINT(*-init-variables)
    {
        const std::lock_guard lock(psgRegisterFastObserverMutex);         // Lock!
        fastObserver = psgRegisterFastObserver_ProtectedByMutex;
    }
    if (fastObserver != nullptr) {
        fastObserver->onNewPsgRegistersOnAudioThread(psgIndexToRegistersAndSamples);
    }

    // Normal observers.
    auto* message = new MessageWithOneParameter(psgIndexToRegistersAndSamples);  // Ref-counted by JUCE.
    listenerNewPsgRegistersFromWorkerThread.postMessage(message);
}

void SongPlayer::notifyListenersForInstrumentPlayed(const InstrumentPlayedInfo& messageInstrumentPlayed) const noexcept
{
    // Audio thread.

    auto* message = new MessageWithOneParameter(messageInstrumentPlayed);  // Ref-counted by JUCE.
    listenerInstrumentPlayedFromWorkerThread.postMessage(message);
}

int SongPlayer::getExpressionValueFromAudioThread(const bool isArpeggio, const OptionalId& expressionId, const int cellIndex) const noexcept
{
    auto value = 0;
    if (expressionId.isAbsent()) {
        return value;           // Happens every time, when no ID is given.
    }

    const auto& expressionHandler = song->getConstExpressionHandler(isArpeggio);

    expressionHandler.performOnConstExpression(expressionId.getValueRef(), [&](const Expression& expression) noexcept {
        value = expression.getValue(cellIndex, true);
    });

    return value;
}


// MessageListenerNewPlayerLocationFromWorkerThread
// --------------------------------------------------------

void SongPlayer::MessageListenerNewPlayerLocationFromWorkerThread::handleMessage(const juce::Message& message)
{
    // We are back on the UI thread.
    const auto& rightMessage = dynamic_cast<const MessageWithOneParameter<SongPlayerObserver::Locations>&>(message);
    const auto& data = rightMessage.getData();
    // Notifies each possible listener.
    parentObject.songPlayerObservers.applyOnObservers([&] (SongPlayerObserver* observer) noexcept {
        observer->onPlayerNewLocations(data);
    });
}


// MessageListenerNewPsgRegistersFromWorkerThread
// --------------------------------------------------------

void SongPlayer::MessageListenerNewPsgRegistersFromWorkerThread::handleMessage(const juce::Message& message)
{
    // We are back on the UI thread.
    const auto& rightMessage = dynamic_cast<const MessageWithOneParameter<std::unordered_map<int, std::pair<PsgRegisters, SampleData>>>&>(message);
    const auto& data = rightMessage.getData();
    // Notifies each possible listener.
    parentObject.songPlayerObservers.applyOnObservers([&] (SongPlayerObserver* observer) noexcept {
        observer->onNewPsgRegisters(data);
    });
}


// MessageListenerNewSpeedFromWorkerThread
// --------------------------------------------------------
void SongPlayer::MessageListenerNewSpeedFromWorkerThread::handleMessage(const juce::Message& message)
{
    // We are back on the UI thread.
    const auto& rightMessage = dynamic_cast<const MessageWithOneParameter<int>&>(message);
    const auto& data = rightMessage.getData();
    // Notifies each possible listener.
    parentObject.songPlayerObservers.applyOnObservers([&] (SongPlayerObserver* observer) noexcept {
        observer->onNewSpeed(data);
    });
}

// MessageListenerInstrumentPlayedFromWorkerThread
// --------------------------------------------------------
void SongPlayer::MessageListenerInstrumentPlayedFromWorkerThread::handleMessage(const juce::Message& message)
{
    // We are back on the UI thread.
    const auto& rightMessage = dynamic_cast<const MessageWithOneParameter<InstrumentPlayedInfo>&>(message);
    const auto& data = rightMessage.getData();
    // Notifies each possible listener.
    parentObject.songPlayerObservers.applyOnObservers([&] (SongPlayerObserver* observer) noexcept {
        observer->onInstrumentPlayed(data);
    });
}

}   // namespace arkostracker
