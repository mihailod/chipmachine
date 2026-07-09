#include "QuartetPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/log.h>
#include "zingzong.h"

#include <set>
#include <cstdarg>

extern "C" {
zz_vfs_dri_t zz_file_vfs(void);
zz_vfs_dri_t zz_ice_vfs(void);
}

namespace musix {

static void zz_log(zz_u8_t level, void* user, const char* fmt, va_list args) {
    // Only log errors to stderr
    if (level == ZZ_LOG_ERR) {
        fprintf(stderr, "ZingZong Error: ");
        vfprintf(stderr, fmt, args);
    }
}

class QuartetPlayer : public ChipPlayer {
public:
    QuartetPlayer(const std::string& fileName) {

        static bool initialized = false;
        if (!initialized) {
            zz_log_fun(zz_log, nullptr);
            zz_log_bit(0, 0xFF);
            zz_vfs_add(zz_file_vfs());
            zz_vfs_add(zz_ice_vfs());
            initialized = true;
        }

        zz_err_t err = zz_new(&play);
        if (err != ZZ_OK) throw player_exception("Quartet: Could not create player");

        uint8_t fmt = 0;
        
        std::string vsetFile = "";
        std::string ext = utils::toLower(utils::path_extension(fileName));
        if (ext == "4v") {
            std::string base = utils::path_basename(fileName);
            std::string dir = utils::path_directory(fileName);
            std::string pre = dir + (dir.empty() ? "" : "/");
            // The voiceset is usually a per-song "<base>.set", but some
            // directories share one fixed "SMP.set" bank across every .4v in
            // them (e.g. modland "Quartet ST/.../Projectyle/SMP.set"). Try the
            // per-song name first, then the shared bank; both in either case.
            for (auto const& cand : {base + ".set", base + ".SET",
                                     std::string("SMP.set"),
                                     std::string("smp.set")}) {
                if (utils::File::exists(pre + cand)) {
                    vsetFile = pre + cand;
                    break;
                }
            }
        }

        err = zz_load(play, fileName.c_str(), vsetFile.empty() ? nullptr : vsetFile.c_str(), &fmt);
        if (err != ZZ_OK) {
            zz_del(&play);
            throw player_exception("Quartet: Could not load song");
        }

        zz_init(play, 0, ZZ_EOF); // Measured length
        zz_setup(play, ZZ_MIXER_DEF, 44100);

        zz_info_t info;
        zz_info(play, &info);

        setMeta("title", info.tag.title,
                "composer", info.tag.artist,
                "format", "Quartet ST",
                "length", info.len.ms / 1000);
    }

    ~QuartetPlayer() override {
        if (play) zz_del(&play);
    }

    int getSamples(int16_t* target, int noSamples) override {
        int16_t framesGenerated = zz_play(play, target, noSamples / 2);
        
        static int nonZeroDetected = 0;
        if (nonZeroDetected < 5 && framesGenerated > 0) {
            for (int i = 0; i < framesGenerated * 2; ++i) {
                if (target[i] != 0) {
                    //fprintf(stderr, "AUDIBLE\n");
                    nonZeroDetected++;
                    break;
                }
            }
        }

        if (framesGenerated <= 0) return 0;
        return framesGenerated * 2;
    }

    virtual bool seekTo(int /*song*/, int seconds) override {
        return false;
    }

private:
    zz_play_t play = nullptr;
};

bool QuartetPlugin::canHandle(const std::string& name) {
    auto ext = utils::toLower(utils::path_extension(name));
    return ext == "4v" || ext == "4q";
}

ChipPlayer* QuartetPlugin::fromFile(const std::string& fileName) {
    try {
        return new QuartetPlayer{fileName};
    } catch (player_exception& e) {
        return nullptr;
    }
}

std::vector<std::string> QuartetPlugin::getSecondaryFiles(const std::string& file) {
    auto ext = utils::toLower(utils::path_extension(file));
    if (ext == "4v") {
        auto base = utils::path_basename(file);
        // Declare both companion conventions: the per-song "<base>.set" and the
        // shared "SMP.set" bank some directories use for all their .4v songs.
        // A missing companion is non-fatal, so requesting both is safe -- one of
        // them resolves the voiceset; the other just 550s and is ignored.
        std::vector<std::string> files{base + ".set"};
        if (utils::toLower(base) != "smp") { files.emplace_back("SMP.set"); }
        return files;
    }
    return {};
}

} // namespace musix

extern "C" void quartetplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::QuartetPlugin>();
    });
}
