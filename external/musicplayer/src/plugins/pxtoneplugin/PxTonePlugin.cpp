#include "PxTonePlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include "pxtnService.h"
#include "pxtnError.h"

#include <coreutils/url.h>
#include <coreutils/utf8.h>

#include <cstring>
#include <vector>

namespace musix {

static bool _pxtn_r(void* user, void* p_dst, int32_t size, int32_t num)
{
    int i = fread(p_dst, size, num, (FILE*)user);
    if (i < num) {
        return false;
    }
    return true;
}

static bool _pxtn_w(void* user, const void* p_dst, int32_t size, int32_t num)
{
    int i = fwrite(p_dst, size, num, (FILE*)user);
    if (i < num) {
        return false;
    }
    return true;
}

static bool _pxtn_s(void* user, int32_t mode, int32_t size)
{
    if (fseek((FILE*)user, size, mode) < 0) {
        return false;
    }
    return true;
}

static bool _pxtn_p(void* user, int32_t* p_pos)
{
    int i = ftell((FILE*)user);
    if (i < 0) {
        return false;
    }
    *p_pos = i;
    return true;
}

class PxTonePlayer : public ChipPlayer {
public:
    explicit PxTonePlayer(const std::string& fileName) : f_src(nullptr), service(nullptr) {
        f_src = fopen(fileName.c_str(), "rb");
        if (!f_src) {
            throw player_exception("Could not open file: " + fileName);
        }

        service = new pxtnService(_pxtn_r, _pxtn_w, _pxtn_s, _pxtn_p);
        pxtnERR ret = service->init();
        if (ret != pxtnOK) {
            cleanup();
            throw player_exception("Failed to initialize pxtone library");
        }

        if (!service->set_destination_quality(2, 44100)) {
            cleanup();
            throw player_exception("Failed to set destination quality");
        }

        ret = service->read(f_src);
        if (ret != pxtnOK) {
            cleanup();
            throw player_exception(std::string("Failed to read pxtone file: ") + pxtnError_get_string(ret));
        }

        ret = service->tones_ready();
        if (ret != pxtnOK) {
            cleanup();
            throw player_exception(std::string("Failed to prepare pxtone tones: ") + pxtnError_get_string(ret));
        }

        pxtnVOMITPREPARATION prep;
        memset(&prep, 0, sizeof(prep));
        prep.flags |= pxtnVOMITPREPFLAG_loop;
        prep.start_pos_float = 0;
        prep.master_volume = 1.0f;
        if (!service->moo_preparation(&prep)) {
            cleanup();
            throw player_exception("Failed to prepare pxtone moo");
        }

        std::string title = "";
        std::string comment = "";
        if (service->text) {
            int32_t name_size = 0;
            const char* name = service->text->get_name_buf(&name_size);
            if (name && name_size > 0) {
                std::vector<uint8_t> temp_name(name, name + name_size);
                temp_name.push_back(0);
                title = utils::utf8_encode(utils::jis2unicode(temp_name.data()));
            }
            int32_t comment_size = 0;
            const char* comment_ptr = service->text->get_comment_buf(&comment_size);
            if (comment_ptr && comment_size > 0) {
                std::vector<uint8_t> temp_comment(comment_ptr, comment_ptr + comment_size);
                temp_comment.push_back(0);
                comment = utils::utf8_encode(utils::jis2unicode(temp_comment.data()));
            }
        }

        int32_t total_samples = service->moo_get_total_sample();
        int length_seconds = (total_samples > 0) ? (total_samples / 44100) : 0;

        setMeta("title", title, "comment", comment, "length", length_seconds, "format", "PxTone");
    }

    ~PxTonePlayer() override {
        cleanup();
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        int bytes = noSamples * sizeof(int16_t);
        if (!service->Moo(target, bytes)) {
            return 0;
        }
        return noSamples;
    }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds >= 0) {
            pxtnVOMITPREPARATION prep;
            memset(&prep, 0, sizeof(prep));
            prep.flags |= pxtnVOMITPREPFLAG_loop;
            prep.start_pos_sample = seconds * 44100;
            prep.master_volume = 1.0f;
            return service->moo_preparation(&prep);
        }
        return true;
    }

private:
    void cleanup() {
        if (service) {
            if (service->evels) service->evels->Release();
            delete service;
            service = nullptr;
        }
        if (f_src) {
            fclose(f_src);
            f_src = nullptr;
        }
    }

    FILE* f_src;
    pxtnService* service;
};

bool PxTonePlugin::canHandle(const std::string& name) {
    auto ext = utils::path_extension(name);
    return ext == "ptcop" || ext == "pttune";
}

std::set<std::string> PxTonePlugin::getSupportedExtensions() const {
    return {"ptcop", "pttune"};
}

ChipPlayer* PxTonePlugin::fromFile(const std::string& name) {
    return new PxTonePlayer{name};
}

} // namespace musix

extern "C" void pxtoneplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::PxTonePlugin>();
    });
}
