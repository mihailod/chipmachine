#include "PTKPlugin.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>

// ProTrekkr replayer, built in the in-memory BZR2 integration mode.
// files.h transitively pulls in variables.h -> replay.h, giving us the engine
// types (UINT8/UINT32/STDCALL), the constants (MAX_TRACKS, MAX_INSTRS,
// DEFAULT_POLYPHONY), the CustomFile reader and the Ptk_*/Load_Ptk API.
#include <files.h>

// ---------------------------------------------------------------------------
// Engine entry points that are not declared in the public headers but exported
// by the replayer / loader objects (plain C++ linkage, STDCALL is empty here).
UINT32 STDCALL Mixer(UINT8* Buffer, UINT32 Len);
int Alloc_Patterns_Pool(void);

extern unsigned char* RawPatterns; // pattern pool, malloc'd by Alloc_Patterns_Pool/Load_Ptk
extern UINT8* Mod_Memory;          // depacked module image, malloc'd by Load_Ptk
extern char customPtkName[20];     // module title parsed out of the .ptk header

// ---------------------------------------------------------------------------
// Globals normally supplied by the tracker (ptk.cpp), which we do not compile.
// This mirrors the reference Winamp / BZRPlayer glue in
// protrekkr/src/plugin/main.cpp so the loader/replayer objects link.
int AUDIO_Play_Flag = 0;
int AUDIO_Latency = 0;
int AUDIO_Milliseconds = 0;
int done = 0;
char artist[20] = {0};
char style[20] = {0};
char SampleName[128][16][64];
int Midiprg[MAX_INSTRS];
char nameins[MAX_INSTRS][20];
int Chan_Midi_Prg[MAX_TRACKS];
char Chan_History_State[256][MAX_TRACKS];

void Set_Default_Channels_Polyphony(void)
{
    for (int i = 0; i < MAX_TRACKS; i++) {
        Channels_Polyphony[i] = DEFAULT_POLYPHONY;
    }
}

// The GSM/MP3/AT3 sample decoders are Windows-only (msacm) and are compiled out
// of samples_unpack.cpp on this platform (see ptk_no_acm_codecs.h). The replayer
// still references these entry points under PTK_GSM/PTK_MP3, so provide no-op
// stubs. They are never reached: such samples simply decode to silence here.
void Unpack_GSM(UINT8* /*Source*/, short* /*Dest*/, int /*Src_Size*/, int /*Dst_Size*/) {}
void Unpack_MP3(UINT8* /*Source*/, short* /*Dest*/, int /*Src_Size*/, int /*Dst_Size*/, int /*BitRate*/) {}
void Unpack_AT3(UINT8* /*Source*/, short* /*Dest*/, int /*Src_Size*/, int /*Dst_Size*/, int /*BitRate*/) {}

namespace musix {

// The replayer keeps all of its state in globals, so only one module can be
// decoded at a time. chipmachine plays a single song at a time, but guard every
// entry point anyway so construction/teardown/rendering never overlap.
static std::mutex ptkMutex;

class PTKPlayer : public ChipPlayer
{
public:
    explicit PTKPlayer(const std::string& fileName, std::string format)
        : format_(std::move(format))
    {
        std::lock_guard<std::mutex> lock(ptkMutex);

        utils::File file{fileName};
        fileData = file.readAll();

        Ptk_InitDriver();
        Alloc_Patterns_Pool();

        CustomFile cf{fileData.data(), fileData.size()};
        if (!Load_Ptk(cf)) {
            cleanup();
            throw player_exception("ProTrekkr: unsupported or corrupt module");
        }
        loaded = true;

        // Reject a degenerate load. ProTrekkr's residual "old NoiseTrekker 1/2"
        // import path desyncs on the original Arguru .ntk format (its per-
        // instrument records don't match what the loader reads), leaving
        // Song_Tracks/Beats_Per_Min = 0 and every voice muted -> the module would
        // "play" as pure silence. Support for those antediluvian modules was
        // formally removed upstream in ProTrekkr 2.8.5 (only 3 hand-reworked beta
        // modules were ever salvaged, and those are the .ntk files that still
        // load). Fail cleanly here so the host Skips instead of sitting on a
        // silent track. A valid module has 1..MAX_TRACKS tracks and a positive
        // tempo.
        if (Song_Tracks < 1 || Song_Tracks > MAX_TRACKS || Beats_Per_Min <= 0) {
            cleanup();
            throw player_exception(
                "ProTrekkr: unsupported pre-conversion NoiseTrekker module");
        }

        Ptk_Play();
        done = 0;

        // NB: Calc_Length() is deliberately not called -- its upstream sequence
        // walk gets stuck in an infinite loop on some modules (certain pattern
        // loop / position-jump layouts), so we leave the song length unknown.
        std::string title = utils::rstrip(customPtkName);

        setMeta("title", title,
                "channels", static_cast<int>(Song_Tracks),
                "format", format_);
    }

    ~PTKPlayer() override
    {
        std::lock_guard<std::mutex> lock(ptkMutex);
        cleanup();
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        std::lock_guard<std::mutex> lock(ptkMutex);

        // The host counts interleaved int16 values; the Mixer counts stereo
        // frames and renders interleaved 32-bit floats.
        int frames = noSamples / 2;
        if (static_cast<int>(floatBuf.size()) < frames * 2) {
            floatBuf.resize(frames * 2);
        }

        UINT32 rendered = Mixer(reinterpret_cast<UINT8*>(floatBuf.data()),
                                static_cast<UINT32>(frames));
        int outSamples = static_cast<int>(rendered) * 2;

        for (int i = 0; i < outSamples; i++) {
            float s = floatBuf[i];
            if (s > 1.0f) s = 1.0f;
            else if (s < -1.0f) s = -1.0f;
            target[i] = static_cast<int16_t>(s * 32767.0f);
        }
        return outSamples;
    }

private:
    void cleanup()
    {
        if (loaded) {
            Ptk_Stop();
            Free_Samples();
        }
        // Free the engine's global buffers even if a load failed part-way
        // through (Alloc_Patterns_Pool / Load_Ptk allocate before we mark
        // ourselves loaded). The pointers are null-guarded by the engine.
        if (Mod_Memory) {
            free(Mod_Memory);
            Mod_Memory = nullptr;
        }
        if (RawPatterns) {
            free(RawPatterns);
            RawPatterns = nullptr;
        }
        loaded = false;
    }

    std::string format_;
    std::vector<uint8_t> fileData;
    std::vector<float> floatBuf;
    bool loaded = false;
};

PTKPlugin::PTKPlugin() = default;

bool PTKPlugin::canHandle(const std::string& name)
{
    auto ext = utils::toLower(utils::path_extension(name));
    return ext == "ptk" || ext == "ntk";
}

ChipPlayer* PTKPlugin::fromFile(const std::string& name)
{
    auto ext = utils::toLower(utils::path_extension(name));
    std::string format = (ext == "ntk") ? "NoiseTrekker" : "ProTrekkr";
    return new PTKPlayer(name, format);
}

} // namespace musix

extern "C" void ptkplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](const std::string& /*config*/) {
        return std::make_shared<musix::PTKPlugin>();
    });
}
