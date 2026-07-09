#include "SksPlugin.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>

#include <algorithm>
#include <mutex>
#include <vector>

// Vendored Arkos Tracker 3 (MIT), repo-root /arkostracker3. We reproduce the
// exact offline render chain of AT3's SongWavExporter / "SongToWav" CLI tool,
// but stream the PCM out of getSamples() instead of writing a WAV.
#include <juce_core/juce_core.h>
#include <import/loader/SongLoader.h>
#include <player/SongPlayer.h>
#include <audio/sources/PsgsProcessor.h>
#include <audio/sources/PsgStreamGenerator.h>
#include <controllers/model/OutputMix.h>
#include <song/Song.h>

namespace musix {

namespace {

// "STK1.0SONG", at file start or just after a 128-byte AMSDOS header.
const char STARKOS_MAGIC[] = "STK1.0SONG";
constexpr size_t STARKOS_MAGIC_LEN = sizeof(STARKOS_MAGIC) - 1;
constexpr size_t AMSDOS_HEADER_SIZE = 128;

bool hasStarkosMagic(const std::vector<uint8_t>& d)
{
    auto at = [&](size_t off) {
        return d.size() >= off + STARKOS_MAGIC_LEN &&
               memcmp(d.data() + off, STARKOS_MAGIC, STARKOS_MAGIC_LEN) == 0;
    };
    return at(0) || at(AMSDOS_HEADER_SIZE);
}

// .aks = an Arkos Tracker song (AT1/AT2/AT3 native). On disk it's an XML
// document, gzip-compressed by the editor (modland's are) but occasionally
// stored raw. AT3's SongLoader transparently gunzips and auto-detects the
// Arkos version, so accept either the gzip magic or a raw Arkos XML signature.
bool looksLikeArkosSong(const std::vector<uint8_t>& d)
{
    // The editor compresses the XML on save: AT1 uses gzip (1F 8B), AT3 a PK
    // zip (50 4B 03 04). AT3's ZipHelper transparently handles both.
    if (d.size() >= 2 && d[0] == 0x1F && d[1] == 0x8B) { return true; }
    if (d.size() >= 4 && d[0] == 0x50 && d[1] == 0x4B &&
        d[2] == 0x03 && d[3] == 0x04) { return true; }
    // Uncompressed XML: look for the Arkos root/version marker near the start.
    static const char marker[] = "ArkosTracker";
    const size_t markerLen = sizeof(marker) - 1;
    const size_t scan = std::min<size_t>(d.size(), 512);
    if (scan >= markerLen) {
        for (size_t i = 0; i + markerLen <= scan; ++i) {
            if (memcmp(d.data() + i, marker, markerLen) == 0) { return true; }
        }
    }
    return false;
}

} // namespace

using namespace arkostracker;

class SksPlayer : public ChipPlayer
{
public:
    explicit SksPlayer(const std::string& fileName)
    {
        // NB: deliberately no juce::ScopedJuceInitialiser_GUI here. The host
        // decodes on a detached worker thread (MusicPlayerList), and spinning
        // up JUCE's GUI/MessageManager off the main thread deadlocks/crashes a
        // non-JUCE host. The import + render path below only uses JUCE value
        // types (File, MemoryBlock, AudioSampleBuffer), which need no message
        // manager.
        SongLoader loader;
        auto result = loader.loadSong(juce::File(juce::String(fileName)));
        if (result == nullptr || result->status != SongLoader::ImportStatus::ok ||
            result->song == nullptr) {
            throw player_exception("STarKos: unsupported or corrupt song");
        }

        song = std::shared_ptr<Song>(std::move(result->song));

        const auto subsongIds = song->getSubsongIds();
        if (subsongIds.empty()) {
            throw player_exception("STarKos: song has no subsong");
        }
        subsongId = subsongIds.front();

        const auto metadata = song->getSubsongMetadata(subsongId);
        const auto psgs = song->getSubsongPsgs(subsongId);
        const auto replayFrequency = metadata.getReplayFrequencyHz();
        const auto sidPlayerCapability = metadata.getSidPlayerCapability();

        // Flat, full-stereo mix (AT3's default), no hardware speaker emulation.
        const OutputMix outputMix(100, 100, 100, 100, false, 100);

        songPlayer = std::make_unique<SongPlayer>(song);
        // One song-end (no extra loops) before the player mutes itself; once
        // the loop point has been reached once we let the track finish.
        songPlayer->setOfflineSongEndCountBeforeMuting(1);
        const auto startLocation = Location(subsongId, 0);
        const auto loopAndEnd = song->getLoopStartAndPastEndPositions(subsongId);
        songPlayer->play(startLocation, loopAndEnd.first, loopAndEnd.second, true, true);

        psgsProcessor.setOutputMix(outputMix);

        auto psgIndex = 0;
        for (const auto& psg : psgs) {
            auto gen = std::make_unique<PsgStreamGenerator>(
                *songPlayer, psg.getType(), psgIndex, replayFrequency,
                psg.getPsgFrequency(), psg.getSamplePlayerFrequency(),
                psg.getPsgMixingOutput(),
                static_cast<double>(outputMix.getChannelAVolume()) / 100.0,
                static_cast<double>(outputMix.getChannelBVolume()) / 100.0,
                static_cast<double>(outputMix.getChannelCVolume()) / 100.0,
                sidPlayerCapability);
            psgsProcessor.addInputSource(gen.get(), false);
            generators.push_back(std::move(gen));
            ++psgIndex;
        }

        floatBuffer.setSize(2, blockSize);
        psgsProcessor.prepareToPlay(blockSize, hz);

        std::string title = utils::rstrip(song->getTitle().toStdString());
        std::string author = utils::rstrip(song->getAuthor().toStdString());
        // The same AT3 engine renders both STarKos (.sks) and native Arkos
        // Tracker (.aks) songs; label by the source extension.
        const char* fmt =
            utils::toLower(utils::path_extension(fileName)) == "aks" ? "Arkos Tracker"
                                                                     : "STarKos";
        setMeta("title", title,
                "composer", author,
                "channels", static_cast<int>(song->getChannelCount(subsongId)),
                "format", fmt);
    }

    int getHZ() override { return static_cast<int>(hz); }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (ended) {
            return -1;
        }

        const int wantedFrames = noSamples / 2;
        int writtenFrames = 0;

        while (writtenFrames < wantedFrames) {
            const int chunk = std::min(blockSize, wantedFrames - writtenFrames);

            juce::AudioSourceChannelInfo info(&floatBuffer, 0, chunk);
            info.clearActiveBufferRegion();
            psgsProcessor.getNextAudioBlock(info);

            const float* left = floatBuffer.getReadPointer(0);
            const float* right = floatBuffer.getReadPointer(1);
            int16_t* out = target + writtenFrames * 2;
            for (int i = 0; i < chunk; ++i) {
                out[i * 2] = toInt16(left[i]);
                out[i * 2 + 1] = toInt16(right[i]);
            }
            writtenFrames += chunk;

            if (songPlayer->hasOfflineSongEndCountReached()) {
                ended = true;
                break;
            }
        }

        return writtenFrames * 2;
    }

private:
    static int16_t toInt16(float s)
    {
        if (s > 1.0f) s = 1.0f;
        else if (s < -1.0f) s = -1.0f;
        return static_cast<int16_t>(s * 32767.0f);
    }

    static constexpr double hz = 44100.0;
    static constexpr int blockSize = 1024;

    std::shared_ptr<Song> song;
    Id subsongId;
    std::unique_ptr<SongPlayer> songPlayer;
    PsgsProcessor psgsProcessor;
    std::vector<std::unique_ptr<PsgStreamGenerator>> generators;
    juce::AudioSampleBuffer floatBuffer;
    bool ended = false;
};

bool SksPlugin::canHandle(const std::string& name)
{
    const auto ext = utils::toLower(utils::path_extension(name));
    if (ext != "sks" && ext != "aks") {
        return false;
    }
    // canHandle is called by the host *outside* its try/catch, so it must never
    // throw -- swallow any read/IO error and simply decline the file.
    try {
        utils::File file{name};
        if (!file.exists()) {
            return false;
        }
        auto data = file.readAll();
        return ext == "aks" ? looksLikeArkosSong(data) : hasStarkosMagic(data);
    } catch (std::exception&) {
        return false;
    }
}

ChipPlayer* SksPlugin::fromFile(const std::string& name)
{
    return new SksPlayer(name);
}

} // namespace musix

extern "C" void sksplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](const std::string& /*config*/) {
        return std::make_shared<musix::SksPlugin>();
    });
}
