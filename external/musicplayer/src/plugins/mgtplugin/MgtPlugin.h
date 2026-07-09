#pragma once

#include "../../chipplugin.h"

namespace musix {

// Megatracker (.mgt) -- an Atari ST sample-based tracker by Cream (modland
// Megatracker/, e.g. Cedyn / Raiden / Seabrush). Played via libxmp's
// mgt_loader. Like cocoplugin we do NOT compile a second libxmp slice here:
// mgtplugin adds only mgt_load.c + this glue and links against musxplugin,
// which already provides the shared libxmp objects (control/load/sample/...).
// That keeps a single libxmp copy in the binary. canHandle is gated to .mgt
// with the "MGT"/"MCS" magic so we never overlap other formats.
class MgtPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Megatracker"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
