#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays JayTrax music (.jxs). JayTrax (originally "Mugician"/CrossX) is a
// software synthesizer + tracker by Reinier "Rhino" van Vliet -- six stereo
// channels with per-instrument synth waveforms (AM/FM/pan/arpeggio modulators),
// sampled instruments, filters and a stereo echo. Songs carry one or more
// subsongs. We render in-process with the public C port of Rhino's own replayer
// (vendored at repo-root jaytrax/), which rebuilds the song from memory and
// mixes int16 stereo at an arbitrary frequency. Detection is by the 16-bit
// "mugiversion" tag at offset 0 (3456 or 3457); the format has no string magic.
// NOT routed through UADE (JayTrax is cross-platform, not an Amiga eagleplayer).
class JxsPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "JayTrax"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
