#pragma once

#include "../../chipplugin.h"

#include <cstddef>
#include <cstdint>

namespace musix {

// Plays the modland "Sam Coupe COP" corpus: SAM Coupé music for the Philips
// SAA1099, where a Z80 replay routine is either compiled into the song or is the
// shared E-Tracker player. We run that original Z80 routine on the GME Z80 core
// and route its SAA1099 port writes into Dave Hooper's SAASound. See
// cop_machine.cpp / CopPlugin.cpp.
//
// The ".cop" extension is shared with the zxart E-Tracker variant that ZXTune
// decodes (its loader uses a 5-byte header). canHandle() therefore content-
// detects the modland layouts (10-byte-header E-Tracker data files and the
// "compiled" raw-Z80 songs) and ZXTunePlugin declines those same files.
class CopPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "Sam Coupe (COP)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

// Shared content test: true when `d` (first `len` bytes of a .cop file) is one
// of the modland Sam Coupe COP layouts. Used by both CopPlugin (to claim) and
// ZXTunePlugin (to decline), so a single rule governs the routing.
bool looksLikeSamCoupeCop(const uint8_t* d, size_t len);

} // namespace musix
