#pragma once

#include "../../chipplugin.h"

#include <string>

namespace musix {

// Compute!'s Sidplayer -- ".mus" (voices 1-3) and its stereo ".str" companion
// (voices 4-6, routed to a second SID). A clean-room sequencer over cSID; see
// mus.c for provenance and README.md for how the undocumented behaviour was
// derived.
//
// REGISTRATION ORDER IS THE VARIANT GATE. This plugin is registered AFTER
// vicepluginbridge, so in the plus build -- where VICE exists and has played
// these formats for years -- VICE claims .mus/.str first and nothing changes.
// In the mas build vicepluginbridge is not linked at all, so this plugin picks
// them up and the ~6.5k Sidplayer songs become playable there instead of being
// dropped from the catalog. That keeps one code path rather than an #ifdef, and
// leaves the plugin constructible (hence testable) in both trees.
class MusPlugin : public ChipPlugin
{
public:
    // Matches the catalog's format string for these tunes, which is what the
    // browse-list override in MusicDatabase resolves against.
    // Shown verbatim on the TAB plugin-filter screen. Deliberately
    // distinct from the FORMAT name ("Sidplayer" / "Stereo Sidplayer"),
    // which the catalog uses and which VICE also serves in the plus build.
    std::string name() const override { return "ChipMachine Clean Room SIDPlayer"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    std::vector<std::string> getSecondaryFiles(const std::string& file) override;
};

} // namespace musix
