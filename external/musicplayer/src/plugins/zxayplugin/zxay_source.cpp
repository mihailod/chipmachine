#include "zxay_source.h"

#include "zx_ay_machine.h"
#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include "players/psc_bin.h"
#include "players/pt1_bin.h"
#include "players/ptxplay_bin.h"

#include <algorithm>
#include <cstring>

namespace musix::zxay {

namespace {

// Where a relocatable module is parked. Every Z80 player here loads at 0x8000
// or 0xC000 and the stack lives just under 0xFFFF, so the whole lower 48K is
// free; 0x0100 leaves the zero page clear of anything that might be read as a
// restart vector.
constexpr uint16_t kModuleBase = 0x0100;

std::string trimmed(const uint8_t* p, size_t n)
{
    std::string s;
    for (size_t i = 0; i < n; i++) {
        char c = static_cast<char>(p[i]);
        s.push_back(c >= 0x20 && c < 0x7F ? c : ' ');
    }
    while (!s.empty() && s.back() == ' ') {
        s.pop_back();
    }
    size_t start = s.find_first_not_of(' ');
    return start == std::string::npos ? std::string{} : s.substr(start);
}

// --- the tracker formats with an original Z80 player -------------------------

// How to drive one player. Bulba's players share a calling convention -- the
// first three words are `LD HL,<default module>` / `JP INIT` / `JP PLAY` -- so
// the descriptor is mostly just which bytes to load and where.
struct Z80Player
{
    const uint8_t* code;
    unsigned len;
    uint16_t org;
    // Offsets from `org`.
    uint16_t initOffset = 3; // entered with HL = module address
    uint16_t playOffset = 6;
    // PTxPlay's SETUP byte: bit 1 selects PT2 over PT3, bit 0 disables
    // looping, and bit 7 comes back set at end of song. 0 = no such byte.
    uint16_t setupOffset = 0;
    uint8_t setupValue = 0;
};

class Z80TrackerSource : public TickSource
{
public:
    Z80TrackerSource(std::vector<uint8_t> module, const Z80Player& player,
                     int sampleRate)
        : machine_(sampleRate), player_(player), module_(std::move(module))
    {
    }

    // Loads and inits. Returns false when the player refuses to come back,
    // which is how a file that passed detection but isn't really this format
    // usually presents.
    bool start()
    {
        machine_.poke(player_.org, player_.code, player_.len);
        machine_.poke(kModuleBase, module_.data(), module_.size());
        if (player_.setupOffset != 0) {
            machine_.pokeByte(player_.org + player_.setupOffset,
                              player_.setupValue);
        }
        return machine_.call(player_.org + player_.initOffset, kModuleBase);
    }

protected:
    bool advance() override
    {
        if (!machine_.call(player_.org + player_.playOffset)) {
            return false; // ran away -- stop rather than hang
        }
        if (player_.setupOffset != 0) {
            uint8_t setup = machine_.peek(player_.org + player_.setupOffset);
            if ((setup & 0x80) != 0) {
                return false; // the player says it reached the end
            }
        }
        if (loop_.addTick(machine_.registers()) || loop_.exhausted()) {
            return false;
        }
        emitTick(machine_);
        return true;
    }

private:
    ZxAyMachine machine_;
    Z80Player player_;
    std::vector<uint8_t> module_;
    LoopDetector loop_;
};

// PTxPlay's SETUP byte. Bit 0 disables looping, which is what makes the player
// set bit 7 at the end instead of wrapping round for ever; bit 1 picks PT2.
constexpr uint8_t kSetupNoLoop = 0x01;
constexpr uint8_t kSetupPt2 = 0x02;

Z80Player playerFor(Format f)
{
    switch (f) {
    case Format::pt3:
        return {ptxplay_bin, ptxplay_bin_len, ptxplay_bin_org, 3, 5,
                10,          kSetupNoLoop};
    case Format::pt2:
        return {ptxplay_bin, ptxplay_bin_len, ptxplay_bin_org, 3, 5,
                10,          static_cast<uint8_t>(kSetupNoLoop | kSetupPt2)};
    case Format::pt1:
        return {pt1_bin, pt1_bin_len, pt1_bin_org};
    case Format::psc:
        return {psc_bin, psc_bin_len, psc_bin_org};
    default:
        return {nullptr, 0, 0};
    }
}

// --- metadata ----------------------------------------------------------------
// Each format parks its title (and sometimes author) at a fixed offset; see
// players/source/ for the layouts.
void readMeta(Format f, const std::vector<uint8_t>& d, SongInfo& info)
{
    const uint8_t* p = d.data();
    const size_t n = d.size();
    auto at = [&](size_t off, size_t len) {
        return off + len <= n ? trimmed(p + off, len) : std::string{};
    };
    switch (f) {
    case Format::pt3:
        // The 0x63-byte identifier is "ProTracker 3.x compilation of
        // "<title>" by <author>", with both fields blank-padded in place.
        info.title = at(0x1E, 32);
        info.author = at(0x42, 32);
        break;
    case Format::pt2:
        info.title = at(101, 30);
        break;
    case Format::pt1:
        info.title = at(69, 30);
        break;
    case Format::stc:
        info.title = at(6, 21);
        break;
    case Format::psc:
        info.title = at(0, 69);
        break;
    default:
        break;
    }
}

} // namespace

std::unique_ptr<Source> createSource(std::vector<uint8_t> data,
                                     const std::string& ext, int sampleRate,
                                     Format* detected)
{
    Format f = detect(data.data(), data.size(), ext);
    if (detected != nullptr) {
        *detected = f;
    }
    if (f == Format::unknown) {
        return nullptr;
    }

    std::unique_ptr<Source> src;
    switch (f) {
    case Format::pt1:
    case Format::pt2:
    case Format::pt3:
    case Format::psc: {
        Z80Player pl = playerFor(f);
        if (pl.code == nullptr) {
            return nullptr;
        }
        auto z = std::make_unique<Z80TrackerSource>(data, pl, sampleRate);
        if (!z->start()) {
            return nullptr;
        }
        src = std::move(z);
        break;
    }
    case Format::stp:
    case Format::sqt:
    case Format::stc:
    case Format::asc:
    case Format::fxm:
    case Format::amad:
    case Format::vtx:
    case Format::psg:
        src = createNativeSource(f, data, sampleRate);
        break;
    case Format::vt2:
        // Vortex Tracker II's TEXT module is Arkos Tracker 3's -- sksplugin
        // drives that engine and claims ".vt2". Detecting it here is only so a
        // Vortex text file that landed under some other extension is DECLINED
        // rather than mistaken for the PT3 binary it names in its header.
    case Format::unknown:
        return nullptr;
    }

    if (src) {
        SongInfo info = src->info();
        readMeta(f, data, info);
        src->setInfo(info);
    }
    return src;
}

} // namespace musix::zxay
