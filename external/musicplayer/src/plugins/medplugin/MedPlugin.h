#pragma once

#include "../../chipplugin.h"

namespace musix {

// Old MED (.med) -- the pre-OctaMED "Music Editor" format by Teijo Kinnunen,
// magic "MED\x02" / "MED\x03" / "MED\x04" (MED 2.00 .. 3.20). This is a
// different, older format than the MMD0..MMD3 OctaMED containers (which
// OpenMPT/UADE already handle); UADE's MED eagleplayer crashes on these
// ("score crashed") and libopenmpt only decodes MMD0..MMD3, so they were
// unplayable. libxmp's med2/med3/med4 loaders decode them, so this plugin
// drives those directly. Like cocoplugin/mgtplugin we do NOT compile a second
// libxmp slice: medplugin adds only med2/3/4_load.c + this glue and links
// against musxplugin, which already provides the shared libxmp objects
// (control/load/sample/med_extras/mmd_common/...). That keeps a single libxmp
// copy in the binary. canHandle is gated to the "MED\x02..\x04" magic so we
// never overlap the MMD OctaMED formats.
class MedPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "MED"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
