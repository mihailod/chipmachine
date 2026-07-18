#include "MidiExporter.h"

#include "../../business/song/tool/context/EffectContextImpl.h"
#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../business/song/tool/speed/BpmCalculator.h"
#include "../../controllers/SongController.h"
#include "../../player/SongPlayer.h"
#include "../../song/subsong/SubsongConstants.h"

namespace arkostracker
{

MidiExporter::MidiExporter(SongController& pSongController, const Id& pSubsongId, const ExportData& pExportData) noexcept :
        songController(pSongController),
        subsongId(pSubsongId),
        exportData(pExportData)
{
}

std::pair<bool, std::unique_ptr<juce::MemoryBlock>> MidiExporter::performTask() noexcept
{
    const auto& song = songController.getSong();

    SongPlayer songPlayer(song);
    const auto startLocation = Location(subsongId, 0);
    const auto [_, newPastEndLocation] = song->getLoopStartAndPastEndPositions(subsongId);
    const auto psgCount = song->getPsgCount(subsongId);

    const auto[counter, upTo, loopToCounter] = FrameCounter::count(*song, subsongId);
    const auto iterationCount = counter;
    jassert(iterationCount > 0);

    songPlayer.play(startLocation, startLocation, newPastEndLocation, true, true);

    // Which instruments are drums, or to be on a specific track?
    std::map<int, int> instrumentIndexToDrumIndex;
    std::map<int, int> instrumentIndexToTrackIndex;
    auto lastTrackIndex = 0;
    for (const auto& [instrumentIndex, midiData] : exportData.getInstrumentIndexToMidiData()) {
        const auto midiDrumOptional = midiData.getMidiDrumIndex();
        const auto trackIndex = midiData.getTrackIndex();
        if (midiDrumOptional.isPresent()) {
            instrumentIndexToDrumIndex.insert({ instrumentIndex, midiDrumOptional.getValue()});
        } else {
            instrumentIndexToTrackIndex.insert({ instrumentIndex, trackIndex });
        }

        lastTrackIndex = std::max(lastTrackIndex, trackIndex);
    }
    const auto trackCount = lastTrackIndex + 1;     // Simplification, there will be potential holes, who cares, empty tracks are discarded.
    const auto drumChannel = exportData.getDrumChannel();

    // Gets initial speed to declare the initial BPM.
    auto speed = SubsongConstants::defaultSpeed;
    auto rowCountPerBeat = SubsongConstants::defaultPrimaryHighlight;
    auto replayFrequencyHz = PsgFrequency::defaultReplayFrequencyHz;
    song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto metadata = subsong.getMetadata();
        speed = metadata.getInitialSpeed();
        rowCountPerBeat = metadata.getHighlightSpacing();
        replayFrequencyHz = metadata.getReplayFrequencyHz();
    });

    constexpr auto ppq = 960;
    constexpr auto timeSignatureDenominator = 4;
    constexpr auto noteLength = ppq * 2;                        // Arbitrary, one bar.
    const auto timestampAdvance = ppq / rowCountPerBeat;        // Arbitrary, seems to work.
    const auto drumLengthInTimestamp = ppq / rowCountPerBeat;
    auto timestamp = 0.0;
    auto currentPosition = -1;  // Sentinel value, to force writing. Warning, not updated if the markers are not exported.

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ppq);

    juce::MidiMessageSequence drumsTrack;
    juce::MidiMessageSequence conductorTrack;
    conductorTrack.addEvent(juce::MidiMessage::timeSignatureMetaEvent(rowCountPerBeat, timeSignatureDenominator));
    // Writes the initial speed MIDI event.
    encodeCurrentBpm(conductorTrack, timestamp, speed, rowCountPerBeat, replayFrequencyHz);

    // Creates the blank tracks.
    std::vector<juce::MidiMessageSequence> midiMessageSequences;
    for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        midiMessageSequences.emplace_back();
    }
    // Creates the note-offs collection.
    std::map<InstrumentMarker, juce::MidiMessage> instrumentMarkerToNoteOffs;

    // Browses the patterns.
    for (auto iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
        auto firstTick = false;

        for (auto browsedPsgIndex = 0; browsedPsgIndex < psgCount; ++browsedPsgIndex) {
            // Necessary to get all the registers of all the PSGs.
            songPlayer.getNextRegisters(browsedPsgIndex);
        }

        // If wanted, writes the position marker, whenever we enter a new position.
        if (exportData.mustExportMarkers()) {
            const auto newPosition = songPlayer.getCurrentLocationInSongOffline().getPosition();
            if (newPosition > currentPosition) {    // Also takes care of the looping at the end.
                currentPosition = newPosition;

                // By default, the position number.
                auto markerName = "Position " + juce::String(currentPosition) + " (&" + juce::String::toHexString(currentPosition) + ")";
                // Is there a position marker?
                song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
                    const auto readMarkerName = subsong.getPosition(newPosition).getMarkerName();
                    if (readMarkerName.isNotEmpty()) {
                        markerName = readMarkerName;
                    }
                });

                auto markerEvent = juce::MidiMessage::textMetaEvent(textEventTypeMarker, markerName);
                markerEvent.setTimeStamp(timestamp);

                conductorTrack.addEvent(markerEvent);
            }
        }

        const auto result = songPlayer.getResultsOffline();

        firstTick = (!result.empty() && (result.at(0)->getTick() == 0));

        // Browses each channel.
        auto channelIndex = -1;
        for (const auto& channelResult : result) {
            ++channelIndex;

            // New speed?
            if (const auto newSpeedOptional = channelResult->getSpeed(); newSpeedOptional.isPresent()) {
                if (const auto newSpeed = newSpeedOptional.getValue(); speed != newSpeed) {
                    speed = newSpeed;
                    encodeCurrentBpm(conductorTrack, timestamp, speed, rowCountPerBeat, replayFrequencyHz);
                }
            }

            const auto& cells = channelResult->getCells();
            jassert(cells.size() <= 1);
            const auto isNewNotePlayed = (cells.size() == 1);   // If 0, tick > 0.

            auto noteOptional = channelResult->getNewPlayedNote();
            // Can stop here if not on the first tick, no note and no new note played.
            if (!firstTick || noteOptional.isAbsent() || !isNewNotePlayed) {
                continue;
            }

            // What is the instrument? Mandatory to know on which channel we are.
            const auto instrumentIdOptional = channelResult->getInstrumentId();
            OptionalInt instrumentIndexOptional;
            if (instrumentIdOptional.isPresent()) {
                instrumentIndexOptional = song->getInstrumentIndex(instrumentIdOptional.getValue());
            }
            if (instrumentIndexOptional.isAbsent()) {
                continue;
            }
            const auto instrumentIndex = instrumentIndexOptional.getValue();

            // Special case for drums.
            const auto isDrum = manageInstrumentIfDrum(drumsTrack, instrumentIndexToDrumIndex, instrumentIndex,
                drumChannel, timestamp, drumLengthInTimestamp);
            if (isDrum) {
                continue;
            }

            // If RST, stops instrument on THIS channel.
            if (instrumentIndex == 0) {
                for (auto& [marker, noteOff] : instrumentMarkerToNoteOffs) {
                    if (marker.channelIndex == channelIndex) {
                        noteOff.setTimeStamp(timestamp);
                        break;
                    }
                }
                continue;
            }

            // Gets the channel from the instrument index, thanks to the mapping.
            const auto it = instrumentIndexToTrackIndex.find(instrumentIndex);
            if (it == instrumentIndexToTrackIndex.cend()) {
                continue;
            }
            const auto trackIndex = it->second;

            auto& midiMessageSequence = midiMessageSequences.at(static_cast<size_t>(trackIndex));

            const InstrumentMarker marker(instrumentIndex, channelIndex);

            // If there was a note-off of this instrument, it must be written first.
            if (auto mIt = instrumentMarkerToNoteOffs.find(marker); mIt != instrumentMarkerToNoteOffs.cend()) {
                auto noteOff = mIt->second;
                // If after the note-on to be written, sets the timestamp to its beginning.
                if (noteOff.getTimeStamp() > timestamp) {
                    noteOff.setTimeStamp(timestamp);
                }
                midiMessageSequence.addEvent(noteOff);

                instrumentMarkerToNoteOffs.erase(mIt);  // Consumes the note-off.
            }

            // Writing the note-on.
            const auto noteNumber = noteOptional.getValue();
            constexpr auto velocity = 1.0F;
            constexpr auto midiChannel = 1;
            auto noteOn = juce::MidiMessage::noteOn(midiChannel, noteNumber, velocity);
            noteOn.setTimeStamp(timestamp);
            midiMessageSequence.addEvent(noteOn);

            // Stores the note-off to be able to stop it. It is set a bit later from "now", but it may be set before when written above.
            auto noteOff = juce::MidiMessage::noteOff(noteOn.getChannel(), noteOn.getNoteNumber(), noteOn.getVelocity());
            noteOff.setTimeStamp(timestamp + noteLength);
            instrumentMarkerToNoteOffs.insert({ marker, noteOff });     // Has been erased above, so insert will work.
        }


        if (firstTick) {
            timestamp += timestampAdvance;
        }
    }

    // Writes the final note-offs.
    for (const auto& [marker, noteOff] : instrumentMarkerToNoteOffs) {
        const auto instrumentIndex = marker.instrumentIndex;
        // Gets the channel from the instrument index, thanks to the mapping.
        if (const auto it = instrumentIndexToTrackIndex.find(instrumentIndex); it != instrumentIndexToTrackIndex.cend()) {
            const auto trackIndex = it->second;

            auto& midiMessageSequence = midiMessageSequences.at(static_cast<size_t>(trackIndex));
            midiMessageSequence.addEvent(noteOff);
        }
    }

    // Adds the non-empty tracks to the file.
    addTrack(midiFile, conductorTrack, "Conductor");
    auto trackIndex = 0;
    for (auto& midiMessageSequence : midiMessageSequences) {
        const auto trackName = determineTrackName(trackIndex);
        addTrack(midiFile, midiMessageSequence, trackName);

        ++trackIndex;
    }
    addTrack(midiFile, drumsTrack, "Drums");

    juce::MemoryOutputStream outputStream;
    midiFile.writeTo(outputStream);
    auto outputMemoryBlock = std::make_unique<juce::MemoryBlock>(outputStream.getMemoryBlock());

    return { true, std::move(outputMemoryBlock) };
}

void MidiExporter::encodeCurrentBpm(juce::MidiMessageSequence& conductorTrack, const double timestamp, const int speed, const int rowCountPerBeat,
    const float replayFrequencyHz) noexcept
{
    const auto bpm = BpmCalculator::calculateBpm(rowCountPerBeat, speed, replayFrequencyHz);
    const auto microsecondsPerQuarterNote = static_cast<int>(60.0 / bpm * 1000000.0);    // 200000 = 300 bpm, 300000 = 200 bpm, 400000 = 150 bpm.

    auto tempoEvent = juce::MidiMessage::tempoMetaEvent(microsecondsPerQuarterNote);
    tempoEvent.setTimeStamp(timestamp);

    conductorTrack.addEvent(tempoEvent);
}

void MidiExporter::addTrack(juce::MidiFile& midiFile, juce::MidiMessageSequence& track, const juce::String& trackName) noexcept
{
    if (track.getNumEvents() == 0) {
        return;
    }

    auto trackNameMessage = juce::MidiMessage::textMetaEvent(textEventTypeTrackName, trackName);
    trackNameMessage.setTimeStamp(0);
    track.addEvent(trackNameMessage);

    track.sort();     // Security.
    midiFile.addTrack(track);
}

bool MidiExporter::manageInstrumentIfDrum(juce::MidiMessageSequence& drumTrack, const std::map<int, int>& instrumentIndexToDrumIndex,
    const int instrumentIndex, const int drumChannel, const double timestamp, const double drumLengthTimestamp) noexcept
{
    // Is the instrument a drum?
    const auto it = instrumentIndexToDrumIndex.find(instrumentIndex);
    if (it == instrumentIndexToDrumIndex.cend()) {
        return false;
    }

    // A drum. Writes it.
    const auto noteNumber = it->second;
    constexpr auto velocity = 1.0F;
    auto noteOn = juce::MidiMessage::noteOn(drumChannel, noteNumber, velocity);
    noteOn.setTimeStamp(timestamp);

    auto noteOff = juce::MidiMessage::noteOff(drumChannel, noteNumber, velocity);
    noteOff.setTimeStamp(timestamp + drumLengthTimestamp);

    drumTrack.addEvent(noteOn);
    drumTrack.addEvent(noteOff);

    return true;
}

juce::String MidiExporter::determineTrackName(const int trackIndex) const noexcept
{
    // Concatenates all the instruments name of the track, if present.
    juce::String trackName;
    for (const auto& [instrumentIndex, midiData] : exportData.getInstrumentIndexToMidiData()) {
        if (trackIndex == midiData.getTrackIndex()) {
            const auto instrumentName = songController.getInstrumentNameFromIndex(instrumentIndex);
            if (instrumentName.isNotEmpty()) {
                if (trackName.isNotEmpty()) {
                    trackName += " + ";
                }
                trackName += instrumentName;
            }
        }
    }

    // As a fallback, use the track index.
    if (trackName.isEmpty()) {
        trackName = "Track " + juce::String(trackIndex);
    }

    return trackName;
}

}   // namespace arkostracker
