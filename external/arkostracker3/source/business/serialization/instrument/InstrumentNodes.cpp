#include "InstrumentNodes.h"

namespace arkostracker 
{

const juce::String InstrumentNodes::nodeInstrumentsRoot = "instruments";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeInstrumentRoot = "instrument";              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeName = "name";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeType = "type";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::valueTypePsg = "psg";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::valueTypeSample = "sample";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSpeed = "speed";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeLoopStartIndex = "loopStartIndex";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeEndIndex = "endIndex";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeIsLooping = "isLooping";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeColorArgb = "colorArgb";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeIsRetrig = "isRetrig";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSfxIsExported = "isSfxExported";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeAutoSpread = "autoSpread";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodePsgSection = "psgSection";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeCells = "cells";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeCell = "cell";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeVolume = "volume";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeNoise = "noise";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodePrimaryPeriod = "primaryPeriod";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodePrimaryArpeggioNoteInOctave = "primaryArpeggioNoteInOctave";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodePrimaryArpeggioOctave = "primaryArpeggioOctave";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodePrimaryPitch = "primaryPitch";              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeLink = "link";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeRatio = "ratio";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeHardwareEnvelope = "hardwareEnvelope";      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSecondaryPeriod = "secondaryPeriod";        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSecondaryArpeggioNoteInOctave = "secondaryArpeggioNoteInOctave";       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSecondaryArpeggioOctave = "secondaryArpeggioOctave";                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSecondaryPitch = "secondaryPitch";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidIsActivated = "sidIsActivated";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidCycleBalancePercent = "sidCycleBalancePercent"; // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidLowShelfVolume = "sidLowShelfVolume";    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidRatio = "sidRatio";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidArpeggio = "sidArpeggio";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSidPitch = "sidPitch";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentNodes::nodeFrequencyHz = "frequencyHz";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeAmplificationRatio = "amplificationRatio";  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeOriginalFilename = "originalFilename";      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeSampleUnsigned8BitsBase64 = "sampleUnsigned8BitsBase64";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentNodes::nodeDigiNote = "digiNote";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

}   // namespace arkostracker
