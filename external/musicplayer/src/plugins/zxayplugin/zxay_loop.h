#pragma once

// Finding where a ZX AY tune ends.
//
// Only one of these formats tells us: Bulba's PTxPlay sets bit 7 of its SETUP
// byte when it runs off the end of the position list. The rest of the players
// in this plugin just wrap around to the loop position and keep going forever,
// and neither the modules nor the disassemblies carry a duration.
//
// So we find the loop the same way a listener would -- by noticing that the
// music has started repeating itself. Every 1/50 s tick, the 14 AY registers
// are the complete audible state of the machine; when a tick's registers match
// some earlier tick's, that is a candidate period, and if the next several
// hundred ticks keep matching at that distance the tune really has looped.
//
// This is a property of the SOUND, not of any one format, so it works
// unchanged for the Z80 players, the native sequencers and the register dumps.
// It runs ONLINE, as playback proceeds -- deliberately, because confirming a
// loop up front would mean emulating minutes of Z80 before the first sample.

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace musix::zxay {

class LoopDetector
{
public:
    // A candidate must match for this many consecutive ticks to be believed.
    // Short repeats are everywhere in this music -- a held chord, a one-bar
    // drum pattern -- and confirming over ~6 seconds is what stops those being
    // mistaken for the end of the song.
    static constexpr int kConfirmTicks = 300;
    // 50 ticks/s, so this caps a tune at 15 minutes. Nothing in this corpus is
    // close to that; the cap exists so a tune that never repeats (or a
    // mis-detected file producing noise) still terminates.
    static constexpr int kMaxTicks = 50 * 60 * 15;
    // At most this many periods are tracked at once, smallest first. A dense
    // texture throws off plenty of coincidental matches and there is no point
    // carrying them all.
    static constexpr int kMaxCandidates = 12;

    // Feeds one tick's register state. Returns true once the loop is found.
    bool addTick(const uint8_t regs[14]);

    bool found() const { return loopPeriod_ > 0; }
    bool exhausted() const { return ticks_ >= kMaxTicks; }
    // Ticks of music before the repeat begins, i.e. the playable length.
    int lengthTicks() const
    {
        return loopPeriod_ > 0 ? loopStart_ + loopPeriod_ : ticks_;
    }

private:
    const uint8_t* snapshot(int tick) const { return &snapshots_[tick * 14]; }

    struct Candidate
    {
        int period;
        int confirmed;
        int start; // tick at which this candidate was first raised
    };

    std::vector<uint8_t> snapshots_; // 14 bytes per tick
    std::unordered_map<uint64_t, std::vector<int>> byHash_;
    std::vector<Candidate> candidates_;
    int ticks_ = 0;
    int loopPeriod_ = 0;
    int loopStart_ = 0;
};

} // namespace musix::zxay
