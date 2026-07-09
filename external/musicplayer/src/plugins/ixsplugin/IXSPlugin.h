#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays Ixalance (.ixs) music. Ixalance is an Impulse-Tracker-family format
// from ~2000 by the (defunct) Shortcut Software Development BV: its trick is
// that it does not store PCM samples but synthesizes and zlib-compresses its
// own wavetable data, so whole songs are only a few KB. All tunes were made by
// Maarten van Strien (Modland "Ixalance/Crystal Score").
//
// The original sources were lost; this plays via "webixs", Juergen Wothke's
// native C++ reimplementation reverse-engineered (Ghidra) from Shortcut's Win32
// player, vendored at repo-root webixs/. We drive its pull-style render API and
// emit stereo S16 at 44100 Hz. Routing is by the "IXS!" magic in canHandle().
//
// NOTE: webixs is CC BY-NC-SA 4.0 (NonCommercial) -- the only such component in
// the project; see webixs/LICENSE and the credits files.
class IXSPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "IXS"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
