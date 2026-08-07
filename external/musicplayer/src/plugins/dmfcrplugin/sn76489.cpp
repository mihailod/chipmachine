// SN76489 / Sega PSG -- see sn76489.h.

#include "sn76489.h"

#include <cmath>

namespace dmfcr {

namespace {
// Bits 0 and 3 tapped, feedback XORed in at bit 15. This is the Sega variant
// (Master System / Mega Drive / Game Gear); the discrete TI part uses a 15-bit
// register with different taps, which is not what a .dmf targets.
constexpr uint16_t kNoiseTaps = 0x0009;
constexpr uint16_t kLfsrReset = 0x8000;
} // namespace

void SN76489::reset(double clock, double rate)
{
    clock_ = clock > 0 ? clock : 3579545.0;
    rate_ = rate > 0 ? rate : 44100.0;
    // The counters are clocked at clock/16.
    step_ = (clock_ / 16.0) / rate_;
    acc_ = 0.0;

    for (int i = 0; i < 4; i++) {
        period_[i] = 1;
        counter_[i] = 0;
        volume_[i] = 15; // silent
        output_[i] = 0;
    }
    lfsr_ = kLfsrReset;
    noiseCtl_ = 0;
    latch_ = 0;

    // 2 dB per attenuation step; 15 is hard off. 10^(-2/20) == 0.79432823.
    for (int i = 0; i < 15; i++) {
        volTable_[i] = static_cast<float>(std::pow(10.0, -0.1 * i));
    }
    volTable_[15] = 0.0f;
}

void SN76489::write(uint8_t value)
{
    if ((value & 0x80) != 0) {
        // LATCH/DATA: %1cctdddd -- channel, type (0 = tone, 1 = volume), data
        latch_ = (value >> 4) & 0x07;
        int ch = (latch_ >> 1) & 0x03;
        if ((latch_ & 1) != 0) {
            volume_[ch] = value & 0x0F;
        } else if (ch == 3) {
            noiseCtl_ = value & 0x07;
            lfsr_ = kLfsrReset; // writing the noise control resets the shifter
        } else {
            period_[ch] = static_cast<uint16_t>((period_[ch] & 0x3F0) | (value & 0x0F));
        }
    } else {
        // DATA: %0-dddddd -- upper 6 bits of the latched tone period, or a
        // volume/noise-control update if that is what was latched.
        int ch = (latch_ >> 1) & 0x03;
        if ((latch_ & 1) != 0) {
            volume_[ch] = value & 0x0F;
        } else if (ch == 3) {
            noiseCtl_ = value & 0x07;
            lfsr_ = kLfsrReset;
        } else {
            period_[ch] =
                static_cast<uint16_t>((period_[ch] & 0x0F) | ((value & 0x3F) << 4));
        }
    }
}

void SN76489::clockChip()
{
    // Tone channels.
    for (int i = 0; i < 3; i++) {
        if (--counter_[i] <= 0) {
            uint16_t p = period_[i];
            if (p == 0) { p = 1; }
            counter_[i] = static_cast<int16_t>(p);
            // A period below 2 is above the audible range; real hardware emits a
            // DC level there rather than a tone, which is what games rely on to
            // silence a channel without touching its volume.
            if (p > 1) { output_[i] ^= 1; }
            else { output_[i] = 1; }
        }
    }

    // Noise channel.
    if (--counter_[3] <= 0) {
        int shiftRate = noiseCtl_ & 0x03;
        uint16_t p;
        switch (shiftRate) {
        case 0: p = 0x10; break;  // clock/512  (÷16 already applied)
        case 1: p = 0x20; break;  // clock/1024
        case 2: p = 0x40; break;  // clock/2048
        default:                  // tracks tone channel 2
            p = period_[2] != 0 ? period_[2] : 1;
            break;
        }
        counter_[3] = static_cast<int16_t>(p);

        // The LFSR advances at half the counter rate: one shift per output
        // toggle, not per reload.
        output_[3] ^= 1;
        if (output_[3] != 0) {
            bool white = (noiseCtl_ & 0x04) != 0;
            uint16_t bit;
            if (white) {
                uint16_t t = lfsr_ & kNoiseTaps;
                // parity of the tapped bits
                t ^= static_cast<uint16_t>(t >> 8);
                t ^= static_cast<uint16_t>(t >> 4);
                t ^= static_cast<uint16_t>(t >> 2);
                t ^= static_cast<uint16_t>(t >> 1);
                bit = static_cast<uint16_t>(t & 1);
            } else {
                bit = static_cast<uint16_t>(lfsr_ & 1);
            }
            lfsr_ = static_cast<uint16_t>((lfsr_ >> 1) | (bit << 15));
        }
    }
}

float SN76489::tick()
{
    acc_ += step_;
    int steps = static_cast<int>(acc_);
    acc_ -= steps;
    // Guard against a pathological rate; the chip is ~224 kHz internally so a
    // 44.1 kHz output only ever needs a handful of clocks per sample.
    if (steps > 4096) { steps = 4096; }
    for (int i = 0; i < steps; i++) { clockChip(); }

    float out = 0.0f;
    for (int i = 0; i < 3; i++) {
        out += (output_[i] != 0 ? 1.0f : -1.0f) * volTable_[volume_[i]];
    }
    // The noise output is the LFSR's low bit, not the toggle.
    out += ((lfsr_ & 1) != 0 ? 1.0f : -1.0f) * volTable_[volume_[3]];

    // Mix level for the PSG against the FM side. Swept against Furnace over the
    // whole corpus: 0.25 keeps the overall level closest (RMS ratio 1.13, versus
    // 0.84 at 0.15), and although lower values nudge the cosine up a little
    // (0.9342 -> 0.9357 at 0.15) they do it by making everything quieter, which
    // is fitting the metric rather than the hardware.
    return out * 0.25f;
}

} // namespace dmfcr
