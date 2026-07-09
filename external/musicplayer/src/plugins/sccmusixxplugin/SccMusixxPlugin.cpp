#include "SccMusixxPlugin.h"
#include "../../chipplayer.h"

#include "scc_machine.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

// Dispatch for ".sng" must NOT rely on the extension's letter case: that
// extension is shared by several unrelated formats. We identify SCC-Musixx by
// content, regardless of case.
//
// SCC-Musixx .SNG files are raw MSX memory images with no magic header of their
// own, so we (a) reject the other ".sng" formats by their magics, then (b)
// confirm the SCC-Musixx structure: the image is a whole number of 256-byte
// records, the first SCC waveform's 8-char ASCII name sits at 0x20, the
// sequence length at 0x780 is sane, and the order list at 0x781 holds small
// pattern indices. Validated against the full SCC-MUSIXX disk (46/46) with zero
// false positives on AdLib (SNGPlay/Faust/AdLib Tracker), GoatTracker and
// Amiga Richard Joseph .sng files.
bool looksLikeSccMusixx(const uint8_t* d, size_t len)
{
    if (len < 0x800 || (len % 256) != 0) {
        return false;
    }
    // Reject the other formats that also use ".sng", by their magic bytes.
    if (memcmp(d, "GTS", 3) == 0) { return false; }  // GoatTracker (GTS!/GTS2/GTS3..5)
    if (memcmp(d, "ObsM", 4) == 0) { return false; } // AdLib SNGPlay
    if (memcmp(d, "FMC!", 4) == 0) { return false; } // AdLib Faust Music Creator
    if (memcmp(d, "RJP1", 4) == 0) { return false; } // Amiga Richard Joseph (RJP1SMOD)
    // First SCC waveform's 8-char name.
    for (int i = 0; i < 8; i++) {
        if (d[0x20 + i] < 0x20 || d[0x20 + i] > 0x7E) {
            return false;
        }
    }
    unsigned seqlen = d[0x780]; // sequence length
    if (seqlen < 1 || seqlen > 200 || 0x781 + seqlen > len) {
        return false;
    }
    for (unsigned i = 0; i < seqlen; i++) { // order list: pattern indices
        if (d[0x781 + i] >= 64) {
            return false;
        }
    }
    return true;
}

} // namespace

class SccMusixxPlayer : public ChipPlayer
{
public:
    SccMusixxPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
        : machine(44100)
    {
        if (!looksLikeSccMusixx(data.data(), data.size())) {
            throw player_exception("Not an SCC-Musixx .SNG file");
        }
        if (!machine.init(data.data(), data.size())) {
            throw player_exception("SCC-Musixx: replayer failed to start");
        }
        setMeta("title", utils::path_basename(fileName), "format", "SCC-Musixx",
                "channels", 5, "length", 0);
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // The machine renders mono; fan out to interleaved stereo.
        int frames = noSamples / 2;
        int got = machine.generate(target, frames);
        if (got <= 0) {
            return 0; // song ended (one full sequence loop)
        }
        for (int i = got - 1; i >= 0; i--) {
            int16_t s = target[i];
            target[i * 2] = s;
            target[i * 2 + 1] = s;
        }
        return got * 2;
    }

private:
    sccmusixx::SccMachine machine;
};

static const std::set<std::string> supported_ext{"sng"};

bool SccMusixxPlugin::canHandle(const std::string& name)
{
    // The extension gate is case-insensitive (.sng / .SNG); the actual decision
    // is made purely on file content by looksLikeSccMusixx().
    if (supported_ext.count(utils::path_extension(utils::toLower(name))) == 0) {
        return false;
    }
    // SCC-Musixx files are small (a few KB to ~32 KB); read the whole image so
    // the structural checks (sequence length / order list) have their bytes.
    utils::File f{name};
    if (!f.exists()) {
        return false;
    }
    auto data = f.readAll();
    return looksLikeSccMusixx(data.data(), data.size());
}

std::set<std::string> SccMusixxPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* SccMusixxPlugin::fromFile(const std::string& fileName)
{
    return new SccMusixxPlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void sccmusixxplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::SccMusixxPlugin>();
    });
}
