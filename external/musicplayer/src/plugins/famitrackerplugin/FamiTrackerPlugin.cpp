#include "FamiTrackerPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "core/io.hpp"
#include "core/soundsink.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/SoundGen.hpp"
#include "famitracker-core/TrackerController.hpp"

// FamiTracker (.ftm) player.
//
// Thin in-process wrapper around the vendored FamiTracker CX engine
// (famitracker-cx/, see famitracker-cx/PROVENANCE.md). The engine's real-time,
// thread-driven sink was replaced with a synchronous render path
// (SoundGen::beginRender / renderSamples) so we can pull samples on demand. The
// APU renders mono; we duplicate to interleaved stereo for the host.

namespace musix {

namespace {

constexpr int kRate = 44100;

// A SoundSink that owns no thread and no device -- it exists only so SoundGen has
// a sample rate and the callback plumbing it expects. Audio is pulled directly
// via SoundGen::renderSamples, not pushed through this sink.
class PullSink : public core::SoundSink
{
public:
    explicit PullSink(int rate) : m_rate(rate) {}
    int sampleRate() const override { return m_rate; }

private:
    int m_rate;
};

// True if the first bytes are the FamiTracker module signature. (Face The Music
// .ftm files begin with "FTMN" and are handled by OpenMPT instead.)
bool looksLikeFtm(const uint8_t* p, size_t n)
{
    static const char kMagic[] = "FamiTracker Module";
    const size_t len = sizeof(kMagic) - 1; // 18, no NUL
    return n >= len && std::memcmp(p, kMagic, len) == 0;
}

} // namespace

class FamiTrackerPlayer : public ChipPlayer
{
public:
    explicit FamiTrackerPlayer(const std::string& fileName) : m_sink(kRate)
    {
        core::FileIO io(fileName.c_str(), core::IO_READ);
        if (!io.isReadable()) {
            throw player_exception("Cannot open FamiTracker file: " + fileName);
        }

        m_doc = new FtmDocument;
        try {
            m_doc->read(&io);
        } catch (const FtmDocumentException& e) {
            delete m_doc;
            m_doc = nullptr;
            throw player_exception(std::string("Could not load FamiTracker module: ") +
                                   e.what());
        }

        m_trackCount = m_doc->GetTrackCount();
        m_doc->SelectTrack(0);

        m_sg = new SoundGen;
        m_sg->setSoundSink(&m_sink);
        m_sg->setDocument(m_doc);
        m_sg->trackerController()->startAt(0, 0);
        m_sg->beginRender();

        std::string title = m_doc->GetSongName() ? m_doc->GetSongName() : "";
        if (title.empty()) {
            title = utils::path_basename(fileName);
        }
        std::string artist = m_doc->GetSongArtist() ? m_doc->GetSongArtist() : "";

        setMeta("title", title,
                "composer", artist,
                "songs", m_trackCount,
                "startSong", 0,
                "format", "FamiTracker");
    }

    ~FamiTrackerPlayer() override
    {
        delete m_sg;   // references the document; destroy first
        delete m_doc;
    }

    int getHZ() override { return kRate; }

    int getSamples(int16_t* target, int size) override
    {
        if (m_sg->isHalted()) {
            return -1;
        }
        const int frames = size / 2; // host buffer is interleaved stereo
        if (static_cast<int>(m_mono.size()) < frames) {
            m_mono.resize(frames);
        }
        core::u32 got = m_sg->renderSamples(m_mono.data(), frames);
        for (core::u32 i = 0; i < got; i++) {
            target[i * 2] = m_mono[i];
            target[i * 2 + 1] = m_mono[i];
        }
        if (got == 0 && m_sg->isHalted()) {
            return -1;
        }
        return static_cast<int>(got) * 2;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song < 0 || song >= m_trackCount) {
            return false;
        }
        // Re-arm the engine on the requested subsong. setDocument() re-runs the
        // per-track channel/controller setup for the newly selected track.
        m_doc->SelectTrack(static_cast<unsigned int>(song));
        m_sg->setDocument(m_doc);
        m_sg->trackerController()->startAt(0, 0);
        m_sg->beginRender();
        return true;
    }

private:
    PullSink m_sink;
    FtmDocument* m_doc = nullptr;
    SoundGen* m_sg = nullptr;
    int m_trackCount = 1;
    std::vector<int16_t> m_mono;
};

static const std::set<std::string> supported_ext{"ftm"};

bool FamiTrackerPlugin::canHandle(const std::string& name)
{
    auto lower = utils::toLower(name);
    bool extOk = false;
    for (auto const& e : supported_ext) {
        if (lower.size() > e.size() + 1 &&
            lower.compare(lower.size() - e.size() - 1, e.size() + 1,
                          "." + e) == 0) {
            extOk = true;
            break;
        }
    }
    if (!extOk) {
        return false;
    }
    // Content-gate: only the FamiTracker magic, so Face The Music .ftm files
    // (magic "FTMN") fall through to OpenMPT.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t hdr[18];
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    return looksLikeFtm(hdr, n);
}

std::set<std::string> FamiTrackerPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* FamiTrackerPlugin::fromFile(const std::string& fileName)
{
    return new FamiTrackerPlayer{fileName};
}

} // namespace musix

extern "C" void famitrackerplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::FamiTrackerPlugin>();
    });
}
