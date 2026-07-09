#pragma once

#include "../../chipplugin.h"

namespace musix {

// MSX music via the vendored libkss replayer: MGSDRV (.mgs), MuSICA/KINROU5
// (.bgm), OPLLDriver (.opx), MPK (.mpk) and MoonBlaster 1.4 (.mbm, MBR143
// driver -- PSG/YM2413/Y8950, with optional .mbk ADPCM sample banks).
class KSSPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "MSX (libkss)"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string &name) override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
