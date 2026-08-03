#include "zxay_format.h"

#include <algorithm>
#include <cstring>
#include <set>

namespace musix::zxay {

namespace {

bool magic(const uint8_t* d, size_t len, size_t at, const char* s)
{
    size_t n = std::strlen(s);
    return len >= at + n && std::memcmp(d + at, s, n) == 0;
}

uint16_t word(const uint8_t* d, size_t at) { return d[at] | (d[at + 1] << 8); }

// Several of these formats are headed by a run of 16-bit offsets into the
// module. "Plausible" means non-zero and inside the file -- the single most
// discriminating test available when there is no magic to check.
bool offsetInFile(uint16_t off, size_t len)
{
    return off != 0 && off < len;
}

// Sound Tracker's compiled layout (players/source/ST11FMT.txt):
//   +0 delay, +1 positions offset, +3 ornaments offset, +5 patterns offset,
//   +27 compiled samples. All offsets are from the module start.
// The three ".stc"-family extensions (.stc, .zxs, .st13) are all this.
bool looksLikeStc(const uint8_t* d, size_t len)
{
    if (len < 32) {
        return false;
    }
    uint16_t pos = word(d, 1), orn = word(d, 3), pat = word(d, 5);
    if (!offsetInFile(pos, len) || !offsetInFile(orn, len) ||
        !offsetInFile(pat, len)) {
        return false;
    }
    // Delay is a frame count per row; 0 is nonsense and huge values are noise.
    if (d[0] == 0 || d[0] > 100) {
        return false;
    }
    // The 22 bytes from +6 are the identifier the tracker writes ("SONG BY ST
    // COMPILE", "SOUND TRACKER v1.3", ...). Requiring it to be text is what
    // separates a real STC from a file that merely has three small words up
    // front.
    int printable = 0;
    for (int i = 6; i < 27; i++) {
        if (d[i] >= 0x20 && d[i] < 0x7F) {
            printable++;
        }
    }
    return printable >= 16;
}

// Sound Tracker Pro (.stp, .stp2): delay then four offsets.
bool looksLikeStp(const uint8_t* d, size_t len)
{
    if (len < 16) {
        return false;
    }
    if (d[0] == 0 || d[0] > 100) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        if (!offsetInFile(word(d, 1 + i * 2), len)) {
            return false;
        }
    }
    // Offsets ascend in every Sound Tracker Pro module: positions, patterns,
    // ornaments, samples are laid down in that order.
    return word(d, 1) < word(d, 3) && word(d, 3) < word(d, 5) &&
           word(d, 5) < word(d, 7);
}

// Pro Tracker 2 (players/source/PT2.txt): delay, length, loop, then 32 sample
// and 16 ornament offsets, a pattern offset, and a 30-char ASCII title at +101.
bool looksLikePt2(const uint8_t* d, size_t len)
{
    if (len < 132) {
        return false;
    }
    if (d[0] == 0 || d[0] > 100 || d[1] == 0) {
        return false;
    }
    if (d[2] > d[1]) {
        return false; // loop position past the end of the position list
    }
    uint16_t pat = word(d, 99);
    if (!offsetInFile(pat, len)) {
        return false;
    }
    int printable = 0;
    for (int i = 101; i < 131; i++) {
        if (d[i] >= 0x20 && d[i] < 0x7F) {
            printable++;
        }
    }
    return printable >= 24;
}

// Pro Tracker 1.xx: same shape as PT2 but only 16 sample offsets, so the
// pattern offset is at +67 and the title at +69.
bool looksLikePt1(const uint8_t* d, size_t len)
{
    if (len < 100) {
        return false;
    }
    if (d[0] == 0 || d[0] > 100 || d[1] == 0) {
        return false;
    }
    if (d[2] > d[1]) {
        return false;
    }
    if (!offsetInFile(word(d, 67), len)) {
        return false;
    }
    int printable = 0;
    for (int i = 69; i < 99; i++) {
        if (d[i] >= 0x20 && d[i] < 0x7F) {
            printable++;
        }
    }
    return printable >= 24;
}

// ASC Sound Master. Two layouts:
//   ASC1: delay, loop, patterns, samples, ornaments, npos
//   ASC0: delay,       patterns, samples, ornaments, npos
// Neither has a magic number at offset 0; the tracker's "ASM COMPILATION OF"
// signature sits after the position list, not at a fixed place.
bool looksLikeAsc(const uint8_t* d, size_t len, bool& isAsc1)
{
    auto plausible = [&](size_t base) {
        uint16_t pat = word(d, base), smp = word(d, base + 2),
                 orn = word(d, base + 4);
        return offsetInFile(pat, len) && offsetInFile(smp, len) &&
               offsetInFile(orn, len) && pat < smp && smp < orn &&
               d[base + 6] != 0;
    };
    if (len < 32 || d[0] == 0 || d[0] > 100) {
        return false;
    }
    if (plausible(2)) {
        isAsc1 = true;
        return true;
    }
    if (plausible(1)) {
        isAsc1 = false;
        return true;
    }
    return false;
}

// SQ-Tracker: a 16-bit total size at +0 that matches the file, then six
// ABSOLUTE addresses (the format is not relocatable).
bool looksLikeSqt(const uint8_t* d, size_t len)
{
    if (len < 16) {
        return false;
    }
    uint16_t size = word(d, 0);
    if (size != len && size + 2 != len) {
        return false;
    }
    uint16_t base = word(d, 2) & 0xFF00;
    if (base < 0x4000) {
        return false;
    }
    for (int i = 1; i <= 5; i++) {
        uint16_t p = word(d, i * 2);
        if (p < base || p > base + len) {
            return false;
        }
    }
    return true;
}

} // namespace

const char* formatName(Format f)
{
    switch (f) {
    case Format::pt1: return "Pro Tracker 1 (Spectrum)";
    case Format::pt2: return "Pro Tracker 2 (Spectrum)";
    case Format::pt3: return "Pro Tracker 3 (Spectrum)";
    case Format::stc: return "Sound Tracker (Spectrum)";
    case Format::stp: return "Sound Tracker Pro (Spectrum)";
    case Format::asc: return "ASC Sound Master (Spectrum)";
    case Format::psc: return "Pro Sound Creator (Spectrum)";
    case Format::sqt: return "SQ-Tracker (Spectrum)";
    case Format::vtx: return "Vortex AY dump (Spectrum)";
    case Format::psg: return "PSG AY dump (Spectrum)";
    case Format::fxm: return "Fuxoft AY Language (Spectrum)";
    case Format::amad: return "AY Amadeus (Spectrum)";
    case Format::vt2: return "Vortex Tracker II (Spectrum)";
    case Format::unknown: break;
    }
    return "AY (Spectrum)";
}

bool isSupportedExtension(const std::string& ext)
{
    // ".vt2" is here for the editor's BINARY save, which is a PT3. Its TEXT
    // export shares the extension and is sksplugin's (Arkos Tracker 3); both
    // plugins claim ".vt2" and the content decides, so neither can steal the
    // other's files.
    static const std::set<std::string> exts = {
        "pt1", "pt2", "pt3", "stc", "st13", "zxs", "stp",  "stp2",
        "asc", "psc", "sqt", "vtx", "psg",  "fxm", "amad", "vt2"};
    return exts.count(ext) > 0;
}

Format detect(const uint8_t* data, size_t len, const std::string& ext)
{
    if (data == nullptr || len < 8) {
        return Format::unknown;
    }

    // --- unambiguous magics first ------------------------------------------
    // ".vt2" is Vortex Tracker II's BINARY save -- a PT3 module with the
    // editor's own 0x63-byte identifier in place of "ProTracker 3.x". Bulba's
    // PTxPlay plays it as a PT 3.6 module, so it is simply a PT3 here.
    if (magic(data, len, 0, "ProTracker 3.") ||
        magic(data, len, 0, "Vortex Tracker II")) {
        return Format::pt3;
    }
    // The editor's TEXT export is the other thing entirely: an ini-style
    // document that Arkos Tracker 3 reads and nothing here does.
    if (magic(data, len, 0, "[Module]")) {
        return Format::vt2;
    }
    if (magic(data, len, 0, "ZXAYAMAD")) {
        return Format::amad;
    }
    if (magic(data, len, 0, "FXSM")) {
        return Format::fxm;
    }
    if (magic(data, len, 0, "PSG\x1A")) {
        return Format::psg;
    }
    // VTX: "ay" or "ym" followed by the stereo-layout byte.
    if (magic(data, len, 0, "ay") || magic(data, len, 0, "ym")) {
        return Format::vtx;
    }
    if (magic(data, len, 0, "PSC ")) {
        return Format::psc;
    }

    // Picatune2 stores a 1-bit beeper project as XML under the ".pt2"
    // extension. It is not an AY file at all and nothing here can play it;
    // declining lets it Skip cleanly instead of failing with a wrong error.
    size_t i = (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
                   ? 3
                   : 0;
    if (i < len && data[i] == '<') {
        return Format::unknown;
    }

    // --- headerless formats -------------------------------------------------
    // Order matters: the more specific shapes are tested before the looser
    // ones, and the extension breaks genuine ties.
    if (looksLikeSqt(data, len)) {
        return Format::sqt;
    }
    bool asc1 = false;
    const bool asc = looksLikeAsc(data, len, asc1);
    const bool stp = looksLikeStp(data, len);
    const bool pt2 = looksLikePt2(data, len);
    const bool pt1 = looksLikePt1(data, len);

    // The Pro Trackers first: their tests are the most specific here, because
    // both demand a printable 30-character title at a fixed offset. They
    // differ from each other only in how many sample offsets precede that
    // title, so a short PT2 can satisfy the PT1 shape and vice versa -- trust
    // the title position the extension implies when both fire.
    if (pt1 && pt2) {
        return ext == "pt1" ? Format::pt1 : Format::pt2;
    }
    if (pt2) {
        return Format::pt2;
    }
    if (pt1) {
        return Format::pt1;
    }
    // Sound Tracker Pro before Sound Tracker, not after. Both start with a
    // delay byte followed by offsets, and STC's identifier-text test is
    // satisfied by an STP module whose title happens to begin within the first
    // 27 bytes -- which is exactly what testmus/zx/africa.stp2 does. The
    // reverse never happens: STP needs FOUR ascending in-range offsets, and an
    // STC's fourth word lands in sample data, far past the end of the file.
    if (stp) {
        return Format::stp;
    }
    // Sound Tracker before ASC, because ASC 0.x has literally the same shape --
    // a delay byte and three ascending in-range offsets -- and would swallow
    // every .stc. What separates them is that Sound Tracker writes 21 bytes of
    // identifier text ("SONG BY ST COMPILE", "SOUND TRACKER v1.3") at +6 and
    // ASC does not. ASC 1.x, which is nearly all of the .asc corpus, carries a
    // loop byte at +1 that pushes its offsets out of Sound Tracker's places
    // and so never reaches this test at all.
    if (looksLikeStc(data, len)) {
        return Format::stc;
    }
    if (asc) {
        return Format::asc;
    }

    // --- last resort: believe the extension ---------------------------------
    // Only for the extensions that are unique to one format anyway, so this
    // cannot mis-route a shared one.
    if (ext == "pt3") return Format::pt3;
    if (ext == "pt2") return Format::pt2;
    if (ext == "pt1") return Format::pt1;
    if (ext == "stc" || ext == "zxs" || ext == "st13") return Format::stc;
    if (ext == "stp" || ext == "stp2") return Format::stp;
    if (ext == "asc") return Format::asc;
    if (ext == "psc") return Format::psc;
    if (ext == "sqt") return Format::sqt;
    if (ext == "vt2") return Format::pt3;

    return Format::unknown;
}

} // namespace musix::zxay
