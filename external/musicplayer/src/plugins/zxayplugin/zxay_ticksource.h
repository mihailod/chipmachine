#pragma once

// Shared plumbing for anything that advances the AY in 1/50 s ticks.
//
// The chip is driven a whole tick at a time -- that is what a 50 Hz interrupt
// means -- but ChipPlayer asks for whatever buffer size the audio device wants,
// so the samples left over from a tick have to survive between render() calls.

#include "zx_ay_machine.h"
#include "zxay_source.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace musix::zxay {

class TickSource : public Source
{
public:
    int render(int16_t* out, int frames) final
    {
        int done = 0;
        while (done < frames) {
            if (cursor_ * 2 >= static_cast<int>(pending_.size())) {
                pending_.clear();
                cursor_ = 0;
                if (ended_ || !advance()) {
                    ended_ = true;
                    break;
                }
            }
            int avail = static_cast<int>(pending_.size()) / 2 - cursor_;
            int n = std::min(frames - done, avail);
            std::memcpy(out + done * 2, pending_.data() + cursor_ * 2,
                        static_cast<size_t>(n) * 2 * sizeof(int16_t));
            cursor_ += n;
            done += n;
        }
        return done;
    }

    bool ended() const final
    {
        return ended_ && cursor_ * 2 >= static_cast<int>(pending_.size());
    }

protected:
    // Advances one tick, appending its samples via emitTick(). Returns false
    // at end of song.
    virtual bool advance() = 0;

    void emitTick(ZxAyMachine& m)
    {
        pending_.resize(static_cast<size_t>(m.samplesPerTick()) * 2);
        m.renderTick(pending_.data(), m.samplesPerTick());
        cursor_ = 0;
    }

private:
    std::vector<int16_t> pending_;
    int cursor_ = 0;
    bool ended_ = false;
};

} // namespace musix::zxay
