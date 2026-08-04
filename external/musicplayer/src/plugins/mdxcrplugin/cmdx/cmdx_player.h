/*
  cmdx -- MDX sequencer (OPM path).

  Emits YM2151 register writes through a callback. Nothing here synthesises
  audio: the register stream IS the observable behaviour, and it is what
  tools/mdxtrace diffs against the mdxmini oracle. A chip (ymfm, BSD-3) is
  only needed later, to make sound.
*/

#ifndef CMDX_PLAYER_H
#define CMDX_PLAYER_H

#include "cmdx.h"

namespace cmdx {

/* Called for every accepted OPM register write, in emission order. */
using RegFn = void (*)(void* ctx, int reg, int val);

constexpr int kDefaultTempo = 200;

struct Track
{
    uint32_t pc = 0;            // offset into the MML stream
    bool     active = false;
    bool     ended = false;

    int  wait = 0;              // frames remaining on the current note/rest
    int  voice = 0;
    int  volume = 0;
    int  pan = 0xC0;            // both speakers
    int  note = -1;             // currently sounding note, -1 = none
    int  detune = 0;
    int  keyOffDisabled = 0;    // set by 0xF7 for the next note
    int  slotMask = 0x0F;       // from the voice record
    bool inited = false;        // per-track init emitted?
    int  connect = 0;           // CONNECT algorithm, selects carriers
    uint8_t voiceTl[4]{};       // voice's own TL per operator
    int  volHeadroom = 0;       // 127-TL budget from volume; 0 = silent
    int  gate = 8;              // 0xF8 gate time, in eighths (8 = full)
    int  keyOffIn = -1;         // frames until the gate-time key-off

    /* 0xF6/0xF5/0xF4 repeat structure. MDX nests these; depth beyond this is
       not seen in the corpus but is clamped rather than overflowing. */
    static constexpr int kMaxNest = 8;
    int  loopCount[kMaxNest]{};
    uint32_t loopStart[kMaxNest]{};
    int  loopDepth = 0;
};

class Player
{
public:
    bool init(const File& file, RegFn fn, void* ctx);

    /* Advance exactly one sequencer frame. Returns false once every track has
       hit an end marker. */
    bool tick();

    int  tempo() const { return tempo_; }
    int  channelCount() const { return channels_; }

private:
    void wr(int reg, int val);
    void resetChip();
    void loadVoice(int ch, int voiceId);
    void trackInit(int ch);
    int  carrierTl(int ch, int op, int voiceTl) const;
    void keyOn(int ch);
    void keyStrike(int ch);
    void keyOff(int ch);
    void setNote(int ch, int note);
    void setVolume(int ch, int vol);
    void setPan(int ch, int pan);

    /* Runs commands for one track until it blocks on a duration. */
    void step(int ch);

    const File* file_ = nullptr;
    RegFn fn_ = nullptr;
    void* ctx_ = nullptr;

    int channels_ = 0;
    int tempo_ = kDefaultTempo;
    Track tr_[kMaxChannels];

    /* Last value written per register, for the few places MDX re-derives a
       register from several independent fields (pan+feedback share 0x20). */
    uint8_t shadow_[256]{};
};

} // namespace cmdx

#endif // CMDX_PLAYER_H
