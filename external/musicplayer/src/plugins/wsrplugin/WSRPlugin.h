#pragma once

#include "../../chipplugin.h"

namespace musix {

// Bandai WonderSwan / WonderSwan Color sound rips (.wsr). These are ROM-style
// images holding the game's original sound driver code plus data, ending in a
// 32-byte "WSRF" footer. Playback is real emulation: a NEC V30MZ CPU core drives
// the WonderSwan sound chip (4 channels incl. PCM/voice, sweep and noise). The
// emulator core is the vendored, self-contained in_wsr replayer (GPL-2.0+).
class WSRPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "WonderSwan (in_wsr)"; }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
