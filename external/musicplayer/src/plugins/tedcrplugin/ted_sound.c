// TED (MOS 7360/8360) sound core. See README.md.
//
// This replaces the sound half of tedplay (Attila Grosz), which carries a
// GPL-2.0-or-later notice and no licence file at all, and which reaches the
// chip only through a full Commodore 264 machine complete with four ROM images.
// Nothing here derives from it: the register map is the published one (the
// C16/Plus4 memory map and the Plus/4 World TED register reference), and every
// behaviour below was MEASURED through the chip's own register interface -- by
// running generated .prg programs that poke the registers and analysing the
// audio -- and then written from that specification. What was measured, and how,
// is recorded in README.md so the derivation can be re-run.
//
// The chip is small: two square-wave channels, the second of which can emit
// noise instead, a shared 4-bit volume, and nothing else -- no envelopes, no
// filter, no per-channel level.

#include "ted_sound.h"

#include <math.h>
#include <string.h>

/* Strict ISO modes do not expose M_PI. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// The register value N is not the period. The counter reloads with
//
//     D = (1022 - N) & 0x3FF
//
// which is what makes the two documented oddities fall out for free: N = $3FE
// gives D = 0, so the source stops toggling and locks up, and N = $3FF wraps to
// D = 1023, the lowest tone the chip can make (108.35 Hz on PAL) rather than the
// highest. Measured against the register-poking programs, this reproduces the
// output frequency to five significant figures from N = 0 to N = 1023.
#define RELOAD_FROM_N(n) ((1022 - (int)(n)) & 0x3FF)

// $FF11 bits.
#define CTRL_DA 0x80     // "sound reload" / D/A mode
#define CTRL_NOISE 0x40  // channel 2 emits noise
#define CTRL_CH2 0x20    // channel 2 square enable
#define CTRL_CH1 0x10    // channel 1 enable
#define CTRL_VOLUME 0x0F

// Volume is linear over 0..8 and then flat: writing 9..15 sounds exactly like 8.
// (Measured: RMS rises in equal steps to 8 and does not move at all above it.)
#define VOLUME_MAX 8

// Full deflection is two channels at volume 8, so 16 steps. Scaled to land on
// the level this corpus has always played at: measured across the HVTC sample,
// tedplay's output was a very consistent 1/2.39 of an unscaled mix (2.30-2.51
// over 135 tunes), so a single factor matches it and the plugin layer needs no
// gain of its own. Retune this rather than adding gain further up.
#define TED_DAC_SCALE 418

// The output is AC-coupled on the way to the modulator. The corner has to stay
// low: D/A-mode digis are volume-register writes, i.e. they live at the bottom
// of the band, and a lazy DC block eats them.
#define TED_HIGHPASS_HZ 20.0f

struct ted_channel
{
    int reload;    // D, in TED sound cycles; 0 means the source is locked
    int counter;   // cycles until the next reload
    int flipflop;  // square output, 0 or 1
};

static struct
{
    struct ted_channel ch[2];
    unsigned int freq[2];  // the raw 10-bit register values, for re-deriving D

    // 8-bit noise shift register. Left shift, feedback = NOT(b7^b5^b4^b1) into
    // bit 0, output = bit 0, natural period 255. Recovered from the audio: the
    // sequence was read back a bit at a time and this was the only 8-bit
    // recurrence consistent with it. It is generated, never tabulated.
    //
    // $FF is the absorbing state of this recurrence -- the inverted-feedback
    // analogue of all-zeroes in an ordinary LFSR -- and feeds itself forever.
    // Every one of the other 255 states lies on the single 255-long cycle, so
    // any seed but $FF will do.
    unsigned char noise;

    int volume;    // 0..8
    int ctrl;      // last value written to $FF11

    // Sample clock: how many TED cycles one output sample is worth, and the
    // fractional remainder carried between calls.
    float cycles_per_sample;
    float leftover_cycles;

    // Mixer accumulator: the summed channel levels over the cycles making up
    // the sample currently being built. Averaging over the whole sample period
    // is a box filter, which is what keeps the high tones from aliasing badly.
    int accum;
    int accum_cycles;

    float highpass_state;
    float highpass_a;
} snd;

void tedsnd_reset(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        snd.ch[i].reload = RELOAD_FROM_N(0);
        snd.ch[i].counter = snd.ch[i].reload;
        snd.ch[i].flipflop = 0;
        snd.freq[i] = 0;
    }
    snd.noise = 0x00;
    snd.volume = 0;
    snd.ctrl = 0;
    snd.accum = 0;
    snd.accum_cycles = 0;
    snd.leftover_cycles = 0.0f;
    snd.highpass_state = 0.0f;
}

void tedsnd_init(int cycles_per_sec, int speed)
{
    memset(&snd, 0, sizeof(snd));
    if (cycles_per_sec <= 0) cycles_per_sec = TEDSND_CLOCK_PAL;
    if (speed <= 0) speed = 44100;
    snd.cycles_per_sample = (float)cycles_per_sec / (float)speed;
    snd.highpass_a = (float)exp(-2.0 * M_PI * TED_HIGHPASS_HZ / (double)speed);
    tedsnd_reset();
}

// A frequency register write changes the reload value; the counter already
// running keeps its remaining count and picks the new value up at the next
// reload, which is how a reload latch behaves.
//
// A source parked on N = $3FE stops with its output HIGH, not low. That is not
// a detail: it is the whole mechanism behind sample playback on this chip.
// Players lock both counters and then modulate the volume field, and the level
// they are modulating is the held-high flip-flop. Measured -- with the counters
// locked, toggling the volume gives full deflection whether or not bit 7 is set.
static void set_freq(int channel, unsigned int n)
{
    snd.freq[channel] = n & 0x3FF;
    snd.ch[channel].reload = RELOAD_FROM_N(snd.freq[channel]);
    if (!snd.ch[channel].reload) snd.ch[channel].flipflop = 1;
}

void tedsnd_store(int addr, unsigned char value)
{
    switch (addr & 0x1f) {
    case 0x0E:  // channel 1 frequency, low 8 bits
        set_freq(0, (snd.freq[0] & 0x300) | value);
        break;
    case 0x0F:  // channel 2 frequency, low 8 bits
        set_freq(1, (snd.freq[1] & 0x300) | value);
        break;
    case 0x10:  // bits 1-0: channel 2 frequency bits 9-8
        set_freq(1, (snd.freq[1] & 0x0FF) | ((value & 0x03) << 8));
        break;
    case 0x11:
        snd.ctrl = value;
        snd.volume = value & CTRL_VOLUME;
        if (snd.volume > VOLUME_MAX) snd.volume = VOLUME_MAX;
        if (value & CTRL_DA) {
            // D/A mode. The oscillators stop and both flip-flops are held, so
            // the volume field alone reaches the output -- which is exactly how
            // sample playback on this chip works. Measured: with bit 7 set a
            // free-running tone disappears and only the volume writes are heard,
            // and with the counters already locked ($3FE) bit 7 changes nothing.
            int i;
            for (i = 0; i < 2; i++) {
                snd.ch[i].flipflop = 1;
                snd.ch[i].counter = snd.ch[i].reload;
            }
        }
        break;
    case 0x12:  // bits 1-0: channel 1 frequency bits 9-8 (the rest is bitmap base)
        set_freq(0, (snd.freq[0] & 0x0FF) | ((value & 0x03) << 8));
        break;
    default:
        break;
    }
}

// One step of the noise register, taken whenever channel 2 reloads.
static void step_noise(void)
{
    unsigned int fb = 1u ^ ((snd.noise >> 7) & 1u) ^ ((snd.noise >> 5) & 1u) ^
                      ((snd.noise >> 4) & 1u) ^ ((snd.noise >> 1) & 1u);
    snd.noise = (unsigned char)((snd.noise << 1) | fb);
}

// Advances the chip by one TED sound cycle and adds the mixed level to the
// accumulator. A locked source (reload 0) holds its level instead of counting,
// and so does everything while D/A mode is asserted.
static void run_cycle(void)
{
    int level = 0;

    if (!(snd.ctrl & CTRL_DA)) {
        if (snd.ch[0].reload) {
            if (--snd.ch[0].counter <= 0) {
                snd.ch[0].counter = snd.ch[0].reload;
                snd.ch[0].flipflop ^= 1;
            }
        }
        if (snd.ch[1].reload) {
            if (--snd.ch[1].counter <= 0) {
                snd.ch[1].counter = snd.ch[1].reload;
                snd.ch[1].flipflop ^= 1;
                // The noise register advances once per reload of the same
                // counter -- measured as exactly D cycles per step.
                step_noise();
            }
        }
    }

    if (snd.ctrl & CTRL_CH1) level += snd.ch[0].flipflop;
    if (snd.ctrl & (CTRL_CH2 | CTRL_NOISE)) {
        // Noise wins when both bits are set. (Measured; the Plus/4 World
        // register reference claims the square wins, which the chip does not do.)
        level += (snd.ctrl & CTRL_NOISE) ? (snd.noise & 1) : snd.ch[1].flipflop;
    }

    snd.accum += level * snd.volume;
    snd.accum_cycles++;
}

int tedsnd_render(int16_t* pbuf, int nr, int soc, int* delta_t)
{
    int produced = 0;
    int budget = delta_t ? *delta_t : 0;

    while (produced < nr) {
        // How many cycles this sample is worth, carrying the fraction over so
        // the output rate stays exact over time.
        float want = snd.cycles_per_sample + snd.leftover_cycles;
        int whole = (int)want;
        if (whole > budget) break;
        snd.leftover_cycles = want - (float)whole;
        budget -= whole;

        while (whole-- > 0)
            run_cycle();

        float v = snd.accum_cycles ? (float)snd.accum / (float)snd.accum_cycles : 0.0f;
        snd.accum = 0;
        snd.accum_cycles = 0;

        // DC block.
        float out = v - snd.highpass_state;
        snd.highpass_state = v * (1.0f - snd.highpass_a) + snd.highpass_state * snd.highpass_a;

        int s = (int)(out * TED_DAC_SCALE);
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;

        int c;
        for (c = 0; c < soc; c++)
            pbuf[produced * soc + c] = (int16_t)s;
        produced++;
    }

    if (delta_t) *delta_t = budget;
    return produced;
}
