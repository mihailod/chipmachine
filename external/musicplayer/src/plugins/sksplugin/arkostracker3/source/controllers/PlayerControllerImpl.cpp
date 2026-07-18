#include "PlayerControllerImpl.h"

#include "../app/preferences/PreferencesManager.h"
#include "../business/song/tool/context/EffectContextImpl.h"
#include "../player/SongPlayer.h"
#include "../song/cells/CellConstants.h"
#include "../utils/NoteUtil.h"
#include "../utils/NumberUtil.h"
#include "MainController.h"
#include "SongController.h"

namespace arkostracker 
{

const int PlayerControllerImpl::maximumBaseOctave = 8;
const int PlayerControllerImpl::middleOctave = 4;

PlayerControllerImpl::PlayerControllerImpl(MainController& pMainController) noexcept :
        mainController(pMainController),
        songController(pMainController.getSongController()),
        effectContext(std::make_unique<EffectContextImpl>(songController)),
        songPlayer(pMainController.getSongPlayer()),
        currentOctave(3),
        currentTestNote(3 * 12),                                // A default note.
        useSelectedArp(true),
        useSelectedPitch(true),
        useMonophony(true),
        monophonicUsesCursorPosition(false),
        monophonicChannelIndex(1),
        polyphonicChannelIndexes({ 0, 1, 2 }),                    // First PSG.
        polyphonicCounter(0U),
        cursorChannelIndex(0),
        playingOnPattern(false),
        cachedMustAddOctaveToInputMidiNote()
{
    songController.getSubsongMetadataObservers().addObserver(this);
    auto& observers = mainController.observers();
    observers.getBlockSelectionObservers().addObserver(this);
    observers.getCursorObservers().addObserver(this);
    observers.getGeneralDataObservers().addObserver(this);

    songPlayer.setEffectContext(effectContext.get());
}

PlayerControllerImpl::~PlayerControllerImpl()
{
    songController.getSubsongMetadataObservers().removeObserver(this);
    auto& observers = mainController.observers();
    observers.getBlockSelectionObservers().removeObserver(this);
    observers.getCursorObservers().removeObserver(this);
    observers.getGeneralDataObservers().removeObserver(this);

    songPlayer.setEffectContext(nullptr);
}

int PlayerControllerImpl::getCurrentOctave() const noexcept
{
    return currentOctave;
}

Observers<SongPlayerObserver>& PlayerControllerImpl::getSongPlayerObservers() noexcept
{
    return songPlayer.getSongPlayerObservers();
}

void PlayerControllerImpl::play(const OptionalInt newNote, const bool isBaseNote, const OptionalId newInstrumentId) noexcept
{
    // What is the note to play?
    const auto originalNote = newNote.isAbsent() ? currentTestNote :
            // Uses the given note. Which may be a base note!
            isBaseNote ? NoteUtil::getNote(newNote.getValue(), currentOctave) : newNote.getValue();
    // What instrument?
    const auto instrumentId = newInstrumentId.isPresent() ? newInstrumentId.getValue() : mainController.getSelectedInstrumentId();
    if (instrumentId.isAbsent()) {
        return;
    }
    const auto instrumentIndex = songController.getInstrumentIndex(instrumentId.getValueRef());
    if (instrumentIndex.isAbsent()) {
        jassertfalse;       // No instrument linked to this ID?
        return;
    }

    // Corrects, who knows what MIDI could send? (Ok, should be 7 bits...).
    const auto note = NumberUtil::correctNumber(originalNote, CellConstants::minimumNote, CellConstants::maximumNote, true);

    currentTestNote = note;

    // Puts a Reset to dismiss all other effects. Then adds the arpeggio and pitch, if wanted.
    CellEffects effects;
    auto effectIndex = 0;
    effects.setEffect(effectIndex++, CellEffect(Effect::reset));
    if (useSelectedArp) {
        const auto selectedArpeggioIdOptional = mainController.getSelectedExpressionId(true);
        if (selectedArpeggioIdOptional.isPresent()) {
            const auto arpeggioIndexOptional = songController.getExpressionIndex(true, selectedArpeggioIdOptional.getValue());
            if (arpeggioIndexOptional.isPresent()) {
                effects.setEffect(effectIndex++, CellEffect::buildFromLogicalValue(Effect::arpeggioTable, arpeggioIndexOptional.getValue()));
            } else {
                jassertfalse;       // Arpeggio does not exist anymore!
            }
        }
    }
    if (useSelectedPitch) {
        const auto selectedPitchIdOptional = mainController.getSelectedExpressionId(false);
        if (selectedPitchIdOptional.isPresent()) {
            const auto pitchIndexOptional = songController.getExpressionIndex(false, selectedPitchIdOptional.getValue());
            if (pitchIndexOptional.isPresent()) {
                effects.setEffect(effectIndex, CellEffect::buildFromLogicalValue(Effect::pitchTable, pitchIndexOptional.getValue()));
            } else {
                jassertfalse;       // Pitch does not exist anymore!
            }
        }
    }
    const Cell cell(Note::buildNote(note), instrumentIndex, effects);

    // What channel (monophonic, polyphonic?)?
    const auto channelIndex = determineChannelToUse();

    songPlayer.playCell(cell, channelIndex, { });        // No location, because the Cell already has its own forced effects.

    // Notifies the listener about the note being played.
    getSongPlayerObservers().applyOnObservers([&](SongPlayerObserver* observer) {
        observer->onNewTestNote(note);
    });
}

void PlayerControllerImpl::playFromEditor(int note, const OptionalInt instrumentIndex, const CellLocationInPosition location) noexcept
{
    // Corrects, who knows what MIDI could send? (Ok, should be 7 bits...).
    note = NumberUtil::correctNumber(note, CellConstants::minimumNote, CellConstants::maximumNote, true);

    const Cell cell(Note::buildNote(note), instrumentIndex);
    playFromEditor(cell, location);
}

void PlayerControllerImpl::playFromEditor(const Cell& cell, const CellLocationInPosition location) noexcept
{
    songPlayer.playCell(cell, location.getChannelIndex(), location);
    // Note: no listener is notified, as contrary to the Play method above (because not really useful).
}

void PlayerControllerImpl::playSongFromStart() noexcept
{
    playingOnPattern = false;
    resetStoredSelectedBlockIfEmpty();

    // No fancy algorithm, simply plays the full subsong.
    const auto subsongId = songController.getCurrentSubsongId();
    const auto [loopStartLocation, pastEndLocation] = songController.getSong()->getLoopStartAndPastEndPositions(subsongId);

    const auto startLocation = Location(loopStartLocation.getSubsongId(), 0);

    // Gives this to the player.
    songPlayer.play(startLocation, loopStartLocation, pastEndLocation, true, true, true);

    sendObserverEvent(SongPlayerObserver::Event::playFromStart);
}

void PlayerControllerImpl::playSongFromCurrentLocation() noexcept
{
    playingOnPattern = false;

    // If already playing, don't change the "flow". Useful to go from play pattern to play song for example.
    const auto playing = songPlayer.isPlaying();

    const auto resetTick = !playing;
    const auto stopSounds = resetTick;

    // Corrects the section to loop. We still want to be able to play outside of bounds (a pattern only is looped).
    // The only little flaw if that it is the SongController viewed location that is used to determine this, and not the actual location in the player.
    // Shouldn't be a problem at all though...
    const auto currentlyShownLocation = songController.getCurrentViewedLocation();
    const auto locations = getLocationsToPlayFromLocationToGoTo(currentlyShownLocation);

    // Gives this to the player.
    songPlayer.play({ }, std::get<1>(locations), std::get<2>(locations), stopSounds, resetTick, true);

    sendObserverEvent(SongPlayerObserver::Event::play);
}

void PlayerControllerImpl::playPatternFromStart() noexcept
{
    playingOnPattern = true;
    resetStoredSelectedBlockIfEmpty();

    // Starts/loop at the beginning of the current Position.
    const auto currentlyShownLocation = songController.getCurrentViewedLocation();
    const auto startLocation = currentlyShownLocation.withLine(0);
    const auto pastEndLocation = startLocation.withPosition(startLocation.getPosition() + 1);

    // Gives this to the player.
    songPlayer.play(startLocation, startLocation, pastEndLocation, true, true, false);

    sendObserverEvent(SongPlayerObserver::Event::playPatternFromStart);
}

void PlayerControllerImpl::playPatternFromShownLocationOrBlock() noexcept
{
    playingOnPattern = true;

    const auto currentlyShownLocation = songController.getCurrentViewedLocation();
    // If the block is empty, uses the current location is a starting point. This allows the global Play Pattern to start where the last viewed line was.
    // If the block is present, starts with its beginning.
    const auto startLine = selectedBlock.getStart();
    const auto endLine = selectedBlock.getEnd();
    const auto startLocation = selectedBlock.isEmpty() ? currentlyShownLocation : currentlyShownLocation.withLine(startLine);
    // Default value.
    auto loopStartLocation = startLocation;
    Location pastEndLocation;

    // Gets the height of the current Position.
    auto currentPositionHeight = TrackConstants::defaultPositionHeight;
    songController.performOnConstSubsong(currentlyShownLocation.getSubsongId(), [&](const Subsong& subsong) {
        currentPositionHeight = subsong.getPositionHeight(currentlyShownLocation.getPosition());
    });

    if ((selectedBlock.getLength() <= 1) || (startLine > currentPositionHeight) || (endLine > currentPositionHeight)) {
        // If the selected block is empty, it means the start line is actually a cursor line, from where to start.
        // If the selection is only one line, starts from here and go to the end of the pattern.
        // In both cases, loops back to the beginning of this pattern. The past end is the beginning of the next Position.
        // Also, if illegal, loops on the full pattern instead of the block. This happens when the selected block is too large for this position
        // (happens when going from one large position to another one which is smaller).
        loopStartLocation = currentlyShownLocation.withLine(0);
        pastEndLocation = loopStartLocation.withPosition(startLocation.getPosition() + 1).withLine(0);
    } else {
        pastEndLocation = currentlyShownLocation.withLine(endLine);
    }

    // Gives this to the player.
    songPlayer.play(startLocation, loopStartLocation, pastEndLocation, true, true, false);

    sendObserverEvent(SongPlayerObserver::Event::playPatternFromTopOrBlock);
}

void PlayerControllerImpl::playSongFromLocation(const Location& locationToGoTo) noexcept
{
    playingOnPattern = false;

    const auto locations = getLocationsToPlayFromLocationToGoTo(locationToGoTo);
    songPlayer.play(std::get<0>(locations), std::get<1>(locations), std::get<2>(locations), true, true, false);

    sendObserverEvent(SongPlayerObserver::Event::playPatternContinue);
}

void PlayerControllerImpl::playLine(const Location& location) noexcept
{
    // Don't change playingOnPattern.
    songPlayer.playLine(location);

    sendObserverEvent(SongPlayerObserver::Event::playLine);
}

void PlayerControllerImpl::setCurrentlyPlayedLocation(const Location& locationToGoTo) noexcept
{
    const auto locations = getLocationsToPlayFromLocationToGoTo(locationToGoTo);
    songPlayer.setCurrentlyPlayedLocations(std::get<0>(locations), std::get<1>(locations), std::get<2>(locations));
}

std::tuple<Location, Location, Location> PlayerControllerImpl::getLocationsToPlayFromLocationToGoTo(const Location& locationToGoTo) const noexcept
{
    const auto subsongId = songController.getCurrentSubsongId();
    const auto [loopStartLocation, pastEndLocation] = songController.getSong()->getLoopStartAndPastEndPositions(subsongId);

    // Are we within bounds? If yes, only changes the currently played location.
    if (locationToGoTo.getPosition() < pastEndLocation.getPosition()) {
        // Within bounds.
        return { locationToGoTo, loopStartLocation, pastEndLocation };
    }

    // Outside of bounds. Plays the pattern only.
    auto loopStartLocationPattern = locationToGoTo.withLine();
    auto pastEndLocationPattern = loopStartLocationPattern.withPosition(loopStartLocationPattern.getPosition() + 1);
    return { locationToGoTo, locationToGoTo.withLine(), pastEndLocationPattern };
}

void PlayerControllerImpl::resetStoredSelectedBlockIfEmpty() noexcept
{
    if (selectedBlock.isEmpty()) {
        selectedBlock = juce::Range(0, 0);
    }
}

void PlayerControllerImpl::stopPlaying() noexcept
{
    songPlayer.stop();

    sendObserverEvent(SongPlayerObserver::Event::stop);
}

bool PlayerControllerImpl::isPlaying() const noexcept
{
    return songPlayer.isPlaying();
}

bool PlayerControllerImpl::isPlayingOnPattern() const noexcept
{
    return playingOnPattern;
}

void PlayerControllerImpl::setPlayedLocation(const Location& location) noexcept
{
    songPlayer.setPlayedLocation(location);
}

void PlayerControllerImpl::setOctave(const int desiredOctave) noexcept
{
    currentOctave = NumberUtil::correctNumber(desiredOctave, 0, maximumBaseOctave);
}

bool PlayerControllerImpl::isSelectedArpUsedWhenPlayingNote() const noexcept
{
    return useSelectedArp;
}

bool PlayerControllerImpl::isSelectedPitchUsedWhenPlayingNote() const noexcept
{
    return useSelectedPitch;
}

bool PlayerControllerImpl::isUsingMonophony() const noexcept
{
    return useMonophony;
}

bool PlayerControllerImpl::toggleIsSelectedArpUsed() noexcept
{
    useSelectedArp = !useSelectedArp;
    return useSelectedArp;
}

bool PlayerControllerImpl::toggleIsSelectedPitchUsed() noexcept
{
    useSelectedPitch = !useSelectedPitch;
    return useSelectedPitch;
}

bool PlayerControllerImpl::toggleIsUsingMonophony() noexcept
{
    useMonophony = !useMonophony;
    return useMonophony;
}

int PlayerControllerImpl::getMonophonicChannel() const noexcept
{
    return monophonicChannelIndex;
}

std::set<int> PlayerControllerImpl::getPolyphonicChannels() const noexcept
{
    return polyphonicChannelIndexes;
}

void PlayerControllerImpl::setMonophonicChannelToUse(const int channel) noexcept
{
    monophonicChannelIndex = channel;
}

void PlayerControllerImpl::setPolyphonicChannelsToUse(const std::set<int>& channels) noexcept
{
    polyphonicChannelIndexes = channels;
}

bool PlayerControllerImpl::isMonophonicUsesCursorPosition() const noexcept
{
    return monophonicUsesCursorPosition;
}

void PlayerControllerImpl::setMonophonicUsesCursorPosition(const bool useCursorPosition) noexcept
{
    monophonicUsesCursorPosition = useCursorPosition;
}

const EffectContext* PlayerControllerImpl::getEffectContext() noexcept
{
    return effectContext.get();
}


// ===================================================

int PlayerControllerImpl::determineChannelToUse() noexcept
{
    const auto mutedChannelIndexes = mainController.getChannelMuteStates();
    const auto channelCount = songController.getChannelCount();

    auto selectedChannelIndex = 1;  // Used in case there is no possibility (because all channels are muted).

    if (useMonophony) {
        // Builds a list from the target channel to the count, and from 0 to the count. Selects the first that is not muted.
        std::vector<int> channelIndexes;
        channelIndexes.reserve(static_cast<size_t>(channelCount));
        const auto firstChannelIndex = monophonicUsesCursorPosition ? cursorChannelIndex : monophonicChannelIndex;

        for (auto channelIndex = firstChannelIndex; channelIndex < channelCount; ++channelIndex) {
            channelIndexes.push_back(channelIndex);
        }
        for (auto channelIndex = 0; channelIndex < firstChannelIndex; ++channelIndex) {
            channelIndexes.push_back(channelIndex);
        }
        const auto iter = std::find_if(channelIndexes.cbegin(), channelIndexes.cend(), [&](const int channelIndex) {
            return (mutedChannelIndexes.find(channelIndex) == mutedChannelIndexes.cend());
        });

        // Selects the unmuted channels, else if nothing is possible, use the one that was the target (even though it is muted).
        selectedChannelIndex = (iter != channelIndexes.cend()) ? *iter : firstChannelIndex;
    } else {
        // Polyphony.
        // Gets the Xth item of the Set, increases the counter.
        // Copies the Set into a vector. This could be prevented, but this is to remove the channels that are out of bounds, but also the muted ones.
        std::vector<int> channelIndexes;
        channelIndexes.reserve(polyphonicChannelIndexes.size());
        std::copy_if(polyphonicChannelIndexes.cbegin(), polyphonicChannelIndexes.cend(), back_inserter(channelIndexes), [&](const int channelIndex) {
            return (channelIndex < channelCount) && (mutedChannelIndexes.find(channelIndex) == mutedChannelIndexes.cend());
        });

        if (!channelIndexes.empty()) {         // Security. Not supposed to happen, unless all the channels are muted.
            selectedChannelIndex = channelIndexes.at(polyphonicCounter % channelIndexes.size());
            ++polyphonicCounter;
        }
    }

    return selectedChannelIndex;
}

void PlayerControllerImpl::sendObserverEvent(const SongPlayerObserver::Event event) noexcept
{
    getSongPlayerObservers().applyOnObservers([&](SongPlayerObserver* observer) {
        observer->onPlayerPlayEvent(event);
    });
}


// SubsongMetadataObserver method implementations.
// ======================================================

void PlayerControllerImpl::onSubsongMetadataChanged(const Id& subsongId, const unsigned int what)
{
    // Only interested in the loop, for the current Subsong, and if the player is playing.
    if (!NumberUtil::isBitPresent(what, SubsongMetadataObserver::What::loop) || (songController.getCurrentSubsongId() != subsongId) || !songPlayer.isPlaying()) {
        return;
    }

    playSongFromCurrentLocation();
}


// SelectedBlockObserver method implementations.
// =======================================================

void PlayerControllerImpl::onNewSelectedBlock(const juce::Range<int> newRange) noexcept
{
    // Stores it, will be used when playing the pattern. Filled with the Block selection, or the cursor line if there is no block.
    selectedBlock = newRange;
}


// MidiController::MidiListener method implementations.
// =======================================================

void PlayerControllerImpl::onMidiNoteReceived(const int originalNoteNumber)
{
    // Adds a possible offset to the note.
    const auto add = mustAddOctaveToInputMidiNote() ? (currentOctave - middleOctave) * CellConstants::noteCountInOctave : 0;
    const auto noteNumber = NumberUtil::correctNumber(originalNoteNumber + add, CellConstants::minimumNote, CellConstants::maximumNote, true);

    // First, asks the main controller to see if the PV is interested. If yes, don't do anything.
    const auto managed = mainController.tryToRecordExternalNote(noteNumber);

    // The PV doesn't want it. Plays it in the "test area" behavior.
    if (!managed) {
        play(noteNumber, false, { });
    }
}

void PlayerControllerImpl::onMidiProgramChangeReceived(const int programChange)
{
    mainController.setSelectedInstrumentFromIndex(programChange);
}


// CursorObserver method implementations.
// =======================================================

void PlayerControllerImpl::onCursorChannelMoved(const CursorLocation& cursorLocation)
{
    // Only the "normal" TrackType will update the cursor index.
    if (cursorLocation.getTrackType() == TrackType::normal) {
        cursorChannelIndex = cursorLocation.getChannelIndex();
    }
}


// GeneralDataObserver method implementations.
// =======================================================

void PlayerControllerImpl::onGeneralDataChanged(const unsigned int whatChanged)
{
    // Only interested in the Add Octave event.
    if (!NumberUtil::isBitPresent(whatChanged, mustAddOctaveToMidiInputNoteChanged)) {
        return;
    }

    // Invalidates the cache.
    cachedMustAddOctaveToInputMidiNote = { };
}

bool PlayerControllerImpl::mustAddOctaveToInputMidiNote() noexcept
{
    // Is the value known? If not, checks in the preferences. Cached, so that we don't check it every time.
    if (cachedMustAddOctaveToInputMidiNote.isPresent()) {
        return cachedMustAddOctaveToInputMidiNote.getValue();
    }

    const auto addOctave = PreferencesManager::getInstance().mustAddOctaveToInputMidiNotes();
    cachedMustAddOctaveToInputMidiNote = addOctave;
    return addOctave;
}

}   // namespace arkostracker
