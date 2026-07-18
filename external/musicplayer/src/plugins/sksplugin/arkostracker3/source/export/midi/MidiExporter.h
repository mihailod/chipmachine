#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../../utils/Id.h"
#include "../../utils/OptionalValue.h"
#include "../../utils/task/Task.h"

namespace arkostracker
{

class SongController;

class MidiExporter final : public Task<std::unique_ptr<juce::MemoryBlock>>
{
public:
    class InstrumentWithMidiData
    {
    public:
        InstrumentWithMidiData(const int pTrackIndex, const OptionalInt pMidiDrumIndex) :
                trackIndex(pTrackIndex),
                midiDrumIndex(pMidiDrumIndex)
        {
        }

        int getTrackIndex() const
        {
            return trackIndex;
        }

        OptionalInt getMidiDrumIndex() const
        {
            return midiDrumIndex;
        }

    private:
        int trackIndex;                     // A virtual track index, to gather the instrument together. Ignored if drums.
        OptionalInt midiDrumIndex;          // If present, to be mapped into this drum midi instrument.
    };

    class ExportData
    {
    public:
        ExportData(const std::map<int, InstrumentWithMidiData>& pInstrumentIndexToMidiData, const int pDrumChannel, const bool pExportMarkers) :
                instrumentIndexToMidiData(pInstrumentIndexToMidiData),
                drumChannel(pDrumChannel),
                exportMarkers(pExportMarkers)
        {
        }

        const std::map<int, InstrumentWithMidiData>& getInstrumentIndexToMidiData() const noexcept
        {
            return instrumentIndexToMidiData;
        }

        int getDrumChannel() const noexcept
        {
            return drumChannel;
        }

        bool mustExportMarkers() const noexcept
        {
            return exportMarkers;
        }

    private:
        std::map<int, InstrumentWithMidiData> instrumentIndexToMidiData;
        int drumChannel;
        bool exportMarkers;
    };

    /**
     * Constructor.
     * @param songController the song controller, with the right Song.
     * @param subsongId the ID of the Subsong to export.
     * @param exportData how to export the instruments.
     */
    MidiExporter(SongController& songController, const Id& subsongId, const ExportData& exportData) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<juce::MemoryBlock>> performTask() noexcept override;

private:
    // See https://www.lim.di.unimi.it/IEEE/MIDI/META.HTM#06
    static constexpr auto textEventTypeTrackName = 3;
    static constexpr auto textEventTypeMarker = 6;

    /**
     * Encodes a BPM MIDI event.
     * @param conductorTrack the track where to encode.
     * @param timestamp the timestamp of the event.
     * @param speed the AT speed (>0).
     * @param rowCountPerBeat the "highlight spacing".
     * @param replayFrequencyHz the replay frequency in Hz.
     */
    static void encodeCurrentBpm(juce::MidiMessageSequence& conductorTrack, double timestamp, int speed, int rowCountPerBeat, float replayFrequencyHz) noexcept;

    /**
     * Adds a track to the midi file, if the track is not empty. It is sorted as a security.
     * @param midiFile the midi file.
     * @param track the track to add.
     * @param trackName the name of the track.
     */
    static void addTrack(juce::MidiFile& midiFile, juce::MidiMessageSequence& track, const juce::String& trackName) noexcept;

    /**
     * Tries to manage the drums, if necessary.
     * @param drumTrack the drum track.
     * @param instrumentIndexToDrumIndex the mapping from instrument index to drum index.
     * @param instrumentIndex the instrument index.
     * @param drumChannel the drum channel (10, 16...).
     * @param timestamp the timestamp.
     * @param drumLengthTimestamp how long is the drum, in timestamp units.
     * @return true if managed.
     */
    static bool manageInstrumentIfDrum(juce::MidiMessageSequence& drumTrack, const std::map<int, int>& instrumentIndexToDrumIndex, int instrumentIndex,
                                       int drumChannel, double timestamp, double drumLengthTimestamp) noexcept;

    /**
     * @return the name of the track, according to the instruments linked to it. Should not be called for Conductor/drums tracks.
     * @param trackIndex the track index.
     */
    juce::String determineTrackName(int trackIndex) const noexcept;

    /** Used to identify an instrument in a map. */
    class InstrumentMarker
    {
    public:
        InstrumentMarker(const int pInstrumentIndex, const int pChannelIndex) :
                instrumentIndex(pInstrumentIndex),
                channelIndex(pChannelIndex)
        {
        }

        bool operator<(const InstrumentMarker& rhs) const
        {
            if (instrumentIndex < rhs.instrumentIndex) {
                return true;
            }
            if (rhs.instrumentIndex < instrumentIndex) {
                return false;
            }
            return channelIndex < rhs.channelIndex;
        }

        int instrumentIndex;
        int channelIndex;
    };

    SongController& songController;
    Id subsongId;
    ExportData exportData;
};

}   // namespace arkostracker
