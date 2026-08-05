#pragma once

#include "../../chipplugin.h"

namespace musix {

// Bandai WonderSwan / WonderSwan Color sound rips (.wsr). These are ROM-style
// images holding the game's original sound driver code plus data, ending in a
// 32-byte "WSRF" footer whose JMP is the machine's reset vector. Playback is
// real emulation: ares' NEC V30MZ (ISC) runs the rip's own driver on a
// WonderSwan machine written for this project -- memory map with bank
// registers, blank timers, interrupts, DMA -- feeding the APU shared with
// libvgmplugin. See wswan/README.md.
class WSRPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "WonderSwan"; }
    // The machine and the sound chip were written here from the WSdev Wiki's
    // documentation, replacing the GPL in_wsr replayer -- same billing as the
    // other own implementations (musplugin, zxayplugin).
    std::string displayName() const override
    {
        return "WonderSwan (clean room)";
    }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
