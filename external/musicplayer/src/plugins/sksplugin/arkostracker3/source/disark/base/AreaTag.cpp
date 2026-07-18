#include "AreaTag.h"

namespace arkostracker 
{

const juce::String AreaTag::tagCodeRegion = "DisarkCodeRegion";                                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagByteRegion = "DisarkByteRegion";                                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagWordRegion = "DisarkWordRegion";                                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagPointerRegion = "DisarkPointerRegion";                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceReferenceArea = "DisarkForceReferenceArea";                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceNonReferenceArea = "DisarkForceNonReferenceArea";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagStart = "Start";                                                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagEnd = "End";                                                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String AreaTag::tagCodeRegionStart = tagCodeRegion + tagStart;                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagCodeRegionEnd = tagCodeRegion + tagEnd;                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagByteRegionStart = tagByteRegion + tagStart;                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagByteRegionEnd = tagByteRegion + tagEnd;                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagWordRegionStart = tagWordRegion + tagStart;                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagWordRegionEnd = tagWordRegion + tagEnd;                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagPointerRegionStart = tagPointerRegion + tagStart;                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagPointerRegionEnd = tagPointerRegion + tagEnd;                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String AreaTag::tagForceReferenceAreaStart = tagForceReferenceArea + tagStart;        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceReferenceAreaEnd = tagForceReferenceArea + tagEnd;            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceNonReferenceAreaStart = tagForceNonReferenceArea + tagStart;  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceNonReferenceAreaEnd = tagForceNonReferenceArea + tagEnd;      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::baseTagForceReferenceDuring = "DisarkForceReferenceDuring";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::baseTagForceNonReferenceDuring = "DisarkForceNonReferenceDuring";     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String AreaTag::tagForceOneWordAreaWithForceReference = "DisarkWordForceReference";   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String AreaTag::tagForceOneWordAreaWithForceNonReference = "DisarkWordForceNonReference";  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String AreaTag::tagGenerateExternalLabel = "DisarkGenerateExternalLabel";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

}   // namespace arkostracker

