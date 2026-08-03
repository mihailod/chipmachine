#include "zxay_loop.h"

#include <algorithm>
#include <cstring>

namespace musix::zxay {

namespace {

// FNV-1a over the 14 audible AY registers.
uint64_t hashRegs(const uint8_t* r)
{
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < 14; i++) {
        h = (h ^ r[i]) * 1099511628211ULL;
    }
    return h;
}

// The shortest period worth considering. Below ~8 ticks (0.16 s) a "repeat" is
// just a steady note, and every sustained chord in the corpus would qualify.
constexpr int kMinPeriod = 8;

} // namespace

bool LoopDetector::addTick(const uint8_t regs[14])
{
    if (loopPeriod_ > 0) {
        return true;
    }
    snapshots_.insert(snapshots_.end(), regs, regs + 14);
    const int now = ticks_++;

    // Advance the candidates already in flight: each one predicts that this
    // tick repeats the tick `period` ago.
    for (auto it = candidates_.begin(); it != candidates_.end();) {
        const int back = now - it->period;
        if (back >= 0 &&
            std::memcmp(snapshot(now), snapshot(back), 14) == 0) {
            if (++it->confirmed >= kConfirmTicks) {
                loopPeriod_ = it->period;
                // The repeat has been running since the candidate was raised,
                // so the music proper ends one period before that.
                loopStart_ = std::max(0, it->start - it->period);
                return true;
            }
            ++it;
        } else {
            it = candidates_.erase(it);
        }
    }

    // Raise new candidates from earlier ticks with the same register state.
    auto& bucket = byHash_[hashRegs(regs)];
    if (static_cast<int>(candidates_.size()) < kMaxCandidates) {
        for (int earlier : bucket) {
            const int period = now - earlier;
            if (period < kMinPeriod) {
                continue;
            }
            const bool have =
                std::any_of(candidates_.begin(), candidates_.end(),
                            [&](const Candidate& c) { return c.period == period; });
            if (have) {
                continue;
            }
            candidates_.push_back({period, 1, now});
            if (static_cast<int>(candidates_.size()) >= kMaxCandidates) {
                break;
            }
        }
    }
    // Keep the bucket from growing without bound on a drone: only the most
    // recent few occurrences of a state can produce a period worth testing.
    bucket.push_back(now);
    if (bucket.size() > 32) {
        bucket.erase(bucket.begin());
    }
    return false;
}

} // namespace musix::zxay
