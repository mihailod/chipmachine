#include "MaxTraxPlugin.h"
#include "../../chipplayer.h"

#include "maxtrax/compat.h"
#include "maxtrax/maxtrax.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace musix {

// MaxTrax modules begin with the ASCII magic "MXTX", a big-endian uint16 tempo
// and uint16 flags. On modland three layouts occur:
//   1. Self-contained: scores + sampled instruments in one file (most files).
//   2. Suffix split: a score "<name>scr.mxtx" + an instrument "<name>inst.mxtx"
//      (Frank Klepacki's Kyrandia).
//   3. Shared-bank split: many tiny score-only parts plus one big instrument
//      bank, with NO naming marker -- e.g. Russell Lieblich's "a-train (...)":
//      every part is score-only and "a-train (intro).mxtx" holds the samples.
// We resolve all three by CONTENT: a file is probed for (#scores, #samples).
// A score-only file (samples==0) borrows the instrument bank of a sibling
// .mxtx in the same directory that has samples; an instrument-only file
// borrows the scores of a sibling that has scores. ScummVM's
// load(stream, loadScores, loadSamples) then loads each half from its file.
namespace {

constexpr uint8_t MXTX_MAGIC[4] = {'M', 'X', 'T', 'X'};

bool hasMagic(const uint8_t* data, size_t len)
{
    return len >= sizeof(MXTX_MAGIC) &&
           memcmp(data, MXTX_MAGIC, sizeof(MXTX_MAGIC)) == 0;
}

bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

struct Counts
{
    int scores = -1; // -1 => not a MaxTrax module
    int samples = 0;
};

// Cheaply count scores and sampled instruments without allocating: walk the
// header, skipping over the score events and sample payloads. Mirrors the
// structure parsed by MaxTrax::load(). Tolerant of a truncated final sample.
Counts probe(const std::vector<uint8_t>& data)
{
    Counts c;
    if (!hasMagic(data.data(), data.size())) {
        return c;
    }
    Common::SeekableReadStream s(data.data(), static_cast<uint32_t>(data.size()));
    s.readUint32BE();              // magic
    s.readUint16BE();              // tempo
    const uint16_t flags = s.readUint16BE();
    if (flags & (1 << 15)) {
        s.skip(128 * 2);           // microtonal table
    }
    const uint16_t scoresInFile = s.readUint16BE();
    for (uint16_t i = 0; i < scoresInFile; ++i) {
        const uint32_t numEvents = s.readUint32BE();
        s.skip(numEvents * 6);
    }
    if (s.eos()) {
        return c;                  // malformed score section
    }
    c.scores = scoresInFile;
    c.samples = s.readUint16BE();  // number of sampled instruments
    if (s.eos()) {
        c.samples = 0;
    }
    return c;
}

std::vector<uint8_t> readAllOrEmpty(const std::string& path)
{
    if (path.empty() || !utils::File::exists(path)) {
        return {};
    }
    return utils::File(path).readAll();
}

size_t commonPrefixLen(const std::string& a, const std::string& b)
{
    size_t n = 0;
    while (n < a.size() && n < b.size() && a[n] == b[n]) {
        ++n;
    }
    return n;
}

// Scan 'dir' for the sibling .mxtx (other than 'selfPath') that best completes
// 'selfPath'. 'wantSamples' picks the kind of sibling sought: true => an
// instrument bank (samples>0), false => a score part (scores>0). The match is
// ranked first by the longest shared filename prefix -- so a score only ever
// borrows from its own set, never an unrelated set that happens to share the
// directory -- then by sample count (the shared bank is the richest) for banks,
// or by having no samples of its own (a pure part) for scores.
std::string scanSiblings(const std::string& dir, const std::string& selfPath,
                         bool wantSamples)
{
    const std::string selfBase = utils::toLower(utils::path_filename(selfPath));
    std::string best;
    long bestRank = -1;
    int bestTie = -1;
    for (auto& f : utils::File(dir.empty() ? "." : dir).listFiles()) {
        const std::string& path = f.getName();
        if (path == selfPath || !endsWith(utils::toLower(path), ".mxtx")) {
            continue;
        }
        Counts c = probe(readAllOrEmpty(path));
        if (c.scores < 0) {
            continue;
        }
        if (wantSamples ? (c.samples <= 0) : (c.scores <= 0)) {
            continue;
        }
        long rank =
            static_cast<long>(commonPrefixLen(
                utils::toLower(utils::path_filename(path)), selfBase));
        int tie = wantSamples ? c.samples : (c.samples == 0 ? 1 : 0);
        if (rank > bestRank || (rank == bestRank && tie > bestTie)) {
            best = path;
            bestRank = rank;
            bestTie = tie;
        }
    }
    return best;
}

} // namespace

class MaxTraxPlayer : public ChipPlayer
{
public:
    static constexpr int kRate = 44100;

    // 'instData' empty => combined module (scores + samples both in scoreData).
    MaxTraxPlayer(const std::vector<uint8_t>& scoreData,
                  const std::vector<uint8_t>& instData,
                  const std::string& fileName)
        : player(kRate, /*stereo*/ true)
    {
        if (!hasMagic(scoreData.data(), scoreData.size())) {
            throw player_exception("Not a MaxTrax module");
        }

        if (instData.empty()) {
            // Self-contained module: scores and samples in one file.
            Common::SeekableReadStream s(scoreData.data(),
                                         static_cast<uint32_t>(scoreData.size()));
            if (!player.load(s, /*scores*/ true, /*samples*/ true)) {
                throw player_exception("Could not load MaxTrax: " + fileName);
            }
        } else {
            // Split set: scores from one file, sampled instruments from the other.
            Common::SeekableReadStream ss(scoreData.data(),
                                          static_cast<uint32_t>(scoreData.size()));
            Common::SeekableReadStream is(instData.data(),
                                          static_cast<uint32_t>(instData.size()));
            if (!player.load(ss, /*scores*/ true, /*samples*/ false) ||
                !player.load(is, /*scores*/ false, /*samples*/ true)) {
                throw player_exception("Could not load split MaxTrax: " + fileName);
            }
        }

        songCount = player.getScoreCount();
        if (songCount <= 0) {
            throw player_exception("MaxTrax module has no scores: " + fileName);
        }

        if (!player.playSong(0, /*loop*/ false)) {
            throw player_exception("Could not start MaxTrax: " + fileName);
        }

        setMeta("title", utils::path_basename(fileName), "songs", songCount,
                "startSong", 0, "format", "MaxTrax");
    }

    int getHZ() override { return kRate; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // Paula::readBuffer fills 'noSamples' interleaved stereo int16s and
        // always returns that count (it zero-fills when not playing).
        return player.readBuffer(target, noSamples);
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song >= 0 && song < songCount) {
            return player.playSong(song, /*loop*/ false);
        }
        return false;
    }

private:
    Audio::MaxTrax player;
    int songCount = 1;
};

static const std::set<std::string> supported_ext{"mxtx"};

bool MaxTraxPlugin::canHandle(const std::string& name)
{
    auto lowerName = utils::toLower(name);
    // Modland names these `<song>.mxtx`; tolerate a `mxtx.<song>` prefix too.
    bool nameMatches =
        supported_ext.count(utils::path_extension(lowerName)) > 0 ||
        supported_ext.count(utils::path_prefix(lowerName)) > 0;
    if (!nameMatches) {
        return false;
    }
    // Confirm via the content magic so we never grab an unrelated file. Every
    // half of a split set carries the MXTX magic, so all route here; fromFile()
    // then pairs scores with samples.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t magic[sizeof(MXTX_MAGIC)];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && hasMagic(magic, n);
}

std::set<std::string> MaxTraxPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

std::vector<std::string> MaxTraxPlugin::getSecondaryFiles(const std::string& name)
{
    // A self-contained module needs nothing. A split half (a score-only part or
    // an instrument-only bank) needs its counterpart, whose filename can't be
    // derived in the shared-bank case -- so ask the host to fetch the whole song
    // directory ("./") and let fromFile() pair scores with samples by content.
    // The host call is a no-op when the directory is already present locally
    // (e.g. a local modland mirror), where the siblings are read in place.
    Counts c = probe(readAllOrEmpty(name));
    bool isMaxTrax = (c.scores >= 0);
    bool selfContained = (c.scores > 0 && c.samples > 0);
    if (isMaxTrax && !selfContained) {
        return {"./"};
    }
    return {};
}

ChipPlayer* MaxTraxPlugin::fromFile(const std::string& fileName)
{
    std::vector<uint8_t> primary = utils::File(fileName).readAll();
    Counts c = probe(primary);
    std::string dir = utils::path_directory(fileName);

    if (c.scores > 0 && c.samples > 0) {
        // Self-contained module.
        return new MaxTraxPlayer{primary, {}, fileName};
    }

    if (c.scores > 0 && c.samples == 0) {
        // Score-only part: borrow the instrument bank from a sibling of the same
        // set that has samples.
        std::string bank = scanSiblings(dir, fileName, /*wantSamples*/ true);
        return new MaxTraxPlayer{primary, readAllOrEmpty(bank), fileName};
    }

    if (c.scores == 0 && c.samples > 0) {
        // Instrument-only bank: the song is a sibling score part of the same set.
        std::string score = scanSiblings(dir, fileName, /*wantSamples*/ false);
        std::vector<uint8_t> scoreData = readAllOrEmpty(score);
        if (!scoreData.empty()) {
            return new MaxTraxPlayer{scoreData, primary, fileName};
        }
    }

    // Unresolved (e.g. a bank with no score sibling): let the player report it.
    return new MaxTraxPlayer{primary, {}, fileName};
}

} // namespace musix

extern "C" void maxtraxplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MaxTraxPlugin>();
    });
}
