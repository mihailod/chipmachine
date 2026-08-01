#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays OPL-family VGM/VGZ logs (YM3812/OPL2, YMF262/OPL3, ...) through
// ValleyBell's libvgm, which GME's Vgm_Emu cannot decode. canHandle content-
// gates to OPL-carrying files only, so the ~14k non-OPL console VGZ in the
// library stay on GME.
class LibVGMPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "libvgm"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "OPL2/OPL3 VGM (libvgm)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
