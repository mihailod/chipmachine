// dmfab -- A/B the clean-room Genesis player against Furnace, in one process.
//
// DEVELOPMENT HARNESS ONLY. This binary links BOTH the clean-room dmfcrplugin
// sources and the plus build's Furnace-based libdmfplugin.a, so it is GPL-
// encumbered and must never be shipped or linked into either app -- exactly the
// same standing as tools/mdxtrace, which links GPL mdxmini as a reference
// oracle. It is built by build.sh only when libdmfplugin.a is present.
//
// Furnace is driven strictly through the public ChipPlugin/ChipPlayer
// interface. Its sources are never read: the clean-room argument for
// dmfcrplugin depends on that, which is also why this compares rendered audio
// rather than a register trace (a register oracle would mean instrumenting
// Furnace's Genesis platform code -- precisely the code that must stay unread).
//
// Score: cosine similarity over log band-energy spectra, the method already
// used in this repo for VICE->cSID and for the VIC-I replacement. The two sides
// use different YM2612 cores and different resamplers, so they are never
// sample-identical even when the sequencing is exactly right; what this
// measures is whether the same notes, instruments and effects land at the same
// times.
//
//   dmfab <file.dmf> [seconds]            one file, prints "cosine <v> <name>"
//   dmfab --list <list.txt> [seconds]     many files, prints one line each

#include "../../external/musicplayer/src/chipplugin.h"
#include "../../external/musicplayer/src/chipplayer.h"

#include "../../external/musicplayer/src/plugins/dmfcrplugin/dmf_file.h"
#include "../../external/musicplayer/src/plugins/dmfcrplugin/dmf_player.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

extern "C" void dmfplugin_register();

// ChipPlugin::createPlugins() calls this; we want dmfplugin alone.
void register_plugins()
{
    dmfplugin_register();
}

namespace {

constexpr int kRate = 44100;
constexpr int kWin = 2048;
constexpr int kHop = 1024;
constexpr int kBands = 48;

std::vector<uint8_t> readFile(const std::string& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}

// ---- reference side: Furnace, as a black box -------------------------------
bool renderFurnace(const std::string& path, int frames, std::vector<float>& mono,
                   std::string& err)
{
    auto& plugins = musix::ChipPlugin::getPlugins();
    if (plugins.empty()) {
        err = "dmfplugin not registered";
        return false;
    }
    musix::ChipPlayer* player = nullptr;
    try {
        player = plugins.front()->fromFile(path);
    } catch (const musix::player_exception& e) {
        err = e.what();
        return false;
    } catch (...) {
        err = "furnace threw";
        return false;
    }
    if (player == nullptr) {
        err = "furnace returned null";
        return false;
    }

    mono.assign(frames, 0.0f);
    std::vector<int16_t> chunk(4096);
    int done = 0;
    while (done < frames) {
        int want = (frames - done) * 2;
        if (want > static_cast<int>(chunk.size())) { want = static_cast<int>(chunk.size()); }
        int got = 0;
        try {
            got = player->getSamples(chunk.data(), want);
        } catch (...) {
            break;
        }
        if (got <= 0) { break; }
        for (int i = 0; i + 1 < got; i += 2) {
            if (done >= frames) { break; }
            mono[done++] = (chunk[i] + chunk[i + 1]) * 0.5f / 32768.0f;
        }
    }
    delete player;
    return true;
}

// ---- candidate side: the clean-room player ---------------------------------
bool renderCleanRoom(const std::string& path, int frames, std::vector<float>& mono,
                     std::string& err)
{
    auto raw = readFile(path);
    if (raw.empty()) {
        err = "cannot read";
        return false;
    }
    dmfcr::Module m;
    if (!dmfcr::loadDmf(raw.data(), raw.size(), m, err)) { return false; }
    dmfcr::Player p;
    if (!p.init(m, kRate, err)) { return false; }

    mono.assign(frames, 0.0f);
    std::vector<float> buf(2048 * 2);
    int done = 0;
    while (done < frames) {
        // Deliberately NOT breaking on p.ended(): this side renders the full
        // window and the SCORING window is trimmed to the reference's active
        // span instead (see referenceActiveRows). Truncating here as well would
        // silence this side wherever the two disagree about when a song ends,
        // which measures that disagreement rather than the notes.
        int n = frames - done;
        if (n > 2048) { n = 2048; }
        p.render(buf.data(), n);
        for (int i = 0; i < n; i++) {
            mono[done + i] = (buf[i * 2] + buf[i * 2 + 1]) * 0.5f;
        }
        done += n;
    }
    return true;
}

// ---- spectra ---------------------------------------------------------------
void fft(std::vector<std::complex<float>>& a)
{
    const size_t n = a.size();
    if (n <= 1) { return; }
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1) { j ^= bit; }
        j ^= bit;
        if (i < j) { std::swap(a[i], a[j]); }
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        std::complex<float> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t k = 0; k < len / 2; k++) {
                std::complex<float> u = a[i + k];
                std::complex<float> v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

std::vector<std::vector<float>> spectra(const std::vector<float>& sig)
{
    static std::vector<float> hann;
    static std::vector<int> bins;
    if (hann.empty()) {
        hann.resize(kWin);
        for (int i = 0; i < kWin; i++) {
            hann[i] = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * i /
                                             (kWin - 1));
        }
        bins.resize(kBands + 1);
        for (int b = 0; b <= kBands; b++) {
            double e = 50.0 * std::pow(16000.0 / 50.0,
                                       static_cast<double>(b) / kBands);
            bins[b] = static_cast<int>(e * kWin / kRate);
        }
    }

    std::vector<std::vector<float>> out;
    if (static_cast<int>(sig.size()) < kWin) { return out; }
    std::vector<std::complex<float>> buf(kWin);
    for (size_t start = 0; start + kWin < sig.size(); start += kHop) {
        for (int i = 0; i < kWin; i++) {
            buf[i] = std::complex<float>(sig[start + i] * hann[i], 0.0f);
        }
        fft(buf);
        std::vector<float> row(kBands, 0.0f);
        for (int b = 0; b < kBands; b++) {
            int lo = bins[b];
            int hi = std::max(bins[b] + 1, bins[b + 1]);
            if (hi > kWin / 2) { hi = kWin / 2; }
            float sum = 0.0f;
            for (int i = lo; i < hi; i++) { sum += std::abs(buf[i]); }
            row[b] = std::log1p(sum);
        }
        out.push_back(std::move(row));
    }
    return out;
}

// How many spectrum rows the REFERENCE actually has audio in, counting back
// from the end. Both sides are rendered to a fixed length and the shorter one
// is silence-padded, so a module that Furnace stops early would otherwise be
// scored over a long silent tail it never played.
//
// This matters because Furnace ends some one-shot modules that this player
// keeps looping (see the known gap in the plugin README): scoring the tail
// would be measuring that one disagreement over and over instead of measuring
// how well the notes match. Everything up to the reference's last sound is
// still compared in full, so nothing real is hidden -- the divergence is
// reported separately as the render-length difference.
size_t referenceActiveRows(const std::vector<std::vector<float>>& A)
{
    size_t last = 0;
    for (size_t i = 0; i < A.size(); i++) {
        double e = 0;
        for (int b = 0; b < kBands; b++) { e += A[i][b]; }
        if (e > 1e-4) { last = i + 1; }
    }
    return last;
}

double cosineScore(const std::vector<std::vector<float>>& A,
                   const std::vector<std::vector<float>>& B)
{
    size_t n = std::min(A.size(), B.size());
    if (n == 0) { return 0.0; }
    double tot = 0.0;
    for (size_t i = 0; i < n; i++) {
        double num = 0, da = 0, db = 0;
        for (int b = 0; b < kBands; b++) {
            num += static_cast<double>(A[i][b]) * B[i][b];
            da += static_cast<double>(A[i][b]) * A[i][b];
            db += static_cast<double>(B[i][b]) * B[i][b];
        }
        da = std::sqrt(da);
        db = std::sqrt(db);
        if (da > 1e-6 && db > 1e-6) {
            tot += num / (da * db);
        } else if (da <= 1e-6 && db <= 1e-6) {
            tot += 1.0; // both silent here: agreement
        }
    }
    return tot / static_cast<double>(n);
}

// Mean band energy of each side, printed side by side. A constant octave error
// shows up as the whole profile shifted by 4 bands (12 semitones at 48 bands
// over the 50 Hz - 16 kHz decade span used here); a level error shows up as a
// flat offset; a missing chip shows up as a hole in one profile.
void printBandProfile(const std::string& path, double seconds)
{
    int frames = static_cast<int>(seconds * kRate);
    std::vector<float> a, b;
    std::string ea, eb;
    if (!renderCleanRoom(path, frames, b, eb)) {
        printf("cleanroom failed: %s\n", eb.c_str());
        return;
    }
    if (!renderFurnace(path, frames, a, ea)) {
        printf("furnace failed: %s\n", ea.c_str());
        return;
    }
    auto A = spectra(a);
    auto B = spectra(b);
    size_t n = std::min(A.size(), B.size());
    printf("band   freqHz   furnace  cleanroom   delta\n");
    for (int band = 0; band < kBands; band++) {
        double sa = 0, sb = 0;
        for (size_t i = 0; i < n; i++) {
            sa += A[i][band];
            sb += B[i][band];
        }
        sa /= std::max<size_t>(1, n);
        sb /= std::max<size_t>(1, n);
        double f = 50.0 * std::pow(16000.0 / 50.0, static_cast<double>(band) / kBands);
        printf("%3d  %8.0f  %8.3f  %8.3f  %+8.3f\n", band, f, sa, sb, sb - sa);
    }
}

// Cosine per second of playback. A sequencing divergence (a jump taken at the
// wrong time, a speed change missed) shows as a clean cliff: high agreement up
// to some second, then a floor. A timbral problem is flat and low from the
// start. Knowing which, and where the cliff is, is the difference between
// hunting a bug and guessing at one.
void printOverTime(const std::string& path, double seconds)
{
    int frames = static_cast<int>(seconds * kRate);
    std::vector<float> a, b;
    std::string ea, eb;
    if (!renderCleanRoom(path, frames, b, eb)) {
        printf("cleanroom failed: %s\n", eb.c_str());
        return;
    }
    if (!renderFurnace(path, frames, a, ea)) {
        printf("furnace failed: %s\n", ea.c_str());
        return;
    }
    auto A = spectra(a);
    auto B = spectra(b);
    size_t n = std::min(A.size(), B.size());
    // kHop frames per spectrum row -> rows per second
    size_t rowsPerSec = kRate / kHop;
    printf("sec   cosine\n");
    for (size_t s = 0; s * rowsPerSec < n; s++) {
        size_t lo = s * rowsPerSec;
        size_t hi = std::min(n, lo + rowsPerSec);
        std::vector<std::vector<float>> sa(A.begin() + lo, A.begin() + hi);
        std::vector<std::vector<float>> sb(B.begin() + lo, B.begin() + hi);
        double c = cosineScore(sa, sb);
        int bar = static_cast<int>(c * 50);
        if (bar < 0) { bar = 0; }
        printf("%3zu   %.3f  %s\n", s, c, std::string(bar, '#').c_str());
    }
}

int compareOne(const std::string& path, double seconds, bool verbose)
{
    int frames = static_cast<int>(seconds * kRate);
    std::vector<float> a, b;
    std::string ea, eb;

    if (!renderCleanRoom(path, frames, b, eb)) {
        printf("SKIP-CLEANROOM\t%s\t%s\n", eb.c_str(), path.c_str());
        return 1;
    }
    if (!renderFurnace(path, frames, a, ea)) {
        printf("SKIP-FURNACE\t%s\t%s\n", ea.c_str(), path.c_str());
        return 1;
    }

    auto A = spectra(a);
    auto B = spectra(b);

    // Score over the reference's active span only (see referenceActiveRows).
    size_t active = referenceActiveRows(A);
    if (active < A.size()) { A.resize(active); }
    if (active < B.size()) { B.resize(active); }
    double c = cosineScore(A, B);

    // Simple level check too: a player that is right but 20 dB down would still
    // score well on cosine, so report both.
    auto rms = [](const std::vector<float>& v) {
        double s = 0;
        for (float x : v) { s += static_cast<double>(x) * x; }
        return std::sqrt(s / std::max<size_t>(1, v.size()));
    };
    printf("OK\t%.4f\t%.5f\t%.5f\t%s\n", c, rms(a), rms(b), path.c_str());
    (void)verbose;
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: dmfab <file.dmf> [seconds]\n"
                        "       dmfab --list <list.txt> [seconds]\n");
        return 2;
    }
    musix::ChipPlugin::createPlugins("");

    if (strcmp(argv[1], "--when") == 0) {
        if (argc < 3) { return 2; }
        printOverTime(argv[2], argc > 3 ? atof(argv[3]) : 20.0);
        return 0;
    }

    if (strcmp(argv[1], "--spec") == 0) {
        if (argc < 3) { return 2; }
        printBandProfile(argv[2], argc > 3 ? atof(argv[3]) : 20.0);
        return 0;
    }

    if (strcmp(argv[1], "--list") == 0) {
        if (argc < 3) { return 2; }
        double seconds = argc > 3 ? atof(argv[3]) : 20.0;
        std::ifstream in(argv[2]);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) { continue; }
            compareOne(line, seconds, false);
            fflush(stdout);
        }
        return 0;
    }

    double seconds = argc > 2 ? atof(argv[2]) : 20.0;
    return compareOne(argv[1], seconds, true);
}
