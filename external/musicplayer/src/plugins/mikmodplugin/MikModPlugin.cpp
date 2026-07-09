#include "MikModPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <mikmod.h>

#include <cstring>
#include <mutex>
#include <set>
#include <vector>

using namespace std;

namespace musix {

// libmikmod is global/non-reentrant: one module is "current" at a time, driven
// by the global virtual mixer. The host plays a single song at a time, so we
// init the library once and let each MikModPlayer own the current module.
static std::once_flag g_initFlag;
static bool g_initOk = false;

static void initMikMod()
{
    std::call_once(g_initFlag, [] {
        // Force the null driver: we pull PCM ourselves via VC_WriteBytes and
        // never want libmikmod opening the real soundcard. Register only the
        // UNI loader (the vendored slice omits mlreg.c / the other loaders).
        MikMod_RegisterDriver(&drv_nos);
        MikMod_RegisterLoader(&load_uni);
        md_mode |= DMODE_SOFT_MUSIC | DMODE_16BITS | DMODE_STEREO;
        md_mixfreq = 44100;
        if (MikMod_Init(const_cast<char*>("")) == 0) { g_initOk = true; }
    });
}

class MikModPlayer : public ChipPlayer {
public:
    MikModPlayer(const uint8_t* data, size_t size)
    {
        initMikMod();
        if (!g_initOk) { throw player_exception("MikMod init failed"); }

        module = Player_LoadMem(reinterpret_cast<const char*>(data),
                                static_cast<int>(size), 64, 0);
        if (module == nullptr) {
            throw player_exception(string("MikMod load failed: ") +
                                   MikMod_strerror(MikMod_errno));
        }
        module->wrap = 1;        // loop back to start instead of stopping
        module->loop = 1;        // honour in-pattern loops
        Player_Start(module);

        string title = module->songname != nullptr ? module->songname : "";
        string fmt = module->modtype != nullptr ? module->modtype : "UNIMOD";
        setMeta("title", utils::rstrip(title), "format", fmt, "channels",
                static_cast<uint32_t>(module->numchn));
    }

    ~MikModPlayer() override
    {
        if (module != nullptr) {
            Player_Stop();
            Player_Free(module);
        }
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        // noSamples counts int16 slots; VC_WriteBytes works in bytes.
        ULONG bytes =
            VC_WriteBytes(reinterpret_cast<SBYTE*>(target), noSamples * 2);
        return static_cast<int>(bytes / 2);
    }

private:
    MODULE* module = nullptr;
};

static bool hasUniMagic(const std::vector<uint8_t>& d)
{
    if (d.size() < 4) { return false; }
    // "UN0x" (MikCvt UNIMOD v2.0+) or legacy "APUN" (UNIMOD v0/1).
    return (d[0] == 'U' && d[1] == 'N' && d[2] == '0') ||
           std::memcmp(d.data(), "APUN", 4) == 0;
}

bool MikModPlugin::canHandle(const std::string& name)
{
    if (utils::path_extension(utils::toLower(name)) != "uni") { return false; }
    utils::File file{name};
    auto data = file.readAll();
    return hasUniMagic(data);
}

std::set<std::string> MikModPlugin::getSupportedExtensions() const
{
    return {"uni"};
}

ChipPlayer* MikModPlugin::fromFile(const std::string& fileName)
{
    utils::File file{fileName};
    auto data = file.readAll();
    return new MikModPlayer{data.data(), data.size()};
}

} // namespace musix

extern "C" void mikmodplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MikModPlugin>();
    });
}
