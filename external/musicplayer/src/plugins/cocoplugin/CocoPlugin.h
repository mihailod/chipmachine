#pragma once

#include "../../chipplugin.h"

namespace musix {

// Coconizer (.coco) -- a sample-based music format from the Acorn Archimedes
// (the same VIDC-era family as Archimedes Tracker .musx). Played via libxmp's
// coco_loader. We do NOT compile a second libxmp slice here: cocoplugin only
// adds coco_load.c + this glue and links against musxplugin, which already
// provides the shared libxmp objects (control/load/sample/voltable/...). That
// keeps a single libxmp copy in the binary and avoids duplicate symbols.
// canHandle is gated to .coco with a first-byte check so we never overlap
// other Amiga/PC formats.
class CocoPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Coconizer"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
