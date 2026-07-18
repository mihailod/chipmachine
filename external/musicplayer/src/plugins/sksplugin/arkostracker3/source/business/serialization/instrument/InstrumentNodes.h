#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Holds the names of the nodes of the Instruments. */
class InstrumentNodes
{
public:
    static const juce::String nodeInstrumentsRoot;

    static const juce::String nodeInstrumentRoot;
    static const juce::String nodeType;
    static const juce::String valueTypePsg;
    static const juce::String valueTypeSample;
    static const juce::String nodeName;
    static const juce::String nodeSpeed;
    static const juce::String nodeLoopStartIndex;
    static const juce::String nodeEndIndex;
    static const juce::String nodeIsLooping;
    static const juce::String nodeColorArgb;
    static const juce::String nodeIsRetrig;

    static const juce::String nodeSfxIsExported;

    static const juce::String nodeAutoSpread;
    static const juce::String nodePsgSection;

    static const juce::String nodeCells;
    static const juce::String nodeCell;

    static const juce::String nodeVolume;
    static const juce::String nodeNoise;
    static const juce::String nodePrimaryPeriod;
    static const juce::String nodePrimaryArpeggioNoteInOctave;
    static const juce::String nodePrimaryArpeggioOctave;
    static const juce::String nodePrimaryPitch;
    static const juce::String nodeLink;
    static const juce::String nodeRatio;
    static const juce::String nodeHardwareEnvelope;
    static const juce::String nodeSecondaryPeriod;
    static const juce::String nodeSecondaryArpeggioNoteInOctave;
    static const juce::String nodeSecondaryArpeggioOctave;
    static const juce::String nodeSecondaryPitch;
    static const juce::String nodeSidIsActivated;
    static const juce::String nodeSidCycleBalancePercent;
    static const juce::String nodeSidLowShelfVolume;
    static const juce::String nodeSidRatio;
    static const juce::String nodeSidArpeggio;
    static const juce::String nodeSidPitch;

    static const juce::String nodeFrequencyHz;
    static const juce::String nodeAmplificationRatio;
    static const juce::String nodeOriginalFilename;
    static const juce::String nodeSampleUnsigned8BitsBase64;
    static const juce::String nodeDigiNote;
};

}   // namespace arkostracker
