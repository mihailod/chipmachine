#pragma once

#include "../../chipplugin.h"

namespace musix {

// Clean-room DefleMask .dmf player: SEGA Genesis and SEGA Master System.
//
// Exists so the Mac App Store variant can play DefleMask modules at all: the
// only other engine that reads .dmf is Furnace, which is GPL-2.0-or-later
// structurally and so is gated out of the mas build (CM_HAVE_DMF). Nothing
// here derives from Furnace -- the format comes from DefleMask's own published
// DMF_SPECS documents and the effect behaviour from DefleMask's own manual,
// both archived under spec/. See README.md.
//
// Ships in BOTH variants, but claims files only in mas: in the plus build
// dmfplugin (Furnace) is registered FIRST and outranks us on the same
// extension, so plus keeps playing every .dmf through Furnace and stays
// byte-identical. That is deliberate -- it keeps a working A/B reference in the
// user's hands. See the registration-order note in plugin_register.cpp and the
// same pattern in musplugin (Compute! Sidplayer) and csidplugin.
class DMFCRPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "DMFCR"; }
    std::string displayName() const override { return "DefleMask (Clean Room)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    // Same priority as dmfplugin so that, when both are linked, plain
    // registration order decides -- not a priority number that would also have
    // to be kept in step with OpenMPT's claim on the extension. OpenMPT sits at
    // 0 and still gets X-Tracker "DDMF" files through its own content gate.
    int priority() override { return 1; }
};

} // namespace musix
