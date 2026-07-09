#include "AyflyPlugin.h"

#include <coreutils/utils.h>

#include "ayfly.h"

#include <fstream>
#include <set>

namespace musix {

class AyflyPlayer : public ChipPlayer
{
public:
    explicit AyflyPlayer(const std::string& fileName)
    {
        aysong = ay_initsong(fileName.c_str(), 44100);
        if (aysong == nullptr) { throw player_exception("Not an AY file"); }
        const auto* songName = ay_getsongname(aysong);
        const auto* songAuthor = ay_getsongauthor(aysong);
        unsigned long len = ay_getsonglength(aysong) / 50;
        if (len > 1000) { len = 0; }
        setMeta("title", songName, "composer", songAuthor, "length", len,
                "format", "AY (Spectrum)");
    }

    ~AyflyPlayer() override
    {
        if (aysong != nullptr) ay_closesong(&aysong);
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        int rc = ay_rendersongbuffer(
            aysong, reinterpret_cast<unsigned char*>(target), noSamples);
        return rc / 2;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    void* aysong{nullptr};
    bool started{false};
    bool ended{false};
};

// NB: .ftc (Fast Tracker) is intentionally NOT here. libayfly throws on every
// Fast Tracker module ("Module: ..." header) -- a verified 0/12 vs ZXTune's
// 12/12 on spread modland samples -- so it is routed to ZXTunePlugin instead
// (aym/fasttracker player). Re-adding it here would steal those files back
// (ayfly is registered first at equal priority) and break playback.
//
// NB: .ay (the ZXAYEMUL container of raw Z80 player rips, e.g. ProjectAY /
// Ironfist) is intentionally NOT here either. libayfly plays ZX .ay but renders
// Amstrad CPC .ay SILENT (19/20 NO SOUND on the SoLO/CORPSE collection), whereas
// GME's Ay_Emul-lineage core plays BOTH reliably (86/86 across Ironfist ZX, CPC
// and zxart ZX). So .ay is owned by gmeplugin; ayfly keeps the tracker AY
// formats GME cannot decode (pt3/stc/vtx/psg/...).
static const std::set<std::string> supported_ext = {
    "stp2", "psg", "asc", "stc", "psc", "sqt", "stp",
    "pt1",  "pt2", "pt3", "vtx", "vt2", "zxs", "st13", "fxm", "amad"};

bool AyflyPlugin::canHandle(const std::string& name)
{
    if (utils::toLower(name).find("/quartet") != std::string::npos)
        return false;
    auto ext = utils::path_extension(name);
    if (supported_ext.count(ext) == 0) return false;
    // ".pt2" is shared by two unrelated formats. Ayfly owns ZX-Spectrum
    // ProTracker 2 (.pt2, a binary AY module), but Modland's "Picatune2" tunes
    // also use .pt2 -- those are Shiru's 1-bit beeper synth stored as XML
    // project source (the file starts with "<track ...>"). Ayfly cannot play
    // them: ay_initsong() rejects the leading '<' and throws "Not an AY file",
    // and since no other plugin handles Picatune2 the tune fails to load with a
    // misleading error instead of Skipping. Decline the XML variant here so it
    // Skips cleanly. If the header is unreadable (e.g. a virtual path during a
    // dry canHandle), keep claiming so genuine PT2 modules don't regress.
    if (ext == "pt2") {
        std::ifstream in(name, std::ios::binary);
        unsigned char hdr[4] = {0, 0, 0, 0};
        if (in && in.read(reinterpret_cast<char*>(hdr), 4)) {
            int i = 0; // skip a UTF-8 BOM if present
            if (hdr[0] == 0xEF && hdr[1] == 0xBB && hdr[2] == 0xBF) i = 3;
            if (hdr[i] == '<') return false; // Picatune2 XML -> not an AY file
        }
    }
    return true;
}

std::set<std::string> AyflyPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* AyflyPlugin::fromFile(const std::string& name)
{
    return new AyflyPlayer{name};
};

} // namespace musix
extern "C" void ayflyplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::AyflyPlugin>();
    });
}
