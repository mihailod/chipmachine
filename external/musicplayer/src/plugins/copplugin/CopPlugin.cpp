#include "CopPlugin.h"
#include "cop_machine.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace musix {

bool looksLikeSamCoupeCop(const uint8_t* d, size_t len)
{
    if (len < 16) {
        return false;
    }
    // E-Tracker data file: the modland layout has a 10-byte header (5 little-
    // endian pointers) and the signature at offset 0x0A. (ZXTune's COP loader
    // expects a 5-byte header, so it does not match these.)
    if (len >= 0x1A && std::memcmp(d + 0x0A, "ETracker", 8) == 0) {
        return true;
    }
    // "Compiled" E-Tracker song: a raw Z80 image whose entry is LD HL,nn / JP nn.
    if (d[0] == 0x21 && d[3] == 0xC3) {
        return true;
    }
    // The two other raw replayer revisions SCPlayer recognises (subsong patches).
    if (len >= 0x19 &&
        std::memcmp(d + 0x13, "\x01\xff\x01\x3e\x1c\xed", 6) == 0) {
        return true;
    }
    if (std::memcmp(d, "\x43\x72\x3d\xc2\x23\x81", 6) == 0) {
        return true;
    }
    // Two further raw-Z80 replayer revisions seen in the modland "Sam Coupe SNG"
    // corpus (both play on the same machine; the load/mirror handles them):
    //   - "DEC A / JP NZ,#8123" (3d c2 23 81) at offset 2 -- the entry preamble
    //     of the Ziutek "ofc" songs (a near-sibling of the 43 72 3d c2 23 81
    //     revision above, differing only in the two lead bytes).
    //   - "JP #81xx" (c3 .. 81) at offset 0 -- a compiled song jumping straight
    //     into the 0x8100 replayer page (e.g. tetris, JP #81ec).
    // Both are specific enough to reject the other .sng chips (Richard Joseph,
    // GoatTracker, ZoundMonitor, Synder start with different bytes).
    if (len >= 6 && d[2] == 0x3d && d[3] == 0xc2 && d[4] == 0x23 && d[5] == 0x81) {
        return true;
    }
    if (len >= 3 && d[0] == 0xc3 && d[2] == 0x81) {
        return true;
    }
    return false;
}

class CopPlayer : public ChipPlayer
{
public:
    CopPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
        : machine(44100)
    {
        // Don't re-gate on looksLikeSamCoupeCop() here: canHandle() already
        // routed us (claiming ".cop" outright, or content-matching ".sng"), and
        // CopMachine::init() is the authoritative check -- it runs the song's
        // own compiled replayer and returns false for anything that isn't a
        // loadable COP image. Gating on the (non-exhaustive) signature list
        // would reject valid compiled songs whose entry preamble it doesn't
        // enumerate (e.g. rozkaz/stovka's "00 3e 01.." subsong stub).
        if (!machine.init(data.data(), data.size())) {
            throw player_exception("Sam Coupe COP: replayer failed to start");
        }
        setMeta("title", utils::path_basename(fileName), "format", "Sam Coupe COP",
                "channels", 6, "length", 0);
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        int frames = noSamples / 2; // interleaved stereo pairs
        int got = machine.generate(target, frames);
        if (got <= 0) {
            return 0; // song ended (one full loop)
        }
        return got * 2;
    }

private:
    cop::CopMachine machine;
};

// The modland "Sam Coupe SNG" corpus uses ".sng" for the same SAM Coupé
// SAA1099 music (E-Tracker data files and raw-Z80 "compiled" songs) that the
// "Sam Coupe COP" corpus stores as ".cop" -- identical replayer family, so the
// same machine plays both. ".sng" is heavily overloaded, but looksLikeSamCoupeCop
// is a strict content gate (E-Tracker signature / specific replayer entry bytes)
// that rejects the other .sng chips (Richard Joseph, GoatTracker, ZoundMonitor,
// Synder, SCC-Musixx), so claiming the extension here is safe.
static const std::set<std::string> supported_ext{"cop", "sng"};

bool CopPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(utils::toLower(name));
    if (supported_ext.count(ext) == 0) {
        return false;
    }
    // ".cop" is (effectively) exclusive to this SAM Coupé SAA1099 family across
    // both corpora we ingest -- the modland "Sam Coupe COP" dir and the zxart
    // E-Tracker rips -- and CopMachine runs the song's own compiled Z80 replayer
    // straight from the load image, so it plays *any* compiled variant, not just
    // the handful of entry-point signatures looksLikeSamCoupeCop() enumerates.
    // The broader modland corpus (191 files) contains further compiled-song
    // preambles (e.g. the "00 3e 01.." subsong stub in rozkaz/stovka, or the
    // "01 ff 00 3e.." one in chrismas) that match none of those signatures.
    // Claiming the extension outright closes that detection gap -- and is safe,
    // as no other plugin can decode these (ZXTune's COP loader fails on them,
    // which is why copplugin exists). A genuinely non-COP ".cop" simply fails to
    // init and Skips. ".sng" stays strictly content-gated below, since that
    // extension is heavily overloaded with unrelated chips.
    if (ext == "cop") {
        return true;
    }
    utils::File f{name};
    if (!f.exists()) {
        return false;
    }
    auto data = f.readAll();
    return looksLikeSamCoupeCop(data.data(), data.size());
}

std::set<std::string> CopPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* CopPlugin::fromFile(const std::string& fileName)
{
    return new CopPlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void copplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::CopPlugin>();
    });
}
