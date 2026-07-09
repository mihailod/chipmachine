#pragma once

#include "../../chipplugin.h"

namespace musix {

// Euphony (.eup) — FM Towns / PC-98 music, played via the vendored eupmini
// replayer (GPLv2).  A song references companion instrument banks (.fmb FM,
// .pmb PCM) by name embedded in its 2 KB header; those siblings must live next
// to the .eup file.
class EUPPlugin : public ChipPlugin {
public:
    std::string name() const override { return "Euphony"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    // The .fmb/.pmb instrument banks named in the .eup header are fetched as
    // siblings (e.g. from Modland) before playback.
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
