#include "MDXPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/log.h>
#include <coreutils/url.h>
#include <coreutils/utf8.h>
#include <coreutils/utils.h>

#include <fstream>
#include <set>
#include <string>
#include <vector>

extern "C"
{
#include "mdxmini.h"
}

namespace musix {

class MDXPlayer : public ChipPlayer
{
public:
    explicit MDXPlayer(const std::string& fileName)
    {
        mdx_set_rate(44100);

        if (mdx_open(&song,
                     fileName.c_str(),
                     utils::path_directory(fileName).c_str()))
        {
            throw player_exception();
        }

        char title[1024]{};

        int len = mdx_get_length(&song);
        mdx_get_title(&song, title);

        auto jis = utils::jis2unicode((uint8_t*)title);
        std::string title_utf8 = utils::utf8_encode(jis);

        LOGD("TITLE: %s", title_utf8.c_str());

        setMeta("sub_title",
                title_utf8,
                "length",
                len,
                "format",
                "MDX");
    }


    ~MDXPlayer() override
    {
        mdx_close(&song);
    }


    int getSamples(int16_t* target, int noSamples) override
    {
        /*
         * ChipPlayer uses number of int16 samples.
         *
         * MDXMini outputs stereo:
         *
         *   left + right = 2 int16 samples per frame
         *
         * Therefore convert total samples to stereo frames.
         */

        noSamples &= ~1; // keep stereo alignment

        mdx_calc_sample(&song,
                        target,
                        noSamples / 2);

        return noSamples;
    }


    int getHZ() override
    {
        return 44100;
    }


    bool seekTo(int /*song*/, int /*seconds*/) override
    {
        return false;
    }


private:
    t_mdxmini song{};
};


static const std::set<std::string> supported_ext = {
    "mdx"
};


bool MDXPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::path_extension(name)) > 0;
}


std::set<std::string> MDXPlugin::getSupportedExtensions() const
{
    return supported_ext;
}


std::vector<std::string> MDXPlugin::getSecondaryFiles(const std::string& name)
{
    std::vector<uint8_t> header(2048);

    std::ifstream f(name, std::ios::in | std::ios::binary);

    if (!f)
        return {};

    f.read(reinterpret_cast<char*>(header.data()), header.size());


    /*
     * MDX files reference PDX samples after:
     *
     * 0x0d 0x0a 0x1a
     */

    for (size_t i = 0; i < header.size() - 3; i++)
    {
        if (header[i] == 0x0d &&
            header[i + 1] == 0x0a &&
            header[i + 2] == 0x1a)
        {
            if (header[i + 3] != 0)
            {
                std::string pdxFile(
                    reinterpret_cast<char*>(&header[i + 3])
                );

                utils::makeLower(pdxFile);

                if (!utils::endsWith(pdxFile, ".pdx"))
                    pdxFile += ".pdx";

                return { pdxFile };
            }

            break;
        }
    }

    return {};
}


ChipPlayer* MDXPlugin::fromFile(const std::string& name)
{
    return new MDXPlayer{name};
}


} // namespace musix


extern "C" void mdxplugin_register()
{
    musix::ChipPlugin::addPluginConstructor(
        [](std::string const& /*config*/) {
            return std::make_shared<musix::MDXPlugin>();
        });
}