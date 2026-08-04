/*
  cmdx -- MDX sequencer (OPM path).

  Command encoding follows the published MDX description:

    00-7F  rest, duration = n+1
    80-DF  note (n-0x80), next byte is duration-1
    E7     fade out          EF  sync send        F7  suppress next key-off
    E8     PCM8 mode shift   F0  key-on delay     F8  sound length
    E9     LFO delay         F1  end / loop       F9  volume up
    EA     OPM LFO           F2  portamento       FA  volume down
    EB     volume LFO        F3  detune           FB  set volume
    EC     pitch LFO         F4  repeat escape    FC  pan
    ED     ADPCM/noise freq  F5  repeat end       FD  set voice
    EE     sync wait         F6  repeat start     FE  direct OPM register
                                                  FF  set tempo

  Where the published tables are ambiguous (marked AMBIGUOUS below) the
  behaviour is settled by diffing the register trace against the mdxmini
  oracle, not by reading its source.
*/

#include "cmdx_player.h"

namespace cmdx {

/* Voice-record operator order maps straight onto the OPM's slot stride of 8.
   (An M1/C1/M2/C2 remap is the intuitive guess and is wrong -- the oracle's
   register order settles it.) */
static const int kSlotStride[4] = {0, 8, 16, 24};

/* Which operators are carriers depends on the CONNECT algorithm; only carriers
   take their TL from the channel volume, modulators keep the voice's own TL.
   Bit n = operator n (voice-record order). */
/* Measured per CONNECT value by sweeping the oracle and seeing which of the
   four TL registers stop tracking the voice. Note CON=4 is ops 2+3, NOT the
   1+3 you get from the usual M1/M2/C1/C2 slot diagram -- voice-record operator
   order is what matters here, and it is plain stride-8. */
static const uint8_t kCarrierMask[8] = {
    0x8, 0x8, 0x8, 0x8, 0xC, 0xE, 0xE, 0xF
};

void Player::wr(int reg, int val)
{
    if (reg < 0 || reg > 0xFF)
        return;
    shadow_[reg] = static_cast<uint8_t>(val & 0xFF);
    if (fn_)
        fn_(ctx_, reg & 0xFF, val & 0xFF);
}

void Player::resetChip()
{
    /* Key everything off, then clear the global LFO block. */
    for (int c = 0; c < kFmChannels; c++)
        wr(0x08, c);

    wr(0x0F, 0x00);             // noise disable
    wr(0x18, 0x00);             // LFO frequency
    wr(0x19, 0x00);             // amplitude modulation depth
    wr(0x19, 0x80);             // phase modulation depth (bit 7 selects PMD)
    wr(0x1B, 0x00);             // LFO waveform / CT

    /* Per-channel block: RL/FB/CONNECT, key code, key fraction, PMS/AMS. */
    for (int c = 0; c < kFmChannels; c++) {
        wr(0x20 + c, 0xC0);     // both speakers on, feedback 0, connect 0
        wr(0x28 + c, 0x00);
        wr(0x30 + c, 0x00);
        wr(0x38 + c, 0x00);
    }

    /* Operator block, slot-major: all six registers for slot 0, then slot 1... */
    for (int s = 0; s < 32; s++) {
        wr(0x40 + s, 0x00);     // DT1 / MUL
        wr(0x60 + s, 0x00);     // TL
        wr(0x80 + s, 0x00);     // KS / AR
        wr(0xA0 + s, 0x00);     // AME / D1R
        wr(0xC0 + s, 0x00);     // DT2 / D2R
        wr(0xE0 + s, 0x00);     // D1L / RR
    }
}

bool Player::init(const File& file, RegFn fn, void* ctx)
{
    file_ = &file;
    fn_ = fn;
    ctx_ = ctx;
    tempo_ = kDefaultTempo;
    channels_ = file.channelCount();

    for (int i = 0; i < kMaxChannels; i++) {
        tr_[i] = Track{};
        if (i < channels_ && file.channel(i).present) {
            tr_[i].pc = file.channel(i).offset;
            tr_[i].active = true;
        }
    }

    resetChip();
    return true;
}

void Player::loadVoice(int ch, int voiceId)
{
    if (ch >= kFmChannels)
        return;
    const Voice* v = file_->findVoice(static_cast<uint8_t>(voiceId));
    if (!v)
        return;

    const uint8_t* d = v->data;

    /* d[1] = FL/CON, combined with the channel's current pan in register 0x20. */
    int flcon = d[1] & 0x3F;
    wr(0x20 + ch, (tr_[ch].pan & 0xC0) | flcon);

    tr_[ch].slotMask = d[2] & 0x0F;

    tr_[ch].connect = d[1] & 0x07;

    /* Per operator, TL comes LAST: for carriers it is derived from the channel
       volume rather than taken from the voice. */
    for (int op = 0; op < 4; op++) {
        int s = ch + kSlotStride[op];
        wr(0x40 + s, d[3 + op]);       // DT1 / MUL
        wr(0x80 + s, d[11 + op]);      // KS / AR
        wr(0xA0 + s, d[15 + op]);      // AME / D1R
        wr(0xC0 + s, d[19 + op]);      // DT2 / D2R
        wr(0xE0 + s, d[23 + op]);      // D1L / RR
        tr_[ch].voiceTl[op] = d[7 + op];
        wr(0x60 + s, carrierTl(ch, op, d[7 + op]));
    }
}

int Player::carrierTl(int ch, int op, int voiceTl) const
{
    if (!((kCarrierMask[tr_[ch].connect & 7] >> op) & 1))
        return voiceTl;                       // modulator keeps the voice level
    int hr = tr_[ch].volHeadroom * (127 - voiceTl) / 127;
    return 127 - hr;
}

/* Emitted once per track before its first voice load: silence the key, clear
   the key code, and mute the last operator. */
void Player::trackInit(int ch)
{
    if (ch >= kFmChannels)
        return;
    wr(0x28 + ch, 0x00);
    wr(0x30 + ch, 0x00);
    wr(0x08, ch);
    wr(0x78 + ch, 0x7F);
}

/* Pan shares register 0x20 with feedback/connect, and the write is skipped
   when it would not change the byte -- a pan command that matches what the
   voice already established emits nothing at all. */
void Player::setPan(int ch, int pan)
{
    if (ch >= kFmChannels)
        return;
    tr_[ch].pan = (pan & 0x03) << 6;
    int v = tr_[ch].pan | (shadow_[0x20 + ch] & 0x3F);
    if (v != shadow_[0x20 + ch])
        wr(0x20 + ch, v);
}

/* Volume v0-v15 indexes a table, and it combines with the voice's own TL
   MULTIPLICATIVELY on the headroom (127 - TL), not additively:
       TL = 127 - headroom[v] * (127 - voiceTL) / 127     (integer truncation)
   Both halves were measured by sweeping the oracle over a 2-D grid of voice TL
   against volume; the formula reproduces every cell. Before any volume command
   the headroom is 0, i.e. silent -- which is NOT the same as volume 0. */
static const int kVolHeadroom[16] = {
    85, 87, 90, 93, 95, 98, 101, 103, 106, 109, 111, 114, 117, 119, 122, 125
};

void Player::setVolume(int ch, int vol)
{
    if (vol < 0) vol = 0;
    tr_[ch].volume = vol;

    /* Two ways to reach the same headroom. Bit 7 set is the direct-level form
       (MML "@v"), where the low 7 bits ARE the level; otherwise v0-v15 index
       the table. Plain 16..127 is neither and comes out silent. */
    if (vol & 0x80)
        tr_[ch].volHeadroom = 127 - (vol & 0x7F);
    else
        tr_[ch].volHeadroom = (vol <= 15) ? kVolHeadroom[vol] : 0;
}

void Player::setNote(int ch, int note)
{
    if (ch >= kFmChannels)
        return;
    /* OPM key code: 3 bits of note within octave (skipping every 4th value)
       plus octave in the high nibble. */
    static const int kKc[12] = {0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14};

    /* Detune is in 1/64 semitone. The whole semitones move the key code, the
       remainder becomes the key fraction in bits 7..2 of register 0x30
       (measured: detune 5 -> KF 0x14 = 5<<2). Floor division so that negative
       detune borrows correctly rather than truncating toward zero. */
    int d = tr_[ch].detune;
    int semis = d >= 0 ? d / 64 : -((-d + 63) / 64);
    int frac  = d - semis * 64;

    int n = note + semis;
    if (n < 0) n = 0;
    if (n > 95) n = 95;
    int oct = n / 12;
    int idx = n % 12;
    wr(0x28 + ch, static_cast<uint8_t>((oct << 4) | kKc[idx]));
    wr(0x30 + ch, static_cast<uint8_t>((frac & 0x3F) << 2));
}

/* A key-on is a fixed five-part sequence, not just register 0x08. The LFO
   block is re-cleared on EVERY note (measured: 3 notes -> 3 blocks), which is
   why it looked like a stray command in the raw trace. */
void Player::keyOn(int ch)
{
    if (ch >= kFmChannels)
        return;
    wr(0x1B, 0x00);
    wr(0x18, 0x00);
    wr(0x19, 0x00);
    wr(0x19, 0x00);
}

void Player::keyStrike(int ch)
{
    if (ch >= kFmChannels)
        return;
    wr(0x08, ((tr_[ch].slotMask & 0x0F) << 3) | ch);
    for (int op = 0; op < 4; op++)
        if ((kCarrierMask[tr_[ch].connect & 7] >> op) & 1)
            wr(0x60 + ch + kSlotStride[op],
               carrierTl(ch, op, tr_[ch].voiceTl[op]));
}

void Player::keyOff(int ch)
{
    if (ch >= kFmChannels)
        return;
    wr(0x08, ch);
}

void Player::step(int ch)
{
    Track& t = tr_[ch];
    const uint8_t* p = file_->data();
    const size_t len = file_->size();

    /* A track emits its init the first time it runs, regardless of what its
       first command is -- a bare rest is enough. */
    if (!t.inited) {
        trackInit(ch);
        t.inited = true;
    }

    int guard = 0;
    while (t.active && !t.ended && t.wait == 0) {
        if (++guard > 4096) {           // runaway MML; stop this track
            t.ended = true;
            break;
        }
        if (t.pc >= len) {
            t.ended = true;
            break;
        }
        uint8_t op = p[t.pc++];

        if (op < 0x80) {                        // rest
            t.wait = op + 1;
            t.keyOffDisabled = 0;
            t.note = -1;
            continue;
        }
        if (op < 0xE0) {                        // note
            if (t.pc >= len) { t.ended = true; break; }
            int dur = p[t.pc++] + 1;
            int note = op - 0x80;
            keyOn(ch);              // LFO clear precedes the key code
            setNote(ch, note);
            keyStrike(ch);
            t.note = note;
            t.wait = dur;
            /* Gate time shortens the sounding portion: measured as a key-off
               dur*q/8 + 1 frames after the strike, with q >= 8 meaning the note
               holds for its full length. */
            t.keyOffIn = (t.gate < 8) ? (dur * t.gate / 8 + 1) : -1;
            continue;
        }

        switch (op) {
        case 0xFF:                              // tempo
            if (t.pc < len) tempo_ = p[t.pc++];
            break;
        case 0xFE:                              // direct OPM register
            if (t.pc + 1 < len) { int r = p[t.pc]; int v = p[t.pc + 1]; t.pc += 2; wr(r, v); }
            break;
        case 0xFD:                              // set voice
            if (t.pc < len) {
                t.voice = p[t.pc++];
                loadVoice(ch, t.voice);
            }
            break;
        case 0xFC:                              // pan
            if (t.pc < len) setPan(ch, p[t.pc++]);
            break;
        case 0xFB:                              // volume
            if (t.pc < len) setVolume(ch, p[t.pc++]);
            break;
        case 0xFA: setVolume(ch, t.volume - 1); break;
        case 0xF9: setVolume(ch, t.volume + 1); break;
        case 0xF8:                              // gate time, in eighths
            if (t.pc < len) t.gate = p[t.pc++];
            break;
        case 0xF7: t.keyOffDisabled = 1; break;
        case 0xF6:                              // repeat start: count, 0x00
            if (t.pc + 1 < len) {
                int n = p[t.pc]; t.pc += 2;
                if (t.loopDepth < Track::kMaxNest) {
                    t.loopCount[t.loopDepth] = n;
                    t.loopStart[t.loopDepth] = t.pc;
                    t.loopDepth++;
                }
            }
            break;
        case 0xF5:                              // repeat end: back nn bytes
            if (t.pc + 1 < len) {
                int16_t back = static_cast<int16_t>((p[t.pc] << 8) | p[t.pc + 1]);
                t.pc += 2;
                if (t.loopDepth > 0) {
                    int d = t.loopDepth - 1;
                    if (--t.loopCount[d] > 0)
                        t.pc = static_cast<uint32_t>(static_cast<int32_t>(t.pc) + back);
                    else
                        t.loopDepth--;
                }
            }
            break;
        case 0xF4:                              // repeat escape
            if (t.pc + 1 < len) {
                int16_t skip = static_cast<int16_t>((p[t.pc] << 8) | p[t.pc + 1]);
                t.pc += 2;
                if (t.loopDepth > 0 && t.loopCount[t.loopDepth - 1] <= 1)
                    t.pc = static_cast<uint32_t>(static_cast<int32_t>(t.pc) + skip);
            }
            break;
        case 0xF3:                              // detune
            if (t.pc + 1 < len) {
                t.detune = static_cast<int16_t>((p[t.pc] << 8) | p[t.pc + 1]);
                t.pc += 2;
            }
            break;
        case 0xF2:                              // portamento  (AMBIGUOUS)
            if (t.pc + 1 < len) t.pc += 2;
            break;
        case 0xF1:                              // end, or loop back
            if (t.pc < len && p[t.pc] == 0x00) {
                t.pc++;
                t.ended = true;
            } else if (t.pc + 1 < len) {
                int16_t back = static_cast<int16_t>((p[t.pc] << 8) | p[t.pc + 1]);
                t.pc += 2;
                t.pc = static_cast<uint32_t>(static_cast<int32_t>(t.pc) + back);
            } else {
                t.ended = true;
            }
            break;
        case 0xF0:                              // key-on delay
            if (t.pc < len) t.pc++;
            break;
        case 0xEF: if (t.pc < len) t.pc++; break;   // sync send
        case 0xEE: break;                            // sync wait (AMBIGUOUS)
        case 0xED: if (t.pc < len) t.pc++; break;   // ADPCM/noise freq
        case 0xEC:                                   // pitch LFO
        case 0xEB:                                   // volume LFO
            if (t.pc < len) {
                uint8_t m = p[t.pc];
                if (m == 0x80 || m == 0x81) t.pc += 1;
                else t.pc += 3;
            }
            break;
        case 0xEA:                                   // OPM LFO
            if (t.pc < len) {
                uint8_t m = p[t.pc];
                if (m == 0x80 || m == 0x81) {
                    t.pc += 1;
                    if (m == 0x80) {                 // disable: clear the block
                        wr(0x1B, 0x00);
                        wr(0x18, 0x00);
                        wr(0x19, 0x00);
                        wr(0x19, 0x00);
                    }
                } else {
                    t.pc += 4;
                }
            }
            break;
        case 0xE9: if (t.pc < len) t.pc++; break;   // LFO delay
        case 0xE8: break;                            // PCM8 mode shift
        case 0xE7: if (t.pc + 1 < len) t.pc += 2; break;  // fade out
        default:
            /* Unknown opcode: stop rather than desynchronise the stream. */
            t.ended = true;
            break;
        }
    }
}

bool Player::tick()
{
    /* Per channel, and interleaved in this order: a due gate-time key-off,
       then that channel's carrier level refresh. The refresh runs every frame
       whether or not the track does anything, and is what carries volume
       envelopes and fades. */
    for (int ch = 0; ch < kFmChannels; ch++) {
        Track& t = tr_[ch];

        if (t.keyOffIn > 0 && --t.keyOffIn == 0) {
            keyOff(ch);
            t.keyOffIn = -1;
        }
        if (!t.inited)
            continue;
        for (int op = 0; op < 4; op++)
            if ((kCarrierMask[t.connect & 7] >> op) & 1)
                wr(0x60 + ch + kSlotStride[op],
                   carrierTl(ch, op, t.voiceTl[op]));
    }

    bool any = false;
    for (int ch = 0; ch < channels_; ch++) {
        Track& t = tr_[ch];
        if (!t.active || t.ended)
            continue;
        any = true;
        if (t.wait > 0)
            t.wait--;
        if (t.wait == 0)
            step(ch);
    }
    return any;
}

} // namespace cmdx
