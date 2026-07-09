#pragma once

#include "../../chipplugin.h"

namespace musix {

// SBStudio (.pac) -- a sample-based tracker for MS-DOS by Henning Hellstroem
// (early 1990s; modland SBStudio/). The on-disk format is an IFF-like chunk
// container whose first four bytes are the magic "PACG". Playback uses a
// vendored copy of Thomas Pfaff's libpac (ISC-licensed ANSI C), which decodes a
// module straight to interleaved 16-bit PCM. canHandle is gated to .pac with
// the PACG magic so we never grab unrelated payloads on that extension.
class SBStudioPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "SBStudio"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
