#include "SongNodes.h"

namespace arkostracker 
{

const juce::String SongNodes::declareNamespaceAttribute = "xmlns:aks";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::declareNamespaceValueForSong = "https://www.julien-nevo.com/arkostracker/ArkosTrackerSong";    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::declareXsiAttribute = "xmlns:xsi";                                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::declareXsiValue = "http://www.w3.org/2001/XMLSchema-instance";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::declareSchemaLocationAttribute = "xsi:schemaLocation";                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::declareSchemaLocationValueForSong = "https://www.julien-nevo.com/arkostracker/schema/at3";   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::song = "song";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::formatVersion = "formatVersion";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::title = "title";                                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::argbColor = "argbColor";                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::author = "author";                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::composer = "composer";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::comment = "comment";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::creationDateMs = "creationDateMs";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::modificationDateMs = "modificationDateMs";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::subsongs = "subsongs";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::subsong = "subsong";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::initialSpeed = "initialSpeed";                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::digiChannel = "digiChannel";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::highlightSpacing = "highlightSpacing";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::secondaryHighlight = "secondaryHighlight";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::loopStartPosition = "loopStartPosition";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::endPosition = "endPosition";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::replayFrequencyHz = "replayFrequencyHz";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::psgs = "psgs";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::psg = "psg";                                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::type = "type";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::frequencyHz = "frequencyHz";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::referenceFrequencyHz = "referenceFrequencyHz";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::samplePlayerFrequencyHz = "samplePlayerFrequencyHz";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::mixingOutput = "mixingOutput";                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::sidPlayerCapability = "sidPlayerCapability";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::profile = "profile";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::minimumPeriod = "minimumPeriod";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::maximumPeriod = "maximumPeriod";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::tracks = "tracks";                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::track = "track";                                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::isReadOnly = "isReadOnly";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::index = "index";                                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::speedTracks = "speedTracks";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::speedTrack = "speedTrack";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::eventTracks = "eventTracks";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::eventTrack = "eventTrack";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String SongNodes::positions = "positions";                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::position = "position";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::height = "height";                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::transpositions = "transpositions";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::transposition = "transposition";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::pattern = "pattern";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongNodes::patterns = "patterns";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

}   // namespace arkostracker
