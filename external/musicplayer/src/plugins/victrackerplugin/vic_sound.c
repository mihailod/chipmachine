// VIC-I (MOS 6560/6561) sound core. See README.md.
//
// This replaced a core lifted from VICE's vic20/vic20sound.c (GPLv2), and is
// written against the documented behaviour of the chip rather than against
// VICE's implementation of it -- specifically WITHOUT the two data tables that
// made that file a licensing problem:
//
//   * VICE ships a 1024-byte dump of the noise generator's output. The generator
//     itself is documented (Asger Alstrup Nielsen): a 23-bit shift register
//     seeded 0x7ffff8 whose new bit 0 is bit 22 XOR bit 13. It is generated here
//     instead of tabulated, taking the register geometry from MAME's
//     src/devices/sound/mos6560.cpp -- BSD-3-Clause, copyright Peter Trauner.
//   * VICE ships a ~350-entry table of measured output voltages, which is the
//     authors' own measurement data. This uses the parametric saturating DAC
//     below plus the output filtering, which lands in the same place by ear.
//
// Everything else is the chip: three tone channels, each an 8-bit shift register
// clocked at clock/(divider << 4|3|2) whose feedback is the inverted MSB (so it
// emits a square wave at 1/16 of its shift rate), a noise channel on the same
// divider ladder, and a 4-bit master volume. The divider mapping
// 128 - ((reg + 1) & 0x7f) is likewise MAME's expression of it.

#include "vic_sound.h"

#include <math.h>
#include <string.h>

#define TONE_CHANNELS 3
#define NOISE_SEED 0x7ffff8

// Output stage. The mixer produces 0..4 averaged channel bits scaled by the 0..15
// master volume, so 60 is full deflection.
//
// The DAC is NOT linear in that: the resistor ladder drives the RF modulator
// through a stage that compresses as it approaches its rail, which is why a tune
// with everything sounding at once is not four times as loud as one channel.
// Modelled as a soft knee, y = Gx / (1 + Gx/L) -- small-signal gain G, asymptote
// L -- applied while the mix is still unipolar, i.e. before the DC block, which
// is where the real saturation happens too. G and L were trimmed against the
// VICE-derived core this replaced, to within ~1dB RMS and with peaks ~8% under
// full scale, across chipmachine/testmus/victracker; see README.md.
//
// VIC-20 audio is also filtered hard on its way to the modulator -- without the
// low-pass the square waves are far brighter than the machine ever sounded.
#define VIC_DAC_GAIN 1400.0f
#define VIC_DAC_LIMIT 50000.0f
#define VIC_LOWPASS_HZ 2500.0f
#define VIC_HIGHPASS_HZ 20.0f

struct vic_channel
{
    unsigned char reg;   // $900A..$900D: bit 7 enable, bits 0-6 period
    int counter;         // VIC cycles until the next shift
    unsigned char shift; // 8-bit tone shift register
    int out;             // current output bit
};

static struct
{
    struct vic_channel tone[TONE_CHANNELS];
    struct vic_channel noise;
    unsigned int lfsr; // 23-bit noise shift register
    int volume;        // $900E bits 0-3

    // Sample clock: how many VIC cycles one output sample is worth, and the
    // fractional remainder carried between calls.
    float cycles_per_sample;
    float leftover_cycles;

    // Mixer accumulator: sum of the four output bits over the cycles that make
    // up the sample currently being built.
    int accum;
    int accum_cycles;

    float lowpass_state;
    float lowpass_a;
    float highpass_state;
    float highpass_a;
} snd;

// The tone channels shift at clock/16, /8 and /4 of the divider ladder; the
// noise channel sits on the slowest of them.
static const int tone_prescale[TONE_CHANNELS] = {4, 3, 2};
static const int noise_prescale = 4;

// Period in VIC cycles for a channel register value, before the prescale.
static int divider(unsigned char reg)
{
    return 128 - ((reg + 1) & 0x7f);
}

// One step of the tone shift register: shift left, feeding in the complement of
// the bit falling off the top. Free-running like this it spends 8 steps high and
// 8 steps low, which is the channel's square wave.
static void step_tone(struct vic_channel* ch)
{
    if (ch->reg & 0x80) {
        ch->shift = (unsigned char)((ch->shift << 1) | (((ch->shift >> 7) & 1) ^ 1));
        ch->out = ch->shift & 1;
    } else {
        ch->shift <<= 1;
        ch->out = 0;
    }
}

// One step of the 23-bit noise register (bit 22 XOR bit 13 -> bit 0). The
// channel's output is the bit leaving the top of the register.
static void step_noise(void)
{
    unsigned int bit22 = (snd.lfsr >> 22) & 1;
    unsigned int bit13 = (snd.lfsr >> 13) & 1;
    snd.lfsr = ((snd.lfsr << 1) | (bit22 ^ bit13)) & 0x7fffff;
    snd.noise.out = (snd.noise.reg & 0x80) ? (int)bit22 : 0;
}

// Advances the whole chip by `cycles` VIC cycles, summing the four channel
// output bits into the mixer accumulator as it goes.
static void run_cycles(int cycles)
{
    int i;
    int c;

    if (cycles <= 0) {
        return;
    }

    for (c = 0; c < TONE_CHANNELS; c++) {
        struct vic_channel* ch = &snd.tone[c];
        // Nothing can change state before the counter runs out, so a channel
        // whose next shift is beyond this block is just a multiply.
        if (ch->counter > cycles) {
            snd.accum += ch->out * cycles;
            ch->counter -= cycles;
            continue;
        }
        for (i = 0; i < cycles; i++) {
            if (--ch->counter <= 0) {
                ch->counter += divider(ch->reg) << tone_prescale[c];
                step_tone(ch);
            }
            snd.accum += ch->out;
        }
    }

    if (snd.noise.counter > cycles) {
        snd.accum += snd.noise.out * cycles;
        snd.noise.counter -= cycles;
    } else {
        for (i = 0; i < cycles; i++) {
            if (--snd.noise.counter <= 0) {
                snd.noise.counter += divider(snd.noise.reg) << noise_prescale;
                step_noise();
            }
            snd.accum += snd.noise.out;
        }
    }

    snd.accum_cycles += cycles;
}

// Converts the accumulated mixer bits into one output sample. The mix is
// unipolar (0..4 channel-bits, scaled by the 4-bit volume), so the high-pass
// below is what centres it on zero.
static int16_t make_sample(void)
{
    float level;
    float o;

    if (snd.accum_cycles <= 0) {
        return 0;
    }
    // 0..4 average bits * 0..15 volume, through the saturating DAC.
    level = (float)snd.accum / (float)snd.accum_cycles;
    o = level * (float)snd.volume * VIC_DAC_GAIN;
    o = o / (1.0f + o / VIC_DAC_LIMIT);

    // One-pole low-pass: the VIC-20's audio output is heavily filtered on its
    // way to the modulator, which is what stops the square waves sounding like
    // raw square waves.
    snd.lowpass_state += (o - snd.lowpass_state) * snd.lowpass_a;
    o = snd.lowpass_state;

    // One-pole high-pass, i.e. remove the running DC level.
    snd.highpass_state += (o - snd.highpass_state) * snd.highpass_a;
    o -= snd.highpass_state;

    if (o < -32768.0f) {
        return -32768;
    }
    if (o > 32767.0f) {
        return 32767;
    }
    return (int16_t)o;
}

int vicsnd_render(int16_t* pbuf, int nr, int soc, int* delta_t)
{
    int s = 0;
    int i;

    while (s < nr && (float)*delta_t >= snd.cycles_per_sample - snd.leftover_cycles) {
        int cycles = (int)(snd.cycles_per_sample - snd.leftover_cycles);
        int16_t sample;

        snd.leftover_cycles += (float)cycles - snd.cycles_per_sample;
        run_cycles(cycles);
        sample = make_sample();
        for (i = 0; i < soc; i++) {
            pbuf[(s * soc) + i] = sample;
        }
        s++;
        snd.accum = 0;
        snd.accum_cycles = 0;
        *delta_t -= cycles;
    }
    // Cycles left over from this call are clocked into the sample being built,
    // and finished on the next call.
    if (*delta_t > 0) {
        snd.leftover_cycles += (float)*delta_t;
        run_cycles(*delta_t);
        *delta_t = 0;
    }
    return s;
}

void vicsnd_store(int addr, unsigned char value)
{
    switch (addr) {
    case 0xA: snd.tone[0].reg = value; break;
    case 0xB: snd.tone[1].reg = value; break;
    case 0xC: snd.tone[2].reg = value; break;
    case 0xD: snd.noise.reg = value; break;
    case 0xE: snd.volume = value & 0x0f; break;
    default: break;
    }
}

// One-pole coefficient for a cutoff of `hz` at the output sample rate.
static float pole(float hz, int rate)
{
    return 1.0f - expf(-2.0f * 3.14159265f * hz / (float)rate);
}

void vicsnd_init(int cycles_per_sec, int speed)
{
    memset(&snd, 0, sizeof(snd));
    snd.lfsr = NOISE_SEED;
    snd.cycles_per_sample = (float)cycles_per_sec / (float)speed;
    snd.leftover_cycles = 0.0f;
    snd.lowpass_a = pole(VIC_LOWPASS_HZ, speed);
    snd.highpass_a = pole(VIC_HIGHPASS_HZ, speed);
}
