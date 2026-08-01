#pragma once

#include "../../chipplugin.h"

namespace musix {

// DefleMask .dmf player. DefleMask modules share the ".dmf" extension with the
// unrelated X-Tracker DMF ("DDMF") format that OpenMPT handles; DefleMask files
// are zlib-compressed (first byte 0x78) and decompress to the magic
// ".DelekDefleMask.". This plugin content-gates on that so the two formats
// coexist. Playback is driven by a vendored slice of the Furnace engine
// (tildearrow/furnace, GPLv2), which natively loads DMF across every DefleMask
// system (Genesis, SMS, Game Boy, PC Engine, NES, C64, Arcade, Neo Geo).
class DMFPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "DMF"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "DefleMask"; }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
    // Beat OpenMPT (priority 0) to the .dmf extension so DefleMask files reach
    // us; OpenMPT still claims X-Tracker DDMF via its own canHandle gate.
    virtual int priority() override { return 1; }
};

} // namespace musix
