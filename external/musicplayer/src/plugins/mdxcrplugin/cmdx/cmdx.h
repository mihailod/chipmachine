/*
  cmdx -- an independent MDX (Sharp X68000 / MXDRV) sequencer.

  Written against the published MDX format description, NOT derived from
  mdxmini/mdxplay (GPL-2). mdxmini is used only as a black-box oracle: its
  register trace is diffed against ours by tools/mdxtrace. See that directory's
  README for the provenance note.

  Scope: the OPM (YM2151) path only. Tunes that reference a .pdx sample bank
  need the PCM8 software multiplexer, which is deliberately out of scope --
  they are classified MDX-PCM and routed elsewhere. Measured over 897 traced
  tunes, no tune without a bank reference ever starts a PCM8 voice, so "no
  bank" is a sound guarantee of OPM-only, not a heuristic.

  All multi-byte fields are big-endian: the X68000 is a 68000.
*/

#ifndef CMDX_H
#define CMDX_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cmdx {

constexpr int kMaxChannels = 16;   // A-H (FM) + P (PCM) + Q-W (Mercury)
constexpr int kFmChannels  = 8;
constexpr int kVoiceBytes  = 27;

struct Voice
{
    uint8_t id = 0;
    uint8_t data[kVoiceBytes]{};   // raw record, mapped to OPM regs at load
};

struct Channel
{
    uint32_t offset = 0;           // absolute offset of this channel's MML
    bool     present = false;
};

/* Parsed MDX file. Owns its bytes. */
class File
{
public:
    /* Returns false and sets error() if the data is not a usable MDX. */
    bool load(const uint8_t* bytes, size_t len);

    const std::string& title() const { return title_; }
    const std::string& pdxName() const { return pdxName_; }
    bool  needsPdx() const { return !pdxName_.empty(); }

    int   channelCount() const { return channelCount_; }
    const Channel& channel(int i) const { return channels_[i]; }

    const std::vector<Voice>& voices() const { return voices_; }
    const Voice* findVoice(uint8_t id) const;

    const uint8_t* data() const { return bytes_.data(); }
    size_t size() const { return bytes_.size(); }

    const std::string& error() const { return error_; }

private:
    std::vector<uint8_t> bytes_;
    std::string title_;
    std::string pdxName_;
    std::string error_;

    uint32_t base_ = 0;            // offsets in the header are relative to this
    int      channelCount_ = 0;
    Channel  channels_[kMaxChannels];
    std::vector<Voice> voices_;
};

/* Big-endian 16-bit read with bounds check. */
inline bool be16(const uint8_t* p, size_t len, size_t at, uint16_t* out)
{
    if (at + 1 >= len)
        return false;
    *out = static_cast<uint16_t>((p[at] << 8) | p[at + 1]);
    return true;
}

} // namespace cmdx

#endif // CMDX_H
