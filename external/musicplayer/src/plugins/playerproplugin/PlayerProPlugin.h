#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays PlayerPRO music. PlayerPRO (Antoine Rosset, 1990s) was the dominant
// Macintosh tracker; its modules drive a software synth (the "MADDriver"). On
// Modland the whole PlayerPro/ tree is Benjamin Birney's "mantra" game score,
// stored in the older "MADG"/"MADF" module format. We render with the vendored
// public-domain MADDriver (repo-root playerpro/) in its offline NoHardwareDriver
// mode, pulling 16-bit stereo PCM at 44100 Hz via MADDirectSave().
//
// Routing is by the 4-char magic in canHandle() ("MADG"/"MADF", plus the native
// "MADK"): the ".mad" extension collides with AdPlug's unrelated Mad Tracker 2
// loader, which is content-gated to decline these so they reach us instead.
class PlayerProPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "PlayerPRO"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
