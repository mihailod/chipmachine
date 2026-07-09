#include "DMFPlugin.h"
#include "../../chipplayer.h"

#include <cctype>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h>
#include <zlib.h>

// Vendored Furnace engine (tildearrow/furnace, GPLv2). engine.h transitively
// pulls Furnace's bundled fmt via ta-log.h; this TU deliberately avoids the
// coreutils headers (which carry chipmachine's *own* fmt) so only one fmt is
// ever on the include path here -- mixing the two versions does not compile.
#include "engine.h"

namespace musix {

namespace {

// DefleMask .dmf are zlib streams (first byte 0x78) whose inflated payload
// starts with the 16-byte magic ".DelekDefleMask.". X-Tracker .dmf start with
// "DDMF" (handled by OpenMPT). We accept on the zlib byte alone -- matching the
// existing OpenMPT fast-fail gate -- which is cheap and unambiguous versus the
// "DDMF" sibling. (Every DefleMask module since the format went public is
// zlib-wrapped; the modland Deflemask/ corpus is uniformly so.)
bool isDeflemask(unsigned char const* data, size_t size)
{
    return size >= 2 && data[0] == 0x78;
}

// A significant fraction of the modland Deflemask corpus is damaged: the zlib
// stream's deflate blocks decode, but the trailing Adler-32 checksum is wrong,
// meaning the decompressed bytes are corrupt. Furnace then rejects them with a
// generic "not a compatible song" further into the parse. Detect the bad
// checksum up front so the failure is reported as what it actually is (a broken
// file) rather than a plugin/format problem. Returns true only for the
// "deflate-decodes-but-checksum-fails" signature.
bool zlibChecksumBad(unsigned char const* data, size_t size)
{
    if (size < 2 || data[0] != 0x78) { return false; }
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) { return false; }
    zs.next_in = const_cast<Bytef*>(data);
    zs.avail_in = static_cast<uInt>(size);
    unsigned char out[16384];
    int rc = Z_OK;
    bool sawData = false;
    while (rc == Z_OK) {
        zs.next_out = out;
        zs.avail_out = sizeof(out);
        rc = inflate(&zs, Z_NO_FLUSH);
        if (zs.avail_out < sizeof(out)) { sawData = true; }
    }
    inflateEnd(&zs);
    // Z_DATA_ERROR after producing output == valid deflate, corrupt payload
    // (bad Adler-32). Z_STREAM_END would mean a healthy stream.
    return rc == Z_DATA_ERROR && sawData;
}

std::string lowerExt(std::string const& path)
{
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) { return ""; }
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) { c = static_cast<char>(tolower((unsigned char)c)); }
    return ext;
}

std::string baseName(std::string const& path)
{
    auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::vector<unsigned char> readFile(std::string const& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(in)),
                                      std::istreambuf_iterator<char>());
}

// DivEngine::loadDMF (like all Furnace loaders) puts a full DivSong -- ~744 KB
// -- on the stack. chipmachine calls fromFile() from a MusicPlayerList worker
// thread whose default stack (~512 KB on macOS) is far too small, so the load
// overflows the stack and faults (SIGBUS "thread stack size exceeded"). cmtest
// and the CLI load on the 8 MB main thread and never hit it. Run the load on a
// dedicated thread with an ample stack so it works from any caller.
struct LoadCtx
{
    DivEngine* engine;
    unsigned char* buf;
    size_t len;
    std::string hint;
    bool result;
};

void* loadThreadFn(void* p)
{
    auto* c = static_cast<LoadCtx*>(p);
    c->result = c->engine->load(c->buf, c->len, c->hint.c_str());
    return nullptr;
}

bool loadOnLargeStack(DivEngine* engine, unsigned char* buf, size_t len,
                      const std::string& hint)
{
    LoadCtx ctx{ engine, buf, len, hint, false };
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) { return engine->load(buf, len, hint.c_str()); }
    pthread_attr_setstacksize(&attr, 16 * 1024 * 1024);
    pthread_t th;
    int rc = pthread_create(&th, &attr, loadThreadFn, &ctx);
    pthread_attr_destroy(&attr);
    if (rc != 0) { return engine->load(buf, len, hint.c_str()); } // best-effort fallback
    pthread_join(th, nullptr);
    return ctx.result;
}

} // namespace

// A single DefleMask module rendered through a private DivEngine instance. The
// engine is driven headless: no live audio backend (DIV_AUDIO_DUMMY), pulled
// one buffer at a time from getSamples via DivEngine::nextBuf.
class DMFPlayer : public ChipPlayer
{
public:
    explicit DMFPlayer(const std::string& fileName)
    {
        auto fileData = readFile(fileName);
        if (fileData.size() < 2 || !isDeflemask(fileData.data(), fileData.size())) {
            throw player_exception("Not a DefleMask DMF");
        }

        engine = new DivEngine();
        engine->preInit(true);
        engine->setAudio(DIV_AUDIO_DUMMY);
        // Render at the host rate; DUMMY init copies want -> got.
        engine->getAudioDescWant().rate = 44100.0;
        engine->getAudioDescWant().outChans = 2;
        engine->getAudioDescWant().bufsize = 1024;

        // DivEngine::load() takes ownership of the buffer (it inflates it and
        // delete[]s the original), so hand it a fresh new[] copy.
        auto* buf = new unsigned char[fileData.size()];
        memcpy(buf, fileData.data(), fileData.size());
        if (!loadOnLargeStack(engine, buf, fileData.size(), baseName(fileName))) {
            std::string err = engine->getLastError();
            bool corrupt = zlibChecksumBad(fileData.data(), fileData.size());
            delete engine;
            engine = nullptr;
            if (corrupt) {
                throw player_exception(
                    "corrupt DefleMask file (damaged data, bad zlib checksum)");
            }
            throw player_exception("DefleMask DMF load failed: " + err);
        }

        if (!engine->init()) {
            delete engine;
            engine = nullptr;
            throw player_exception("Furnace engine init failed");
        }

        hz = static_cast<int>(engine->getAudioDescGot().rate);
        if (hz <= 0) { hz = 44100; }

        engine->play();

        for (int i = 0; i < 2; i++) { scratch[i].resize(kMaxFrames); }
    }

    ~DMFPlayer() override
    {
        if (engine != nullptr) {
            engine->quit();
            delete engine;
        }
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (engine == nullptr) { return -1; }

        // Furnace loops trackers indefinitely by default; isPlaying() goes false
        // only for genuinely non-looping modules that ran off the end. Report
        // SONG_END so the host stops cleanly rather than streaming silence.
        if (!engine->isPlaying()) { return -1; }

        int frames = noSamples / 2;
        int done = 0;
        while (done < frames) {
            int chunk = frames - done;
            if (chunk > kMaxFrames) { chunk = kMaxFrames; }

            float* out[2] = { scratch[0].data(), scratch[1].data() };
            engine->nextBuf(nullptr, out, 0, 2, chunk);

            for (int i = 0; i < chunk; i++) {
                target[(done + i) * 2 + 0] = toS16(out[0][i]);
                target[(done + i) * 2 + 1] = toS16(out[1][i]);
            }
            done += chunk;
        }
        return frames * 2;
    }

    int getHZ() override { return hz; }

private:
    static constexpr int kMaxFrames = 4096;

    static int16_t toS16(float v)
    {
        if (v > 1.0f) { v = 1.0f; }
        if (v < -1.0f) { v = -1.0f; }
        return static_cast<int16_t>(v * 32767.0f);
    }

    DivEngine* engine = nullptr;
    std::vector<float> scratch[2];
    int hz = 44100;
};

bool DMFPlugin::canHandle(const std::string& name)
{
    if (lowerExt(name) != "dmf") { return false; }
    auto data = readFile(name);
    return !data.empty() && isDeflemask(data.data(), data.size());
}

std::set<std::string> DMFPlugin::getSupportedExtensions() const
{
    return { "dmf" };
}

ChipPlayer* DMFPlugin::fromFile(const std::string& fileName)
{
    return new DMFPlayer{ fileName };
}

} // namespace musix

extern "C" void dmfplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::DMFPlugin>();
    });
}
