/*
  cmdx -- MDX file parsing.

  Layout, per the published format description:

    <title, Shift_JIS>  0x0d 0x0a 0x1a  <pdx name> 0x00
    base:  word  voice data offset
           word  MML offset, channel 0
           ...   (9 channels, or 16 when the Mercury/PCM8 extension is used)
    base + voice offset:  N * 27-byte voice records

  Every offset in the header is relative to `base` -- the byte immediately
  after the PDX name's NUL terminator -- not to the start of the file.
*/

#include "cmdx.h"

#include <cstring>

namespace cmdx {

const Voice* File::findVoice(uint8_t id) const
{
    for (const Voice& v : voices_)
        if (v.id == id)
            return &v;
    return nullptr;
}

bool File::load(const uint8_t* bytes, size_t len)
{
    error_.clear();
    title_.clear();
    pdxName_.clear();
    voices_.clear();
    channelCount_ = 0;
    for (Channel& c : channels_)
        c = Channel{};

    if (!bytes || len < 8) {
        error_ = "too small";
        return false;
    }
    bytes_.assign(bytes, bytes + len);
    const uint8_t* p = bytes_.data();

    /* Title, terminated by 0d 0a 1a. Some files have no title at all, in which
       case the marker is at offset 0. */
    size_t mark = SIZE_MAX;
    for (size_t i = 0; i + 2 < len; i++) {
        if (p[i] == 0x0D && p[i + 1] == 0x0A && p[i + 2] == 0x1A) {
            mark = i;
            break;
        }
    }
    if (mark == SIZE_MAX) {
        error_ = "no 0d0a1a header marker";
        return false;
    }
    title_.assign(reinterpret_cast<const char*>(p), mark);

    /* PDX name: NUL-terminated, empty when the first byte is already NUL. */
    size_t at = mark + 3;
    if (at >= len) {
        error_ = "truncated after marker";
        return false;
    }
    size_t nameStart = at;
    while (at < len && p[at] != 0x00)
        at++;
    if (at >= len) {
        error_ = "unterminated pdx name";
        return false;
    }
    pdxName_.assign(reinterpret_cast<const char*>(p + nameStart), at - nameStart);
    at++;                       // step over the NUL

    base_ = static_cast<uint32_t>(at);

    uint16_t voiceOff = 0;
    if (!be16(p, len, base_, &voiceOff)) {
        error_ = "truncated voice offset";
        return false;
    }

    /* Channel count. The first MML offset word sits at base+2 and its value is
       relative to base, so the gap between them is the size of the offset
       table: (value - 2) / 2 entries. A 16-channel file can also be spotted by
       its first channel opening with 0xE8 (PCM8 mode shift), which is the
       cross-check used below. */
    uint16_t firstMml = 0;
    if (!be16(p, len, base_ + 2, &firstMml)) {
        error_ = "truncated mml offset table";
        return false;
    }
    if (firstMml < 2) {
        error_ = "bogus first mml offset";
        return false;
    }
    int count = (firstMml - 2) / 2;
    if (count != 9 && count != 16) {
        /* Fall back on the documented sniff rather than rejecting outright --
           a few files in the wild carry padding between the table and the
           first channel, which inflates the computed count. */
        size_t probe = base_ + firstMml;
        count = (probe < len && p[probe] == 0xE8) ? 16 : 9;
    }
    if (count > kMaxChannels)
        count = kMaxChannels;
    channelCount_ = count;

    for (int i = 0; i < count; i++) {
        uint16_t off = 0;
        if (!be16(p, len, base_ + 2 + i * 2, &off)) {
            error_ = "truncated mml offset table";
            return false;
        }
        uint32_t abs = base_ + off;
        if (abs >= len)
            continue;           // empty/unused channel; leave present=false
        channels_[i].offset = abs;
        channels_[i].present = true;
    }

    /* Voices run from the voice offset to the end of the file. The count is
       not stored; records are fixed size, so divide. */
    uint32_t vbase = base_ + voiceOff;
    if (vbase < len) {
        size_t avail = len - vbase;
        size_t n = avail / kVoiceBytes;
        voices_.reserve(n);
        for (size_t i = 0; i < n; i++) {
            Voice v;
            const uint8_t* rec = p + vbase + i * kVoiceBytes;
            v.id = rec[0];
            std::memcpy(v.data, rec, kVoiceBytes);
            voices_.push_back(v);
        }
    }

    return true;
}

} // namespace cmdx
