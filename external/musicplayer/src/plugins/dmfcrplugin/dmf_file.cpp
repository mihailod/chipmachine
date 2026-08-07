// Clean-room DefleMask .dmf parser -- see dmf_file.h and README.md.
//
// Version gates:
//
//   ver <= 0x16   TOTAL_ROWS_PER_PATTERN is 1 byte   (4 bytes from 0x17 up)
//   ver <= 0x13   an "Arpeggio Tick Speed" byte follows TOTAL_ROWS_IN_...
//   ver <= 0x12   FM operators use the old OPL-flavoured record, and the FM
//                 header carries RESERVED / TOTAL_OPERATORS bytes
//   ver >= 0x12   Game Boy STD instruments skip the volume macro and carry 4
//                 extra envelope bytes
//   ver >= 0x16   samples carry a "Sample Bits" byte
//   ver >= 0x17   samples carry a name
//
// TWO of these contradict the published specs, and the corpus wins:
//
//   * Arpeggio Tick Speed. The specs say it was "REMOVED since ver 11.1", and
//     DMF_SPECS_0x15 is the 11.1 document -- so the byte should survive through
//     0x14. It does not: all 37 v0x14 Genesis files parse exactly WITHOUT it
//     and none parse with it. The removal landed at 0x14.
//
//   * Macro LOOP_POSITION. DMF_SPECS_0x11 lists it unconditionally, with the
//     "IF ENVELOPE_SIZE > 0" guard appearing only from 0x12. In the corpus the
//     guard is already in force at 0x11: all 94 v0x11 Genesis files parse
//     exactly with the conditional read and none with the unconditional one.
//     So the loop byte is conditional at every version this parser accepts.
//
// Both were established the same way, and so were the two undocumented
// versions (0x14, 0x17): DMF has no length fields or section markers, so a
// mis-sized gate silently shifts every later field, and the one property that
// catches it is that a correct parse lands exactly on the end of the buffer.
// Every combination of the six gates was tried against all 833 Genesis files
// and scored on exact consumption; the assignment above is the unique winner.
// See tools/dmfcheck and README.md ("Resolving the undocumented versions").
//
// One further undocumented quirk, also measured: most files carry a single
// unexplained trailing byte after the PCM sample block (606 of the 643
// well-formed Genesis files; the other 37 end exactly). It is not a sentinel --
// its value varies -- and nothing references it, so the strict check tolerates
// 0 or 1 leftover bytes and playback ignores it.

#include "dmf_file.h"

#include <cstring>
#include <zlib.h>

namespace dmfcr {

namespace {

class Reader
{
public:
    Reader(const uint8_t* d, size_t n) : d_(d), n_(n) {}

    bool ok() const { return ok_; }
    size_t pos() const { return p_; }
    size_t size() const { return n_; }
    bool atEnd() const { return p_ >= n_; }

    uint8_t u8()
    {
        if (p_ + 1 > n_) { return fail(); }
        return d_[p_++];
    }

    uint16_t u16()
    {
        if (p_ + 2 > n_) { return fail(); }
        uint16_t v = static_cast<uint16_t>(d_[p_] | (d_[p_ + 1] << 8));
        p_ += 2;
        return v;
    }

    int16_t s16() { return static_cast<int16_t>(u16()); }

    uint32_t u32()
    {
        if (p_ + 4 > n_) { return fail(); }
        uint32_t v = static_cast<uint32_t>(d_[p_]) |
                     (static_cast<uint32_t>(d_[p_ + 1]) << 8) |
                     (static_cast<uint32_t>(d_[p_ + 2]) << 16) |
                     (static_cast<uint32_t>(d_[p_ + 3]) << 24);
        p_ += 4;
        return v;
    }

    int32_t s32() { return static_cast<int32_t>(u32()); }

    // 1-byte length followed by that many chars.
    std::string pstr()
    {
        uint8_t len = u8();
        if (!ok_ || p_ + len > n_) { fail(); return {}; }
        std::string s(reinterpret_cast<const char*>(d_ + p_), len);
        p_ += len;
        return s;
    }

    void skip(size_t n)
    {
        if (p_ + n > n_) { fail(); return; }
        p_ += n;
    }

private:
    uint8_t fail()
    {
        ok_ = false;
        p_ = n_;
        return 0;
    }

    const uint8_t* d_;
    size_t n_;
    size_t p_ = 0;
    bool ok_ = true;
};

const char kMagic[] = ".DelekDefleMask.";
constexpr size_t kMagicLen = 16;

// Read one macro. LOOP_POSITION is present only when ENVELOPE_SIZE > 0 -- at
// every version this parser accepts, including 0x11 where the spec claims
// otherwise (see the version-gate notes at the top of this file).
bool readMacro(Reader& r, Macro& m)
{
    uint8_t size = r.u8();
    if (size > 127) { return false; } // spec says 0-127; a larger value is a desync
    m.values.resize(size);
    for (uint8_t i = 0; i < size; i++) {
        m.values[i] = r.s32();
    }
    if (size > 0) {
        int8_t loop = static_cast<int8_t>(r.u8());
        m.loopPos = (loop < 0 || loop >= static_cast<int>(size)) ? -1 : loop;
    } else {
        m.loopPos = -1;
    }
    return true;
}

} // namespace

int systemChannels(uint8_t sys)
{
    switch (sys) {
    case SYS_YMU759: return 17;
    case SYS_GENESIS: return 10;
    case SYS_GENESIS_EXT:
    case SYS_GENESIS_EXT_OLD: return 13;
    case SYS_SMS: return 4;
    case SYS_GAMEBOY: return 4;
    case SYS_PCENGINE: return 6;
    case SYS_NES: return 5;
    case SYS_C64_8580:
    case SYS_C64_6581: return 3;
    case SYS_ARCADE: return 13;
    case SYS_NEOGEO: return 13;
    case SYS_NEOGEO_EXT: return 16;
    default: return 0;
    }
}

double Module::baseHz() const
{
    if (usingCustomHz != 0) {
        // Three ASCII-ish digit bytes; the spec calls them "Custom HZ value
        // 1/2/3". DefleMask stores them as the decimal digits of the rate.
        int v = (customHz[0] - '0') * 100 + (customHz[1] - '0') * 10 +
                (customHz[2] - '0');
        if (v >= 1 && v <= 999) { return v; }
    }
    return framesMode != 0 ? 60.0 : 50.0;
}

bool inflateDmf(const uint8_t* data, size_t len, std::vector<uint8_t>& out,
                std::string& err)
{
    if (len < 2) {
        err = "file too short";
        return false;
    }
    if (data[0] != 0x78) {
        err = "not a zlib stream (DefleMask .dmf are zlib-wrapped)";
        return false;
    }

    // Plain zlib inflate, Adler-32 enforced. See the header for why the
    // checksum is NOT worked around: the files that fail it are really damaged.
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) {
        err = "inflateInit failed";
        return false;
    }
    zs.next_in = const_cast<Bytef*>(data);
    zs.avail_in = static_cast<uInt>(len);

    std::vector<uint8_t> buf;
    buf.reserve(len * 4);
    uint8_t chunk[65536];
    int rc = Z_OK;
    while (rc == Z_OK) {
        zs.next_out = chunk;
        zs.avail_out = sizeof(chunk);
        rc = inflate(&zs, Z_NO_FLUSH);
        size_t produced = sizeof(chunk) - zs.avail_out;
        if (produced > 0) { buf.insert(buf.end(), chunk, chunk + produced); }
        if (rc == Z_BUF_ERROR && produced == 0) { break; }
    }
    inflateEnd(&zs);

    if (rc != Z_STREAM_END) {
        err = "corrupt DefleMask file (damaged data, bad zlib checksum)";
        return false;
    }
    out.swap(buf);
    return true;
}

bool parseDmf(const uint8_t* data, size_t len, Module& m, std::string& err,
              bool strict)
{
    Reader r(data, len);

    if (len < kMagicLen + 2 || std::memcmp(data, kMagic, kMagicLen) != 0) {
        err = "missing .DelekDefleMask. magic";
        return false;
    }
    r.skip(kMagicLen);

    m.version = r.u8();
    m.system = r.u8();

    if (m.version < kMinVersion || m.version > kMaxVersion) {
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "unsupported DMF version 0x%02X (this parser covers 0x%02X-0x%02X)",
                 m.version, kMinVersion, kMaxVersion);
        err = buf;
        return false;
    }

    m.totalChannels = systemChannels(m.system);
    if (m.totalChannels <= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "unknown system byte 0x%02X", m.system);
        err = buf;
        return false;
    }

    const uint8_t v = m.version;

    m.songName = r.pstr();
    m.songAuthor = r.pstr();
    m.highlightA = r.u8();
    m.highlightB = r.u8();

    m.timeBase = r.u8();
    m.tickTime1 = r.u8();
    m.tickTime2 = r.u8();
    m.framesMode = r.u8();
    m.usingCustomHz = r.u8();
    m.customHz[0] = r.u8();
    m.customHz[1] = r.u8();
    m.customHz[2] = r.u8();

    m.rowsPerPattern = (v >= 0x17) ? r.u32() : r.u8();
    m.matrixRows = r.u8();
    if (v <= 0x13) { m.arpTickSpeed = r.u8(); }

    if (!r.ok()) {
        err = "truncated in module header";
        return false;
    }
    if (m.rowsPerPattern == 0 || m.rowsPerPattern > 1024) {
        err = "implausible rows-per-pattern";
        return false;
    }

    // Pattern matrix: [channel][order row]
    m.matrix.assign(m.totalChannels, std::vector<uint8_t>(m.matrixRows, 0));
    for (int c = 0; c < m.totalChannels; c++) {
        for (int o = 0; o < m.matrixRows; o++) {
            m.matrix[c][o] = r.u8();
        }
    }
    if (!r.ok()) {
        err = "truncated in pattern matrix";
        return false;
    }

    // Instruments
    const bool gameboy = (m.system == SYS_GAMEBOY);
    uint8_t nins = r.u8();
    m.instruments.resize(nins);
    for (uint8_t i = 0; i < nins && r.ok(); i++) {
        Instrument& ins = m.instruments[i];
        ins.name = r.pstr();
        ins.fm = (r.u8() == 1);

        if (ins.fm) {
            if (v <= 0x12) {
                // Old layout: ALG, RESERVED, FB, RESERVED, LFO, RESERVED,
                // TOTAL_OPERATORS, LFO2 -- exactly as DMF_SPECS_0x11/0x12 list
                // it. TOTAL_OPERATORS (0 = 2 ops, 1 = 4 ops) is read and
                // discarded: on a YM2612 every channel always has 4 operators,
                // and DefleMask's own 2-op mode is expressed through the
                // algorithm, not through the operator count.
                ins.alg = r.u8();
                r.u8();
                ins.fb = r.u8();
                r.u8();
                ins.lfo = r.u8();
                r.u8();
                r.u8(); // TOTAL_OPERATORS
                ins.lfo2 = r.u8();
                for (auto& op : ins.ops) {
                    op.am = r.u8();
                    op.ar = r.u8();
                    r.u8();          // DAM
                    op.dr = r.u8();
                    r.u8();          // DVB
                    r.u8();          // EGT
                    r.u8();          // KSL
                    op.mult = r.u8();
                    op.rr = r.u8();
                    op.sl = r.u8();
                    r.u8();          // SUS
                    op.tl = r.u8();
                    r.u8();          // VIB
                    r.u8();          // WS
                    op.rs = r.u8();  // "KSR (RS on SEGA Genesis)"
                    op.dt = r.u8();
                    op.d2r = r.u8();
                    op.ssgMode = r.u8();
                }
            } else {
                ins.alg = r.u8();
                ins.fb = r.u8();
                ins.lfo = r.u8();
                ins.lfo2 = r.u8();
                for (auto& op : ins.ops) {
                    op.am = r.u8();
                    op.ar = r.u8();
                    op.dr = r.u8();
                    op.mult = r.u8();
                    op.rr = r.u8();
                    op.sl = r.u8();
                    op.tl = r.u8();
                    op.dt2 = r.u8();
                    op.rs = r.u8();
                    op.dt = r.u8();
                    op.d2r = r.u8();
                    op.ssgMode = r.u8();
                }
            }
        } else {
            bool macrosOk = true;
            if (!(gameboy && v >= 0x12)) {
                macrosOk = readMacro(r, ins.volume);
            }
            macrosOk = macrosOk && readMacro(r, ins.arpeggio);
            ins.arpeggio.mode = r.u8();
            macrosOk = macrosOk && readMacro(r, ins.duty);
            macrosOk = macrosOk && readMacro(r, ins.wavetable);
            if (!macrosOk) {
                err = "implausible macro length (parse desynchronised)";
                return false;
            }

            if (m.system == SYS_C64_8580 || m.system == SYS_C64_6581) {
                r.skip(19); // 14 instrument bytes + 5 filter globals
            } else if (gameboy && v >= 0x12) {
                r.skip(4); // envelope volume/direction/length, sound length
            }
        }
    }
    if (!r.ok()) {
        err = "truncated in instruments";
        return false;
    }

    // Wavetables
    uint8_t nwav = r.u8();
    m.wavetables.resize(nwav);
    for (uint8_t i = 0; i < nwav && r.ok(); i++) {
        uint32_t sz = r.u32();
        if (sz > 4096) {
            err = "implausible wavetable size";
            return false;
        }
        m.wavetables[i].resize(sz);
        for (uint32_t j = 0; j < sz; j++) {
            m.wavetables[i][j] = r.s32();
        }
    }
    if (!r.ok()) {
        err = "truncated in wavetables";
        return false;
    }

    // Patterns
    m.channels.resize(m.totalChannels);
    for (int c = 0; c < m.totalChannels && r.ok(); c++) {
        Channel& ch = m.channels[c];
        ch.effectColumns = r.u8();
        if (ch.effectColumns < 0 || ch.effectColumns > 4) {
            err = "implausible effect column count";
            return false;
        }
        ch.patterns.resize(m.matrixRows);
        for (int p = 0; p < m.matrixRows && r.ok(); p++) {
            Pattern& pat = ch.patterns[p];
            pat.rows.resize(m.rowsPerPattern);
            for (uint32_t rw = 0; rw < m.rowsPerPattern; rw++) {
                Row& row = pat.rows[rw];
                row.note = r.u16();
                row.octave = r.u16();
                row.volume = r.s16();
                for (int e = 0; e < ch.effectColumns; e++) {
                    row.effects[e].code = r.s16();
                    row.effects[e].value = r.s16();
                }
                row.instrument = r.s16();
            }
        }
    }
    if (!r.ok()) {
        err = "truncated in pattern data";
        return false;
    }

    // PCM samples
    uint8_t nsmp = r.u8();
    m.samples.resize(nsmp);
    for (uint8_t i = 0; i < nsmp && r.ok(); i++) {
        Sample& s = m.samples[i];
        uint32_t sz = r.u32();
        if (sz > (1u << 24)) {
            err = "implausible sample size";
            return false;
        }
        if (v >= 0x17) { s.name = r.pstr(); }
        s.rate = r.u8();
        s.pitch = r.u8();
        s.amp = r.u8();
        if (v >= 0x16) { s.bits = r.u8(); }
        s.data.resize(sz);
        for (uint32_t j = 0; j < sz; j++) {
            s.data[j] = r.s16();
        }
    }
    if (!r.ok()) {
        err = "truncated in samples";
        return false;
    }

    // Tolerate the single undocumented trailing byte most files carry (see the
    // note at the top). Anything more means the parse desynchronised.
    if (strict && r.size() - r.pos() > 1) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%zu trailing bytes (parse desynchronised)",
                 r.size() - r.pos());
        err = buf;
        return false;
    }

    return true;
}

bool loadDmf(const uint8_t* raw, size_t len, Module& m, std::string& err,
             bool strict)
{
    std::vector<uint8_t> inflated;
    if (!inflateDmf(raw, len, inflated, err)) { return false; }
    return parseDmf(inflated.data(), inflated.size(), m, err, strict);
}

} // namespace dmfcr
