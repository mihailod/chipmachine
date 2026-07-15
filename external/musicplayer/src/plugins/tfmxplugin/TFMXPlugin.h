#ifndef TFMX_PLAYER_H
#define TFMX_PLAYER_H

#include "../../chipplugin.h"

namespace musix {

class TFMXPlugin : public ChipPlugin
{
public:
    TFMXPlugin();
    std::string name() const override { return "TFMXPlugin"; }
    bool canHandle(const std::string& name) override;
    // TFMX is named by PREFIX, not extension: canHandle() tests
    // startsWith(name, "mdat.") and the files are "mdat.<title>" + "smpl.<title>"
    // pairs (modland naming). So no extension list can describe this plugin
    // honestly -- "mdat" is reported as the token it routes on, which is what the
    // cmtest priority_map audit needs to stop counting TFMX rows as having no
    // decoder. NOTE it does NOT make TFMX findable inside a ZIP: that picker
    // matches path_extension (a suffix), and "mdat.<title>"'s suffix is the
    // title. Fixing that needs prefix support in the picker, which is a separate
    // change -- prefix matching against the full extension set is what made it
    // pick "AD.EXE" and "Thumbs.db" as music (see db.lua v114/v115).
    std::set<std::string> getSupportedExtensions() const override
    {
        return { "mdat" };
    }
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // TFMX_PLAYER_H
