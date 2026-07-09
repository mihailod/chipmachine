#include "PokeyNoisePlugin.h"
#include "../../chipplayer.h"

#include "asap/asap.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace musix {

// PokeyNoise .pn files are raw Atari 8-bit executables ("XEX"): a 6-byte
// signature followed by load segments, and a $02E0/$02E1 RUNAD vector pointing
// at the init routine. The init clamps the subsong number, calls an init
// subroutine, then installs a ~50Hz VBL handler via SETVBV ($E45C); the handler
// is just `JSR <play> ; JMP SYSVBV ($E45F)`.
//
// ASAP has no loader for this format, but it can play a SAP "type B" module
// (explicit INIT address + a PLAYER routine called once per frame) and, crucially,
// emulates no OS ROM -- it zeroes memory and copies in the XEX segments verbatim.
// So we synthesize a SAP-B image around the original bytes:
//   * INIT   = RUNAD,
//   * PLAYER = the VBL handler address (parsed out of the init code),
//   * SONGS  = the subsong count (the `CMP #n` in the init, if present),
// and we inject two tiny stub segments so the player's OS calls behave:
//   * SETVBV ($E45C) -> RTS  (we install the handler ourselves via PLAYER),
//   * SYSVBV ($E45F) -> increment the RTCLOK jiffy clock ($12/$13/$14) then RTS,
//     which some players (e.g. Peter Langston's) gate playback on.

namespace {

constexpr uint8_t PN_MAGIC[6] = {0xFF, 0xFF, 0xE0, 0x02, 0xE1, 0x02};

bool hasMagic(const uint8_t* data, size_t len)
{
    return len >= sizeof(PN_MAGIC) && memcmp(data, PN_MAGIC, sizeof(PN_MAGIC)) == 0;
}

// Read the SAP "TYPE" tag from a real .sap text header. Returns the type letter
// or 0 if it is not a readable SAP. GME owns the common register types B and C;
// this plugin's ASAP core plays the rest (notably 'D' Digimusic), so we claim a
// .sap only when GME would reject it -- keeping the two plugins mutually
// exclusive regardless of registration order.
char sapType(const uint8_t* data, size_t len)
{
    if (len < 4 || memcmp(data, "SAP", 3) != 0) { return 0; }
    size_t cap = len < 1024 ? len : 1024;
    for (size_t i = 0; i + 5 < cap; i++) {
        if (data[i] == 0xFF && data[i + 1] == 0xFF) { break; }
        if (memcmp(data + i, "TYPE ", 5) == 0) {
            char t = static_cast<char>(data[i + 5]);
            return (t >= 'A' && t <= 'Z') ? t : 0;
        }
    }
    return 0;
}

bool isAsapOnlySap(char t)
{
    return t != 0 && t != 'B' && t != 'C';
}

// Sizes of the documented 6502 opcodes (1..3 bytes). Undocumented opcodes, which
// the regular PokeyNoise init routines never use, default to 1 so the walk
// always terminates.
const uint8_t OP_SIZE[256] = {
    1,2,1,1,1,2,2,1,1,2,1,1,1,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1,
    3,2,1,1,2,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1,
    1,2,1,1,1,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1,
    1,2,1,1,1,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1,
    2,2,1,1,2,2,2,1,1,1,1,1,3,3,3,1, 2,2,1,1,2,2,2,1,1,3,1,1,1,3,1,1,
    2,2,2,1,2,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,2,2,2,1,1,3,1,1,3,3,3,1,
    2,2,1,1,2,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1,
    2,2,1,1,2,2,2,1,1,2,1,1,3,3,3,1, 2,2,1,1,1,2,2,1,1,3,1,1,1,3,3,1};

// Parse INIT/PLAYER/SONGS from a PokeyNoise XEX and build a SAP type-B image
// that ASAP can load. Returns false (leaving 'out' untouched) on anything that
// doesn't look like a PokeyNoise file.
bool buildSap(const std::vector<uint8_t>& xex, std::vector<uint8_t>& out)
{
    if (!hasMagic(xex.data(), xex.size())) {
        return false;
    }

    // Replay the XEX load to recover RUNAD and the bytes of the first segment
    // (which holds the init code we need to inspect).
    std::vector<uint8_t> mem(65536, 0);
    size_t i = 2;
    int firstStart = -1;
    while (i + 4 <= xex.size()) {
        int s = xex[i] | (xex[i + 1] << 8);
        int e = xex[i + 2] | (xex[i + 3] << 8);
        i += 4;
        int n = e - s + 1;
        if (n <= 0 || i + static_cast<size_t>(n) > xex.size()) {
            break;
        }
        memcpy(mem.data() + s, xex.data() + i, n);
        if (firstStart < 0) {
            firstStart = s;
        }
        i += n;
        if (i + 2 <= xex.size() && xex[i] == 0xFF && xex[i + 1] == 0xFF) {
            i += 2;
        }
    }

    int runad = mem[0x2E0] | (mem[0x2E1] << 8);
    if (runad == 0) {
        return false;
    }

    // Walk the init code from RUNAD: the first `CMP #n` gives the subsong count,
    // the `LDX #hi`/`LDY #lo` immediately before the SETVBV ($E45C) call give the
    // VBL handler (the per-frame player) address.
    int songs = 1, hi = -1, lo = -1, handler = -1;
    bool gotCmp = false;
    int a = runad;
    for (int k = 0; k < 64 && a < 0xFFFD; k++) {
        uint8_t op = mem[a];
        if (op == 0xC9 && !gotCmp) { // CMP #imm
            songs = mem[a + 1];
            gotCmp = true;
        } else if (op == 0xA2) { // LDX #imm
            hi = mem[a + 1];
        } else if (op == 0xA0) { // LDY #imm
            lo = mem[a + 1];
        }
        // JMP/JSR $E45C (SETVBV)
        if ((op == 0x4C || op == 0x20) && mem[a + 1] == 0x5C && mem[a + 2] == 0xE4) {
            if (hi >= 0 && lo >= 0) {
                handler = (hi << 8) | lo;
            }
            break;
        }
        a += OP_SIZE[op];
    }
    if (handler < 0) {
        return false;
    }
    if (songs < 1 || songs > 32) {
        songs = 1;
    }

    char header[96];
    int hlen = snprintf(header, sizeof(header),
                        "SAP\r\nTYPE B\r\nINIT %04X\r\nPLAYER %04X\r\nSONGS %d\r\n",
                        runad, handler, songs);

    // Stub OS-vector segments appended after the original XEX data.
    static const uint8_t stubs[] = {
        0xFF, 0xFF, 0x5C, 0xE4, 0x5C, 0xE4, 0x60,             // SETVBV: RTS
        0xFF, 0xFF, 0x5F, 0xE4, 0x69, 0xE4,                   // SYSVBV ($E45F-$E469):
        0xE6, 0x14, 0xD0, 0x06, 0xE6, 0x13, 0xD0, 0x02,       //   INC $14 / INC $13 /
        0xE6, 0x12, 0x60};                                    //   INC $12 / RTS

    out.clear();
    out.reserve(hlen + xex.size() + sizeof(stubs));
    out.insert(out.end(), header, header + hlen);
    out.insert(out.end(), xex.begin(), xex.end());
    out.insert(out.end(), stubs, stubs + sizeof(stubs));
    return true;
}

} // namespace

class PokeyNoisePlayer : public ChipPlayer
{
public:
    explicit PokeyNoisePlayer(const std::vector<uint8_t>& fileData,
                              const std::string& fileName)
    {
        asap = ASAP_New();
        if (asap == nullptr) {
            throw player_exception("Could not create ASAP");
        }
        ASAP_SetSampleRate(asap, 44100);

        // A real SAP module (any type) goes straight to ASAP's own parser. A
        // PokeyNoise XEX has no SAP header, so we synthesize a type-B image
        // around it first.
        bool isSap = fileData.size() >= 3 &&
                     memcmp(fileData.data(), "SAP", 3) == 0;
        std::string format = "PokeyNoise";
        std::vector<uint8_t> sap;
        const uint8_t* loadData = nullptr;
        int loadSize = 0;
        const char* loadName = nullptr;
        if (isSap) {
            loadData = fileData.data();
            loadSize = static_cast<int>(fileData.size());
            loadName = "song.sap";
            format = "SAP";
        } else {
            if (!buildSap(fileData, sap)) {
                ASAP_Delete(asap);
                asap = nullptr;
                throw player_exception("Not a PokeyNoise file");
            }
            loadData = sap.data();
            loadSize = static_cast<int>(sap.size());
            // The ".sap" name tells ASAP to use its SAP parser on our image.
            loadName = "pokeynoise.sap";
        }

        if (!ASAP_Load(asap, loadName, loadData, loadSize)) {
            ASAP_Delete(asap);
            asap = nullptr;
            throw player_exception("Could not load " + format + ": " + fileName);
        }

        const ASAPInfo* info = ASAP_GetInfo(asap);
        songCount = ASAPInfo_GetSongs(info);
        int startSong = ASAPInfo_GetDefaultSong(info);
        if (startSong < 0 || startSong >= songCount) { startSong = 0; }

        if (!ASAP_PlaySong(asap, startSong, -1)) {
            ASAP_Delete(asap);
            asap = nullptr;
            throw player_exception("Could not start " + format + ": " + fileName);
        }

        if (isSap) {
            const char* title = ASAPInfo_GetTitle(info);
            const char* author = ASAPInfo_GetAuthor(info);
            int durMs = ASAPInfo_GetDuration(info, startSong);
            setMeta("title",
                    (title != nullptr && title[0] != 0)
                        ? std::string(title)
                        : utils::path_basename(fileName),
                    "composer", author != nullptr ? author : "", "length",
                    durMs > 0 ? durMs / 1000 : 0, "songs", songCount, "startSong",
                    startSong, "format", format);
        } else {
            std::string title = utils::path_basename(fileName);
            setMeta("title", title, "songs", songCount, "startSong", startSong,
                    "format", format);
        }
    }

    ~PokeyNoisePlayer() override
    {
        if (asap != nullptr) {
            ASAP_Delete(asap);
        }
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // ASAP renders mono; the host wants interleaved stereo. Render half the
        // requested int16s into the back of the buffer, then fan out to stereo.
        int frames = noSamples / 2;
        int wantBytes = frames * 2; // mono int16 -> bytes
        auto* bytes = reinterpret_cast<uint8_t*>(target);
        int gotBytes =
            ASAP_Generate(asap, bytes, wantBytes, ASAPSampleFormat_S16_L_E);
        int gotFrames = gotBytes / 2;
        for (int j = gotFrames - 1; j >= 0; j--) {
            int16_t s = target[j];
            target[j * 2] = s;
            target[j * 2 + 1] = s;
        }
        return gotFrames * 2;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song >= 0 && song < songCount) {
            return ASAP_PlaySong(asap, song, -1);
        }
        return false;
    }

private:
    ASAP* asap = nullptr;
    int songCount = 1;
};

static const std::set<std::string> supported_ext{"pn", "sap"};

bool PokeyNoisePlugin::canHandle(const std::string& name)
{
    auto lowerName = utils::toLower(name);
    auto ext = utils::path_extension(lowerName);

    // A .sap is claimed only for the types GME cannot play (Digimusic etc.);
    // register types B/C stay with GME. Confirm against the file's TYPE tag.
    if (ext == "sap") {
        FILE* fp = fopen(name.c_str(), "rb");
        if (fp == nullptr) { return false; }
        uint8_t hdr[1024];
        size_t n = fread(hdr, 1, sizeof(hdr), fp);
        fclose(fp);
        return isAsapOnlySap(sapType(hdr, n));
    }

    // Modland names these `pn.<song>` (prefix); `<song>.pn` (suffix) also occurs.
    bool nameMatches = supported_ext.count(ext) > 0 ||
                       supported_ext.count(utils::path_prefix(lowerName)) > 0;
    if (!nameMatches) {
        return false;
    }
    // Confirm via the content magic so we don't grab unrelated files.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t magic[sizeof(PN_MAGIC)];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && hasMagic(magic, n);
}

std::set<std::string> PokeyNoisePlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* PokeyNoisePlugin::fromFile(const std::string& fileName)
{
    return new PokeyNoisePlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void pokeynoiseplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::PokeyNoisePlugin>();
    });
}
