#pragma once

// One decoded ZX AY tune, whatever route it takes to make sound.
//
// Three routes exist behind this interface and they have nothing in common
// below it:
//   * Z80TrackerSource -- the tracker's ORIGINAL Z80 replay routine running on
//     ZxAyMachine (pt1/pt2/pt3/stp/psc/sqt).
//   * DumpSource       -- a recorded AY register stream, no player at all
//     (vtx/psg).
//   * a native sequencer written from the published format description
//     (stc/asc/fxm/amad), where no redistributable Z80 player exists.

#include "zxay_format.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace musix::zxay {

struct SongInfo
{
    std::string title;
    std::string author;
    // Length in seconds, 0 when the format does not record one and the player
    // exposes no end-of-song flag.
    int lengthSeconds = 0;
};

class Source
{
public:
    virtual ~Source() = default;

    // Renders interleaved stereo. Returns frames produced; a short return
    // means the tune ended.
    virtual int render(int16_t* out, int frames) = 0;
    virtual bool ended() const = 0;

    const SongInfo& info() const { return info_; }
    // Titles and authors live at fixed offsets that are a property of the
    // FORMAT, not of the playback route, so they are parsed once in
    // createSource() and handed to whichever Source it built.
    void setInfo(const SongInfo& i) { info_ = i; }

protected:
    SongInfo info_;
};

// Builds the right Source for `data`, or nullptr if nothing here can play it.
// `ext` is the lower-cased extension with no dot, used only to break genuine
// content ties (see zxay_format.h).
std::unique_ptr<Source> createSource(std::vector<uint8_t> data,
                                     const std::string& ext, int sampleRate,
                                     Format* detected);

} // namespace musix::zxay
