#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Holds the nodes in the XML of a Song. */
class SongNodes
{
public:
    static const juce::String declareNamespaceAttribute;
    static const juce::String declareNamespaceValueForSong;
    static const juce::String declareXsiAttribute;
    static const juce::String declareXsiValue;
    static const juce::String declareSchemaLocationAttribute;
    static const juce::String declareSchemaLocationValueForSong;

    static const juce::String song;
    static const juce::String formatVersion;
    static const juce::String title;
    static const juce::String argbColor;
    static const juce::String author;
    static const juce::String composer;
    static const juce::String comment;
    static const juce::String creationDateMs;
    static const juce::String modificationDateMs;

    static const juce::String subsongs;
    static const juce::String subsong;

    static const juce::String initialSpeed;
    static const juce::String digiChannel;
    static const juce::String highlightSpacing;
    static const juce::String secondaryHighlight;
    static const juce::String loopStartPosition;
    static const juce::String endPosition;
    static const juce::String replayFrequencyHz;
    static const juce::String psgs;
    static const juce::String psg;
    static const juce::String type;
    static const juce::String frequencyHz;
    static const juce::String referenceFrequencyHz;
    static const juce::String samplePlayerFrequencyHz;
    static const juce::String mixingOutput;

    static const juce::String sidPlayerCapability;
    static const juce::String profile;
    static const juce::String minimumPeriod;
    static const juce::String maximumPeriod;

    static const juce::String tracks;
    static const juce::String track;
    static const juce::String isReadOnly;
    static const juce::String index;

    static const juce::String speedTracks;
    static const juce::String speedTrack;
    static const juce::String eventTracks;
    static const juce::String eventTrack;

    static const juce::String positions;
    static const juce::String position;
    static const juce::String height;
    static const juce::String transpositions;
    static const juce::String transposition;
    static const juce::String pattern;
    static const juce::String patterns;
};

}   // namespace arkostracker
