#pragma once

#include "../../chipplugin.h"

namespace musix {

class AOPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Audio Overload"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin. Leads with
    // the engine name so the row is findable by the name the plugin is actually
    // called, then names both format families it owns: of the 658 songs it
    // holds, ~650 are .psf/.minipsf/.psf2/.minipsf2 (inherited from the deleted
    // heplugin) and ~8 are Capcom QSound. Keeping "PSF" in the label also keeps
    // it in the type-to-narrow haystack, which the old separate
    // "PlayStation 1/2 (PSF)" row used to provide.
    //
    // Nothing keys off this string -- formatOverride / formatPlayer /
    // nameToPlugin all use name() ("Audio Overload"), which is unchanged.
    virtual std::string displayName() const override { return "Audio Overload (PS 1/2 PSF / QSound)"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string &name) override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
