#pragma once
//
// Texas Instruments SN76489 / Sega PSG -- the Mega Drive's second sound chip.
//
// Written here from the published hardware description rather than vendored,
// for the same reason victrackerplugin's VIC-I core was: the obvious existing
// cores are entangled. libvgm's sn76496.c is BSD-3 and would be fine on
// licence, but it is welded to libvgm's device framework (EmuStructs / DEV_DEF /
// SoundDevs), which is a lot of scaffolding to drag in for four counters. The
// chip itself is small and completely documented, so it is cheaper and cleaner
// to write it.
//
// Behaviour follows the standard public description of the Sega variant (as
// documented by SMS Power! and MAME's sn76496 header notes):
//
//   * 3 square-wave tone channels, each a 10-bit period counter clocked at
//     clock/16; the output flips each time the counter underflows, so the tone
//     is clock/(32*period).
//   * 1 noise channel driven by a 16-bit LFSR, tapped on bits 0 and 3 and fed
//     back XORed for white noise, or on bit 0 alone for periodic noise.
//   * Noise is clocked at clock/512, /1024, /2048, or from tone channel 2's
//     period -- which is why DefleMask's 20xy "special" noise mode costs you
//     the third square channel.
//   * 4-bit attenuation per channel, 2 dB per step, 15 == silence.
//
// A period register of 0 is treated as 1 (the counter reloads with the full
// range rather than freezing), matching documented Sega behaviour.

#include <cstdint>

namespace dmfcr {

class SN76489
{
public:
    // `clock` is the chip clock in Hz (Mega Drive: master/15 = 3579545 NTSC),
    // `rate` the output sample rate.
    void reset(double clock, double rate);

    // Latch/data write, exactly as the chip sees it on its 8-bit bus.
    void write(uint8_t value);

    // Render one mono sample, nominally in [-1, 1] before mixing.
    float tick();

    // Stereo enables (Game Gear-style) are not used on the Mega Drive PSG;
    // DefleMask's 8xx panning applies to the FM side only.

private:
    void clockChip();

    double clock_ = 3579545.0;
    double rate_ = 44100.0;
    double acc_ = 0.0;      // fractional chip-clocks carried between samples
    double step_ = 0.0;     // chip clocks (÷16) per output sample

    uint16_t period_[4] = { 1, 1, 1, 1 };
    int16_t counter_[4] = { 0, 0, 0, 0 };
    uint8_t volume_[4] = { 15, 15, 15, 15 }; // attenuation, 15 == off
    uint8_t output_[4] = { 0, 0, 0, 0 };

    uint16_t lfsr_ = 0x8000;
    uint8_t noiseCtl_ = 0;
    uint8_t latch_ = 0; // low 3 bits: which register the next data byte updates

    float volTable_[16] = { 0 };
};

} // namespace dmfcr
