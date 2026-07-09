#include "FileExtensions.h"

namespace arkostracker 
{

// NOTE: On Linux/mac, extensions are case-sensitive. A bit of a hack, though...
// Plus, on Linux, only one filter is available at one time, which is really not handy. Better to have "*" available to see all the songs.

const juce::String FileExtensions::instrumentExtension(".aki");                                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::instrumentExtensionWithWildcard("*" + instrumentExtension);      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
#ifdef WIN32
const juce::String FileExtensions::instrumentExtensionsFilter("*.aki");
#else
const juce::String FileExtensions::instrumentExtensionsFilter("*.aki,*.AKI");                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
#endif

const juce::String FileExtensions::arpeggioExtensionWithWildcard("*.aksarp");                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::pitchExtensionWithWildcard("*.akspitch");                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::wavExtensionWithWildcard("*.wav");                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::txtExtensionWithWildcard("txt");                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)


#ifdef WIN32
const juce::String FileExtensions::loadSongExtensionsFilter = "*.aks;*.sks;*.mod;*.128;*.wyz;*.vt2;*.chp;*.txt;*.mid";
#else
const juce::String FileExtensions::loadSongExtensionsFilter = "*;*.aks;*.sks;*.mod;*.128;*.wyz;*.vt2;*.chp;*.txt;*.mid;*.AKS;*.SKS;*.MOD;*.WYZ;*.VT2;*.TXT;*.MID"; // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
#endif

const juce::String FileExtensions::saveSongExtension = ".aks";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::saveSongExtensionFilter = "*" + saveSongExtension;   // Only one! // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::ymExtensionWithoutDot = "ym";                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::ymExtensionWithWildcard = "*." + ymExtensionWithoutDot;       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::vgmExtensionWithoutDot = "vgm";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::vgzExtensionWithoutDot = "vgz";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::akgExtensionWithoutDot = "akg";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::akyExtensionWithoutDot = "aky";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::akmExtensionWithoutDot = "akm";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::akxExtensionWithoutDot = "akx";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::fapExtensionWithoutDot = "fap";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::midiExtensionWithoutDot = "midi";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::modExtensionWithoutDot = "mod";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::wavExtensionWithoutDot = "wav";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::xmlExtensionWithoutDot = "xml";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::xmlExtensionFilter = "*." + xmlExtensionWithoutDot;          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::csvExtensionWithWildcard = "*.csv";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String FileExtensions::dskExtensionWithoutDot = "dsk";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::dskExtensionWithWildcard = "*." + dskExtensionWithoutDot;    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String FileExtensions::snaExtensionWithoutDot = "sna";                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

}   // namespace arkostracker
