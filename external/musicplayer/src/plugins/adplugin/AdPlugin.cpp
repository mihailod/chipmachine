
#include "AdPlugin.h"

#include "../../chipplayer.h"
#include "adplug/adplug.h"
#include "adplug/emuopl.h"
#include "libbinio/binfile.h"
#include "libbinio/binio.h"

#include <math.h>

#include "opl/dbemuopl.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>

#ifdef min
#    undef min
#endif

namespace musix {

class AdPlugPlayer : public ChipPlayer
{

    Copl* emu;
    CPlayer* m_player = nullptr;
    /* STATIC! */ CAdPlugDatabase* g_database = nullptr;
    // Per-instance sample/tick accumulator. (Was a function-static long, which
    // leaked timing state across every song and broke restart/subsong switches.)
    long minicnt = 0;

public:
    AdPlugPlayer(const std::string& fileName, const std::string& configDir)
    {

        if (g_database == nullptr) {
            binistream* fp = new binifstream(configDir + "/" + "adplug.db");
            fp->setFlag(binio::BigEndian, false);
            fp->setFlag(binio::FloatIEEE);

            g_database = new CAdPlugDatabase();
            g_database->load(*fp);
            delete fp;
            CAdPlug::set_database(g_database);
        }

        emu = new DBemuopl(44100, true);

        m_player = CAdPlug::factory(fileName, emu, CAdPlug::players);

        if (m_player == nullptr) { throw player_exception(); }

        setMeta("title", m_player->gettitle(), "composer",
                m_player->getauthor(), "length", m_player->songlength() / 1000,
                "songs", m_player->getsubsongs(), "format",
                m_player->gettype());
    }

    ~AdPlugPlayer() override
    {
        delete m_player;
        delete emu;
        emu = nullptr;
        m_player = nullptr;
    }

    int getSamples(int16_t* target, int noSamples) override
    {

        int freq = 44100; // 49716;

        long i = 0;
        long towrite = noSamples / 2;
        auto* pos = target;

        while (towrite > 0) {
            while (minicnt < 0) {
                minicnt += freq;
                auto playing = m_player->update();
                if (!playing) { return -1; }
            }
            i = std::min(towrite,
                         (long)(minicnt / m_player->getrefresh() + 4) & ~3);
            emu->update(pos, i);
            pos += i * 2;
            towrite -= i;
            minicnt -= (long)(m_player->getrefresh() * i);
        }

        return noSamples;
    }

    // Subsong navigation. Many AdPlug formats -- and Westwood ADL/SND banks in
    // particular (Eye of the Beholder, Kyrandia, ...) -- pack dozens of tracks
    // into one file, reported via the "songs" meta. Without this override the
    // base seekTo() returns false, so next/prev in the player never advances.
    // Time-based seeking isn't supported by these replayers, so `seconds` is
    // ignored and we only switch subsong.
    bool seekTo(int song, int /*seconds*/) override
    {
        if (song < 0 || song >= static_cast<int>(m_player->getsubsongs())) {
            return false;
        }
        m_player->rewind(song);
        minicnt = 0;
        return true;
    }
};

// Mirrors the full player table registered by the vendored AdPlug (adplug.cpp),
// minus extensions another chipmachine plugin owns: ".s3m" (OpenMPT, exposed
// here as the renamed ".as3m"), ".sng" (UADE SoundMon), ".ims" (OpenMPT/UADE),
// ".mus" (UADE/Vice C64), and ".vgm"/".vgz" (GME console VGM -- AdPlug only
// decodes the rare OPL-chip VGMs).
static const std::set<std::string> supported_ext{
    "a2m",  "a2t",   "adl", "adlib", "agd", "amd", "as3m", "bam", "bmf",
    "cff",  "cmf",   "d00", "dfm",   "dmo", "dro", "dtm",  "got", "ha2",
    "hsc",  "hsp",   "hsq", "imf",   "jbm", "ksm", "laa",  "lds", "m",
    "mad",  "mdi",   "mdy", "mid",   "mkf", "mkj", "msc",  "mtk", "mtr",
    "pis",  "plx",   "rac", "rad",   "raw", "rix", "rol",  "sa2", "sat",
    "sci",  "sdb",   "snd", "sng",   "sop", "sqx", "wlf",  "xad", "xms",
    "xsm"};

// Westwood .snd files (Eye of the Beholder, Legend of Kyrandia, ...) are
// headerless ADL tunes that reuse a generic extension also claimed by other
// formats (e.g. Atari sc68). Reproduce the ADL loader's version detection so we
// only claim files that actually parse as ADL and leave the rest alone.
//
// NB: many real-world per-track .snd rips contain only the sequence data and
// reference an external instrument bank not present in the file -- those parse
// here but decode to silence. The self-contained .adl version of the same tune
// is the playable path; we don't attempt to merge an external bank.
static bool isWestwoodAdl(const std::string& name)
{
    if (!utils::File::exists(name)) { return false; }
    try {
        utils::File f{name};
        if (f.getSize() < 720) { return false; } // minimum size of an ADL v1 file

        std::array<uint8_t, 270> hdr{};
        if (f.read(hdr.data(), static_cast<int>(hdr.size())) <
            static_cast<int>(hdr.size())) {
            return false;
        }

        auto le16 = [&](size_t off) {
            return static_cast<uint16_t>(hdr[off] | (hdr[off + 1] << 8));
        };

        // The first 120 bytes are 16-bit values in v1/v2 but not in v3.
        int version = 3;
        for (int i = 0; i < 120; i += 2) {
            uint16_t w = le16(i);
            if (w >= 500 && w < 0xffff) {
                version = 1; // actually v1 or v2
                break;
            }
        }

        if (version != 3) {
            // The track-offset table at byte 120 separates v1 from v2 and lets
            // us reject files whose offsets are too small to be ADL at all.
            version = 2;
            for (int i = 0; i < 150; i += 2) {
                uint16_t w = le16(120 + i);
                if (w > 0 && w < 600) { return false; }
                if (w > 0 && w < 1000) { version = 1; }
            }
            if (version == 2 && f.getSize() < 1120) { return false; }
        } else if (f.getSize() < 2500) {
            return false; // minimum size of an ADL v3 file
        }
        return true;
    } catch (utils::io_exception&) {
        return false;
    }
}

// AdPlug's MAD loader (mad.cpp) is for Jurgen Wothke's "Mad Tracker 2", which
// begins with the literal magic "MAD+". The ".mad" extension is also used by
// Macintosh PlayerPRO modules ("MADG"/"MADF"/"MADK"/...), which this loader
// would reject at decode time -- surfacing as a hard "failed to load". Gate on
// the magic so only genuine Mad Tracker 2 files are claimed.
static bool isMadTracker(const std::string& name)
{
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) { return false; }
    char magic[4] = {0};
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && memcmp(magic, "MAD+", 4) == 0;
}

// ".sng" is heavily overloaded (SCC-Musixx MSX, Richard Joseph "RJP1SMOD" Amiga,
// SoundMon, ...). Only claim the three AdLib variants AdPlug actually decodes:
//   "FMC!"  -> Faust Music Creator (fmc.cpp)
//   "ObsM"  -> SNG Player          (sng.cpp)
//   AdLib Tracker (adtrack.cpp) has no magic but is always exactly 36000 bytes
//   and pairs with a 468-byte ".ins" instrument file (see getSecondaryFiles).
// Everything else is left to its owning plugin (SCC-Musixx, UADE, ...).
// Single-file AdLib .sng variants carry a magic: "FMC!" (Faust Music Creator)
// or "ObsM" (SNG Player). AdLib Tracker has none (detected by its 36000B size).
static bool isAdlibSngWithMagic(const std::string& name)
{
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) { return false; }
    char magic[4] = {0};
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) &&
           (memcmp(magic, "FMC!", 4) == 0 || memcmp(magic, "ObsM", 4) == 0);
}

// ".dtm" is shared: AdPlug's loader is "DeFy DTM" (Riven the Mage's AdLib OPL
// tracker, magic "DeFy DTM " + version 0x10, see dtm.cpp), but the same
// extension is far more commonly Digital Tracker / Digital Home Studio modules
// (Atari Falcon, magic "D.T." -- modland "Digital Tracker DTM", 100+ tunes)
// which OpenMPT decodes. Without this gate AdPlug claims every .dtm, fails to
// parse the Digital Tracker ones, and they never reach OpenMPT. Only claim
// genuine DeFy DTM files; the rest fall through to openmptplugin.
static bool isDefyDtm(const std::string& name)
{
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) { return false; }
    char magic[9] = {0};
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && memcmp(magic, "DeFy DTM ", 9) == 0;
}

static bool isAdlibSng(const std::string& name)
{
    if (isAdlibSngWithMagic(name)) { return true; }
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) { return false; }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size == 36000; // AdLib Tracker (adtrack.cpp)
}

bool AdPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    for (auto& c : ext) {
        c = static_cast<char>(tolower(c));
    }
    if (ext == "snd") { return isWestwoodAdl(name); }
    // ".mad" is shared: AdPlug's loader is Mad Tracker 2 (magic "MAD+"), but the
    // same extension is used by Macintosh PlayerPRO modules ("MADG"/"MADF"/...),
    // which are handled by playerproplugin. Decline anything that isn't a real
    // Mad Tracker 2 file so PlayerPRO tunes route there instead of failing here.
    if (ext == "mad") { return isMadTracker(name); }
    // ".sng" overlaps several non-AdLib formats; only claim AdLib variants.
    if (ext == "sng") { return isAdlibSng(name); }
    // ".dtm" is shared with Digital Tracker (Atari Falcon); only claim DeFy DTM.
    if (ext == "dtm") { return isDefyDtm(name); }
    return supported_ext.count(ext) > 0;
}

std::set<std::string> AdPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* AdPlugin::fromFile(const std::string& fileName)
{
    return new AdPlugPlayer{fileName, configDir};
};

std::vector<std::string> AdPlugin::getSecondaryFiles(const std::string& file)
{
    auto ext = utils::path_extension(file);
    for (auto& c : ext) {
        c = static_cast<char>(tolower(c));
    }
    // Sierra SCI AdLib music (.sci) is a multi-file format: the OPL2 instrument
    // bank lives in a companion "<prefix>patch.003" file in the same directory,
    // where <prefix> is the first 3 characters of the song's file name. This
    // mirrors mid.cpp's load_sierra_ins(), and matches modland's layout
    // (e.g. "kq1 flutey.sci" + "kq1patch.003" under .../Kings Quest 1/).
    if (ext == "sci") {
        auto name = utils::path_filename(file);
        if (name.size() >= 3) {
            return {name.substr(0, 3) + "patch.003"};
        }
    }
    // Ken Silverman's AdLib music (.ksm) is multi-file too: ksm.cpp loads a
    // shared instrument bank with the fixed name "insts.dat" from the same
    // directory (modland co-hosts it, e.g. under "Ad Lib/Ken's AdLib Music/").
    if (ext == "ksm") {
        return {"insts.dat"};
    }
    // AdLib Tracker (.sng, adtrack.cpp) pairs the 36000-byte song with a
    // 468-byte "<base>.ins" instrument file (modland co-hosts it). The other
    // AdLib .sng variants we claim (FMC!/ObsM) are single-file, so only request
    // the companion when this isn't one of those.
    if (ext == "sng" && !isAdlibSngWithMagic(file)) {
        auto name = utils::path_filename(file);
        auto dot = name.find_last_of('.');
        if (dot != std::string::npos) { return {name.substr(0, dot) + ".ins"}; }
    }
    return {};
}

} // namespace musix

extern "C" void adplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::AdPlugin>(config);
    });
}
