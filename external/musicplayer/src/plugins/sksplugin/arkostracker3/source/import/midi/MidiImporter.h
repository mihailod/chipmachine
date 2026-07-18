#pragma once

#include "../SongImporter.h"
#include "../../business/song/tool/builder/SongBuilder.h"
#include "MidiTrackReader.h"

namespace arkostracker 
{

class MidiTrackReader;

/** Imports a Midi file. */
class MidiImporter final : public SongImporter
{
public:
    /** Constructor. */
    MidiImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<SongImporter::Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ConfigurationType requireConfiguration() const noexcept override;
    ImportFirstPassReturnData getReturnData() const noexcept override;
    ImportedFormat getFormat() noexcept override;

    /**
     * Finds the channel count.
     * This method is useful to know how many tracks the Midi file has, before fully parsing it.
     * PUBLIC for test units. Not great, please bear with me on this one.
     * @param midiFile the midi file.
     * @return the channel count, or 0 if it couldn't be found, and the track indexes that (0-based) which are not empty.
     */
    static std::pair<int, std::set<int>> findTrackCount(const juce::MidiFile& midiFile) noexcept;

private:
    /**
     * Attempts to load a Midi file.
     * @param inputStream the Stream of the file to load.
     * @return the midi file if successful.
     */
    static std::unique_ptr<juce::MidiFile> loadMidiFile(juce::InputStream& inputStream) noexcept;

    /**
     * Parses the given MidiFile, fills the SongBuilder.
     * @param inputStream the input Stream, to check the MIDI 0 format.
     * @param midiFile the MidiFile.
     * @param midiConfiguration the MIDI configuration to use.
     */
    void parse(juce::InputStream& inputStream, const juce::MidiFile& midiFile, const MidiConfiguration& midiConfiguration) noexcept;

    /**
     * @returns false if the midi track has no notes. It may have time signature, etc.
     * @param midiFile the midi file.
     * @param midiTrackIndex the midi track index.
     */
    static bool hasMidiTrackNotes(const juce::MidiFile& midiFile, int midiTrackIndex) noexcept;

    /**
     * @return true if format >0.
     * @param inputStream the Input Stream. Will be rewind.
     */
    static bool checkIfFormatOk(juce::InputStream& inputStream) noexcept;

    /**
     * Mixes the song Tracks (extracted from the Midi file, one per Track) according to what the user wants.
     * @param cellIndexToCellsForEachMidiTrack the song tracks. They may be modified.
     * @param channelMixerKeeper indicates what channel to mix and keep.
     */
    static void mixTracks(std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack, const ChannelMixerKeeper& channelMixerKeeper) noexcept;

    /**
     * Keeps the song Tracks (extracted from the Midi file, one per Track) according to what the user wants.
     * @param cellIndexToCellsForEachMidiTrack the song tracks. They may be modified.
     * @param channelMixerKeeper indicates what channel to mix and keep.
     */
    static void keepTracks(std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack, const ChannelMixerKeeper& channelMixerKeeper) noexcept;

    /**
     * Generates the Song from the Midi Tracks, into the Song Builder. They have been mixed and only the "good" ones are kept.
     * @param midiTrackReader the object that reads the Midi Tracks.
     * @param cellIndexToCellsForEachMidiTrack the song tracks.
     * @param importVelocities true to import velocities.
     * @param originalPpq the ppq in the original song.
     * @param ppq the desired ppq.
     */
    void generateSongFromMidiCells(const MidiTrackReader& midiTrackReader, const std::vector<std::map<int, Cell>>& cellIndexToCellsForEachMidiTrack,
                                   bool importVelocities, int originalPpq, int ppq) const noexcept;

    /**
     * Removes the possible volume effect, if the volume is the same as the given volume for this channel.
     * @param currentVolumeForChannel the current volume for this channel.
     * @param cell the Cell to modify.
     * @return the volume from the Cell, if it was different. This should become the new channel effect.
     */
    static int removeVolumeEffectIfTheSame(int currentVolumeForChannel, Cell& cell) noexcept;

    /**
     * Checks whether a Speed Cell must be written here. The BPM must be converted to speed.
     * We consider the Speed Track is already opened!
     * @param subsongBuilder the SubsongBuilder to use.
     * @param midiTrackReader the object that reads the Midi Tracks.
     * @param midiCell the index of Cell in the midi file.
     * @param speedTrackIndex the index of the Speed Track.
     * @param cellIndexInPosition the cell index in the current position where to write the Speed.
     * @param currentSpeed the current speed.
     * @param originalSongPpq the ppq in the original song.
     * @param ppq the desired ppq.
     * @return the new/unchanged speed.
     */
    static int manageSpeedCell(SubsongBuilder& subsongBuilder, const MidiTrackReader& midiTrackReader, int midiCell, int speedTrackIndex,
                               int cellIndexInPosition, int currentSpeed,int originalSongPpq, int ppq) noexcept;

    /**
     * Finds the original PPQ.
     * @param midiFile the midi file.
     * @return the original PPQ, or 0 if it couldn't be found.
     */
    static int findOriginalSongPpq(const juce::MidiFile& midiFile) noexcept;

    /**
     * Builds the Instruments into the Song.
     * @param midiTrackReader the object that reads the Midi Tracks.
     */
    void buildInstruments(const MidiTrackReader& midiTrackReader) const noexcept;

    /**
     * Builds an Instrument into the Song.
     * @param instrumentIndex the index of the Instrument.
     * @param instrumentType the type of the Instrument to build.
     * @param name the name of the Instrument.
     */
    void buildInstrument(int instrumentIndex, MidiTrackReader::MidiInstrumentType instrumentType, const juce::String& name) const noexcept;

    static void encodePatternAndPosition(SubsongBuilder& subsongBuilder, int position, int patternBaseTrackIndex, int channelCount, int patternHeight) noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
    ImportFirstPassReturnData importFirstPassReturnData;
};

}   // namespace arkostracker
