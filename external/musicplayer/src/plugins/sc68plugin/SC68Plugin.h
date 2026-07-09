#ifndef SC68PLAYER_H
#define SC68PLAYER_H

#include "../../chipplugin.h"

namespace musix {

class SC68Plugin : public ChipPlugin {
public:
    std::string name() const override { return "SC68"; }
    explicit SC68Plugin(const std::string& dataDir) : dataDir(dataDir) {}
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;

    // The generic ".snd" extension is shared with Westwood ADL tunes claimed by
    // AdPlugin. SC68's canHandle does authoritative magic validation (it rejects
    // anything that isn't an Atari sc68/SNDH file), so give it first crack to
    // make sure real Atari .snd files are never mis-claimed by AdPlug's
    // structural heuristic.
    int priority() override { return 1; }

private:
    std::string dataDir;
};

} // namespace musix

#endif // SC68PLAYER_H
