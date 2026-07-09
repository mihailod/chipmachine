#pragma once

#include "../../chipplugin.h"

namespace musix {

// STarKos (.sks) — the Amstrad CPC AY-3-8912 / YM2149 tracker by Targhan
// (Arkos), predecessor of Arkos Tracker. Played by a vendored slice of the
// MIT-licensed Arkos Tracker 3 source (repo-root /arkostracker3): the author's
// own StarkosImporter feeds the AT3 SongPlayer + PsgStreamGenerator chain,
// rendered to PCM offline (the same path as AT3's headless "SongToWav" tool).
// The same SongLoader also imports native Arkos Tracker songs (.aks, an XML
// document the editor saves gzip- or zip-compressed), so they share this plugin.
class SksPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "STarKos"; }
    bool canHandle(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    std::set<std::string> getSupportedExtensions() const override { return {"sks", "aks"}; }
};

} // namespace musix
