
#include "OpenMPTPlugin.h"

#include "openmpt/libopenmpt/libopenmpt.h"
#include "openmpt/libopenmpt/libopenmpt_stream_callbacks_file.h"

#include "../../chipplayer.h"

#include <coreutils/split.h>
#include <coreutils/utils.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <vector>

#include <coreutils/file.h>

namespace musix {

// ---------------------------------------------------------------------------
// Composer 670 (CDFM) ".670" -> C67 in-memory conversion.
//
// libopenmpt's Load_c67.cpp decodes the *unpacked* editor format (a fixed
// 1954-byte named header + a 128-entry pattern offset/length table). The
// modland "Composer 670 (CDFM)" corpus is the demo/stripped ".670" layout: a
// compact 10-byte header, no instrument names, and only a pattern *offset*
// table. Crucially the pattern byte-stream and the 16-byte PCM / 11-byte OPL
// instrument structs are byte-identical to C67, so this is a header/table
// repack, not a re-decode. We rebuild the C67 image and hand it to libopenmpt,
// reusing its full CDFM decoder. Returns nullopt if the data is not a
// well-formed AdLib/SoundBlaster 670.
static uint32_t cdfmU32LE(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static void cdfmWriteU32LE(std::vector<uint8_t>& v, size_t off, uint32_t x)
{
    v[off] = x & 0xFF;
    v[off + 1] = (x >> 8) & 0xFF;
    v[off + 2] = (x >> 16) & 0xFF;
    v[off + 3] = (x >> 24) & 0xFF;
}

static std::optional<std::vector<uint8_t>>
convert670ToC67(const std::vector<uint8_t>& d)
{
    if (d.size() < 10) { return std::nullopt; }
    const uint8_t speed = d[0];
    const uint8_t orderListLen = d[1];
    const uint8_t numPatterns = d[2];
    const uint8_t numDigInst = d[3];
    const uint8_t numOPLInst = d[4];
    const uint8_t loopDest = d[5];
    const uint32_t sampleOffset = cdfmU32LE(d.data() + 6);

    // Header constraints mirroring Load_c67's ValidateHeader.
    if (speed < 1 || speed > 15) { return std::nullopt; }
    if (numPatterns < 1 || numPatterns > 128) { return std::nullopt; }
    if (numDigInst > 32 || numOPLInst > 32) { return std::nullopt; }
    if (orderListLen < 1) { return std::nullopt; }

    const size_t orderOff = 10;
    const size_t patTableOff = orderOff + orderListLen;
    const size_t pcmOff = patTableOff + 4u * numPatterns;
    const size_t oplOff = pcmOff + 16u * numDigInst;
    const size_t patDataStart = oplOff + 11u * numOPLInst;
    if (patDataStart > d.size()) { return std::nullopt; }
    if (sampleOffset < patDataStart || sampleOffset > d.size()) {
        return std::nullopt;
    }
    const uint32_t regionLen = sampleOffset - static_cast<uint32_t>(patDataStart);

    std::array<uint32_t, 128> off670{};
    for (unsigned i = 0; i < numPatterns; i++) {
        uint32_t o = cdfmU32LE(d.data() + patTableOff + 4u * i);
        if (o > regionLen) { return std::nullopt; }
        off670[i] = o;
    }
    for (unsigned i = 0; i < orderListLen; i++) {
        if (d[orderOff + i] >= numPatterns) { return std::nullopt; }
    }

    // ---- Build the fixed 1954-byte C67 header (names left zeroed). ----
    std::vector<uint8_t> out(1954 + 1024, 0);
    out[0] = speed;
    out[1] = loopDest; // restart position
    const size_t hdrSamples = 2 + 32 * 13; // after sampleNames[32][13]
    for (unsigned i = 0; i < numDigInst; i++) {
        std::memcpy(&out[hdrSamples + 16u * i], &d[pcmOff + 16u * i], 16);
    }
    const size_t hdrFmNames = hdrSamples + 32 * 16;
    const size_t hdrFmInstr = hdrFmNames + 32 * 13;
    for (unsigned i = 0; i < numOPLInst; i++) {
        std::memcpy(&out[hdrFmInstr + 11u * i], &d[oplOff + 11u * i], 11);
    }
    const size_t hdrOrders = hdrFmInstr + 32 * 11; // == 1698
    for (unsigned i = 0; i < 256; i++) {
        out[hdrOrders + i] = (i < orderListLen) ? d[orderOff + i] : 0xFF;
    }
    // hdrOrders + 256 == 1954

    // Each 670 pattern's byte length is the distance to the next-higher offset
    // in the shared pattern-data blob (the last one runs to the region end).
    auto pat670Len = [&](unsigned i) -> uint32_t {
        uint32_t start = off670[i], end = regionLen;
        for (unsigned j = 0; j < numPatterns; j++) {
            uint32_t o = off670[j];
            if (o > start && o < end) { end = o; }
        }
        return end - start;
    };

    // ---- Emit 128 patterns in index order so pattern 127 is physically last;
    // Load_c67 reads sample data from the position right after that chunk. ----
    std::vector<uint8_t> region;
    region.reserve(regionLen + 3u * 128u);
    std::array<uint32_t, 128> newOff{}, newLen{};
    static const uint8_t kDummy[3] = {0x40, 0x00, 0x60};
    for (unsigned i = 0; i < 128; i++) {
        newOff[i] = static_cast<uint32_t>(region.size());
        if (i < numPatterns) {
            const uint8_t* src = d.data() + patDataStart + off670[i];
            region.insert(region.end(), src, src + pat670Len(i));
            // C67 requires every pattern chunk >= 3 bytes. Pad short/empty ones
            // with end markers; the parser stops at the first 0x60 so trailing
            // bytes never affect playback.
            while (region.size() - newOff[i] < 3) { region.push_back(0x60); }
        } else {
            region.insert(region.end(), kDummy, kDummy + 3);
        }
        newLen[i] = static_cast<uint32_t>(region.size()) - newOff[i];
        if (newLen[i] > 0x1000) { return std::nullopt; } // malformed pattern
    }
    for (unsigned i = 0; i < 128; i++) {
        cdfmWriteU32LE(out, 1954 + 4u * i, newOff[i]);
        cdfmWriteU32LE(out, 1954 + 512 + 4u * i, newLen[i]);
    }

    out.insert(out.end(), region.begin(), region.end());
    // Append exactly the PCM sample bytes the loader consumes (sum of the
    // digital-instrument lengths), clamped to what the file holds -- this drops
    // trailing junk in the rip (e.g. FM-only tunes with numDigInst == 0).
    uint32_t sumLen = 0;
    for (unsigned i = 0; i < numDigInst; i++) {
        sumLen += cdfmU32LE(d.data() + pcmOff + 16u * i + 4);
    }
    size_t avail = d.size() - sampleOffset;
    size_t toCopy = (sumLen < avail) ? sumLen : avail;
    out.insert(out.end(), d.begin() + sampleOffset,
               d.begin() + sampleOffset + toCopy);
    return out;
}

// Read a whole file into a byte vector without throwing (returns empty on
// failure); used by canHandle's content gates on virtual/non-existent paths.
static std::vector<uint8_t> readFileBytes(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}

// Startrekker AM (and AudioSculpture EXO) modules store their synthesized
// instruments in an external .nt/.as companion. This libopenmpt build has no
// external-instrument loading, so those voices would render silent -- UADE
// owns them instead. But purely *sampled* FLT4/EXO4 modules play correctly
// here, so only decline the genuine AM ones. An AM module references several
// samples that have zero length (the synth voices); mirror libopenmpt's own
// ">1 empty referenced sample" heuristic from Load_mod.cpp.
static bool isStartrekkerAM(std::vector<uint8_t> const& data)
{
    const uint8_t* p = data.data();
    if (data.size() < 1084) { return false; }
    if (memcmp(p + 1080, "FLT", 3) != 0 &&
        memcmp(p + 1080, "EXO", 3) != 0) {
        return false;
    }
    // Sample lengths (big-endian words) from the 31-entry sample table.
    uint32_t sampleLen[32] = {0};
    for (int i = 0; i < 31; i++) {
        const uint8_t* s = p + 20 + i * 30;
        sampleLen[i + 1] = (static_cast<uint32_t>(s[22]) << 8 | s[23]) * 2;
    }
    // Highest pattern index used by the order list -> pattern count.
    int numPatterns = 0;
    for (int i = 0; i < 128; i++) {
        if (p[952 + i] >= numPatterns) { numPatterns = p[952 + i] + 1; }
    }
    // Mark every sample referenced by pattern data (4 bytes/cell, 64 rows,
    // 4 channels). FLT8 stores 8 channels as paired 4-channel patterns, so
    // reading 4-wide still sweeps every cell.
    bool referenced[32] = {false};
    size_t base = 1084;
    for (int pat = 0; pat < numPatterns; pat++) {
        for (int cell = 0; cell < 64 * 4; cell++) {
            size_t off = base + pat * 1024 + cell * 4;
            if (off + 4 > data.size()) { break; }
            int smp = (p[off] & 0xF0) | (p[off + 2] >> 4);
            if (smp >= 1 && smp <= 31) { referenced[smp] = true; }
        }
    }
    int emptyReferenced = 0;
    for (int i = 1; i <= 31; i++) {
        if (referenced[i] && sampleLen[i] == 0) { emptyReferenced++; }
    }
    return emptyReferenced > 1;
}

class OpenMPTPlayer : public ChipPlayer
{
public:
    explicit OpenMPTPlayer(std::vector<uint8_t> const& data)
    {
        if (data.size() < 1090) { throw player_exception("Data too short"); }
        if (isStartrekkerAM(data)) {
            throw player_exception("Can not play Startrekker AM module");
        }

        module = openmpt_module_create_from_memory(data.data(), data.size(),
                                                   nullptr, nullptr, nullptr);

        if (module == nullptr) {
            throw player_exception("Could not load module");
        }

        openmpt_module_set_repeat_count(module, 99);

        auto length =
            static_cast<uint32_t>(openmpt_module_get_duration_seconds(module));
        auto songs = openmpt_module_get_num_subsongs(module);

        auto get = [&](const char* what) {
            return std::string(openmpt_module_get_metadata(module, what));
        };

        auto type_long = get("type_long");
        auto type = get("type");

        auto p = utils::split(type_long, " / ");
        if (p.size() > 1) { type_long = p[0]; }

        setMeta("title", get("title"), "composer", get("artist"), "message",
                get("message"), "tracker", get("tracker"), "format", type_long,
                "type", type, "songs", songs, "length", length);

        openmpt_module_set_render_param(
            module, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,
            type == "mod" ? 1 : 0);

        //auto& Settings = utils::Settings::getGroup("openmpt");
        auto separation = 100.0;
        openmpt_module_set_render_param(
           module, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, separation);
    }

    ~OpenMPTPlayer() override
    {
        if (module != nullptr) { openmpt_module_destroy(module); }
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        auto len = openmpt_module_read_interleaved_stereo(
            module, 44100, noSamples / 2, target);
        return len * 2;
    }

    bool seekTo(int song, int seconds) override
    {
        if (module != nullptr) {
            if (song >= 0) {
                openmpt_module_select_subsong(module, song);
            } else {
                openmpt_module_set_position_seconds(module, seconds);
            }
            return true;
        }
        return false;
    }

private:
    openmpt_module* module;
};

// Extensions libopenmpt advertises but this plugin deliberately does NOT claim.
//  - gz/rns: handled elsewhere or known-bad here (pre-existing).
//  - ".dtm" is NOT excluded anymore: it is mostly Digital Tracker / Digital
//    Home Studio (Atari Falcon, magic "D.T.") which only OpenMPT decodes
//    (modland "Digital Tracker DTM"). AdPlug owns the rarer DeFy DTM AdLib
//    variant and now content-gates its claim (magic "DeFy DTM "), and AdPlug
//    is registered first, so DeFy files still route there; the D.T. ones fall
//    through to here.
//  - the Amiga batch: the libopenmpt 0.8 upgrade added portable loaders for
//    formats UADEPlugin already plays via the original 68k replayers.
//    openmptplugin is registered before uadeplugin, so without this guard the
//    upgrade would silently re-route them away from UADE. Keep them with UADE
//    (no behavior change for existing files); revisit individually if the
//    portable decode is preferred. The genuinely-new formats led by Symphonie
//    (.symmod) and Digital Symphony (.dsym) are unaffected and flow through.
// getSupportedExtensions() subtracts this set too, so the advertised list and
// the routing decision stay consistent (coverage/priority_map reflect reality).
static const std::set<std::string>& excludedExts()
{
    static const std::set<std::string> s{
        "gz",   "rns",  "smod", "fc",   "fc13", "fc14",
        "puma", "kris", "unic", "gmc",  "ims",  "st26"};
    return s;
}

bool OpenMPTPlugin::canHandle(const std::string& n)
{
    auto name = utils::toLower(n);
    auto ext = utils::path_extension(name);
    if (excludedExts().count(ext) > 0) { return false; }
    // Deflemask .dmf (zlib-compressed, first byte 0x78) is an unrelated format
    // from X-Tracker .dmf ("DDMF"); it is played by dmfplugin (Furnace engine).
    // Decline it here so only the X-Tracker variant routes to libopenmpt. (The
    // fromFile() throw below stays as a defensive backstop.)
    if (ext == "dmf") {
        std::ifstream in(n, std::ios::binary);
        if (in) {
            unsigned char b = 0;
            in.read(reinterpret_cast<char*>(&b), 1);
            if (in && b == 0x78) { return false; }
        }
    }
    auto prefix = utils::path_prefix(name);
    if (prefix == "stk" || prefix == "mod" || ext == "ft") { return true; }
    // libopenmpt advertises "med" for OctaMED and its loader decodes the whole
    // MMD0..MMD3 family by content. But ".med" is also the pre-OctaMED "old MED"
    // format (magic "MED\x02".."MED\x04", modland "Music Editor/"), which
    // libopenmpt cannot decode (its Load_med only reads MMD0..MMD3). Before this
    // gate OpenMPT claimed those files by extension and then failed in fromFile,
    // and the host fell through to UADE, whose 68k MED player crashes on them
    // ("score crashed"). Decline the old-MED magic here so those route to
    // medplugin (libxmp med2/3/4). Genuine OctaMED files start with "MMD" and are
    // unaffected. If the header is unreadable (dry canHandle on a virtual path),
    // keep the old extension-claims-all behaviour so nothing regresses.
    if (ext == "med") {
        std::ifstream in(n, std::ios::binary);
        unsigned char hdr[4] = {0, 0, 0, 0};
        if (in && in.read(reinterpret_cast<char*>(hdr), 4)) {
            if (hdr[0] == 'M' && hdr[1] == 'E' && hdr[2] == 'D' &&
                hdr[3] >= 2 && hdr[3] <= 4) {
                return false;
            }
        }
    }
    // libopenmpt only advertises "med" for OctaMED, but its loader decodes the
    // whole MMD0..MMD3 family by content. UADE already claims mmd0/mmd1/mmd2
    // (and is registered later, so first-match routing keeps them there); MMD3
    // is unclaimed by any plugin, so route it here where it actually decodes.
    if (ext == "mmd3") { return true; }
    // modland's "Digital Symphony/" dir is almost all ".dsym" (which libopenmpt
    // advertises and plays), but 8 files from one composer dir carry the
    // extension MISSPELLED -- 7 ".dsyn" and 1 ".dysn" -- so they routed nowhere
    // and dead-ended in the GUI. The bytes are ordinary Digital Symphony: they
    // hit Load_dsym's exact magic and all 8 decode to audible audio, so claim
    // them here. Gate on the magic (Load_dsym's own Validate()) rather than the
    // extension alone, so a misnamed non-DSym file Skips instead of hard-failing.
    if (ext == "dsyn" || ext == "dysn") {
        auto data = readFileBytes(n);
        return data.size() >= 8 &&
               std::memcmp(data.data(), "\x02\x01\x13\x13\x14\x12\x01\x0B", 8) == 0;
    }
    // ".670" (modland "Composer 670 (CDFM)") is the demo/stripped layout of the
    // C67 format. libopenmpt's Load_c67 only reads the unpacked editor format, so
    // we convert .670 -> C67 in memory (see convert670ToC67 / fromFile) and let
    // libopenmpt decode the result. Claim only files that convert cleanly.
    if (ext == "670") { return convert670ToC67(readFileBytes(n)).has_value(); }
    // .psm is shared between two unrelated formats. libopenmpt owns Epic
    // MegaGames MASI (header magic "PSM " / "PSM\xFE"), but the ZX-Spectrum
    // "Pro Sound Maker" .psm (magic "psm1" at offset 12) is an AY tracker that
    // libopenmpt cannot decode -- before this guard those tunes routed here and
    // failed silently. Claim .psm only for the MASI magic and let the ZX variant
    // fall through to ZXTunePlugin, which plays it. If the header is unreadable
    // (e.g. virtual path during a dry canHandle), keep the old MASI-claims-all
    // behaviour so nothing regresses.
    if (ext == "psm") {
        std::ifstream in(n, std::ios::binary);
        char hdr[4] = {0, 0, 0, 0};
        if (in && in.read(hdr, 4)) {
            return std::memcmp(hdr, "PSM ", 4) == 0 ||
                   std::memcmp(hdr, "PSM\xFE", 4) == 0;
        }
        return true;
    }
    // ".dtm" is a THREE-way collision. OpenMPT decodes only Digital Tracker /
    // Digital Home Studio (Atari Falcon, magic "D.T." at offset 0). The other
    // two are AdPlug's DeFy DTM ("DeFy DTM ", claimed earlier by adplugin) and
    // DigiTrekker (MS-DOS, chunked "SONG"/"NAME"/"INFO"/... -- modland
    // "Digitrekker"), which NO vendored engine plays. Without this gate OpenMPT
    // claimed every .dtm and DigiTrekker files hard-FAILED ("error loading
    // file") instead of Skipping. Claim only the "D.T." magic; let the rest fall
    // through. Keep claim-all if the header is unreadable (dry canHandle on a
    // virtual path) so Digital Tracker routing never regresses.
    if (ext == "dtm") {
        std::ifstream in(n, std::ios::binary);
        char hdr[4] = {0, 0, 0, 0};
        if (in && in.read(hdr, 4)) {
            return std::memcmp(hdr, "D.T.", 4) == 0;
        }
        return true;
    }
    // ".gtk" is shared. libopenmpt decodes Graoumf Tracker (Atari Falcon, magic
    // "GTK" + version byte at offset 0, versions GTK1..GTK4). The modland
    // "Beaver Sweeper" corpus (author Steffo, just 4 tunes) also uses .gtk but
    // is an unrelated format (magic "+SNT") that NO vendored engine plays: not
    // libopenmpt, not UADE (PTK-Prowiz routes via the snt prefix but renders
    // pure silence), not libxmp/NostalgicPlayer, and it is undocumented on
    // modland. Without this gate OpenMPT claimed every .gtk and the +SNT tunes
    // hard-FAILED ("error loading file") instead of Skipping cleanly. Claim only
    // the "GTK" magic; let the rest fall through. See
    // data/misc/not_supported_extensions.txt.
    if (ext == "gtk") {
        std::ifstream in(n, std::ios::binary);
        char hdr[3] = {0, 0, 0};
        if (in && in.read(hdr, 3)) {
            return std::memcmp(hdr, "GTK", 3) == 0;
        }
        return true;
    }
    return openmpt_is_extension_supported(ext.c_str()) != 0;
}

std::vector<std::string>
OpenMPTPlugin::getSecondaryFiles(const std::string& file)
{
    // OpenMPT claims ".mod" and is registered before UADE, so the host asks it
    // (first canHandle match) for companions -- even for Startrekker AM modules
    // that OpenMPT itself declines and hands to UADE. Those need their external
    // synth-instrument file (".nt" on modland, ".as" for AudioSculpture) fetched
    // next to the song, or UADE renders the synth voices silent. Surface both
    // candidates -- a missing one is non-fatal -- using the same names UADE and
    // libopenmpt expect: the full module filename plus the companion suffix
    // (e.g. "war hawk.st1.3.mod" -> "war hawk.st1.3.mod.nt"). Only do this for
    // genuine AM modules; sampled MODs need nothing extra.
    auto data = utils::File(file).readAll();
    if (!isStartrekkerAM(data)) { return {}; }
    auto stem = utils::path_filename(file);
    return { stem + ".nt", stem + ".as" };
}

std::set<std::string> OpenMPTPlugin::getSupportedExtensions() const
{
    std::string s = openmpt_get_supported_extensions();
    auto exts = utils::split(s, ";");
    std::set<std::string> res;
    for (auto const& e : exts) {
        if (excludedExts().count(e) > 0) { continue; } // routed elsewhere
        res.insert(e);
    }
    // Composer 670 (CDFM): not advertised by libopenmpt itself, but we decode it
    // by converting .670 -> C67 (see convert670ToC67). Advertise so the host's
    // coverage/priority maps match what canHandle actually claims.
    res.insert("670");
    // Digital Symphony under modland's misspelled extensions (see canHandle):
    // libopenmpt advertises only "dsym", but the same loader decodes these.
    res.insert("dsyn");
    res.insert("dysn");
    return res;
}

ChipPlayer* OpenMPTPlugin::fromFile(std::string const& fileName)
{
    auto data = utils::File(fileName).readAll();
    auto ext = utils::path_extension(utils::toLower(fileName));
    // Deflemask DMF is zlib-compressed and shares the .dmf extension with
    // X-Tracker DMF (which starts with "DDMF"). Reject it before openmpt spins.
    if (ext == "dmf" && data.size() >= 1 && data[0] == 0x78) {
        throw player_exception("Deflemask DMF format is unsupported");
    }
    // Composer 670 (CDFM): repack the stripped .670 into the C67 image that
    // libopenmpt's Load_c67 decodes.
    if (ext == "670") {
        auto c67 = convert670ToC67(data);
        if (!c67) {
            throw player_exception("Not a valid Composer 670 (CDFM) file");
        }
        data = std::move(*c67);
    }
    return new OpenMPTPlayer{data};
};

} // namespace musix
//
extern "C" void openmptplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::OpenMPTPlugin>();
    });
}
