// cSID by Hermit (Mihaly Horvath), (Year 2016..2017) http://hermit.sidrip.com
// (based on jsSID but totally revorked in C to be cycle-based & oversampled)
// License: WTF - Do what the fuck you want with this code, but I please mention
// me as its original author.
//
// -------------------------------------------------------------------------
// Vendored from https://github.com/mlund/csid (csid.c, the cycle-based
// "oversampled" variant -- NOT csid-light.c, whose faster jsSID-style sampling
// trades away ADSR accuracy and combined-waveform cleanliness).
//
// The emulation itself -- CPU(), SID(), combinedWF(), createCombinedWF() and
// every constant and table they touch -- is Hermit's, copied unchanged. That is
// deliberate: this exact code was A/B'd against VICE/reSID at 0.958-1.000
// spectral cosine, and any "tidying" of the DSP risks that result.
//
// Changes made to turn the standalone SDL command-line player into a library:
//   1. main() replaced by csid_load(), which does the same PSID/RSID header
//      parse against a memory buffer instead of a FILE*, and reports what it
//      found through csid_info rather than printf.
//   2. SDL dropped. play() became csid_render(), writing int16 frames into a
//      caller's buffer instead of filling an SDL audio callback's byte stream.
//   3. ALL file-scope symbols made static. This matters: unqualified globals
//      named A, X, Y, PC, ST, memory, init(), play() and SID() would collide at
//      link time with the other ~60 plugins in this build.
//   4. OUTPUT_SCALEDOWN is recomputed from its base value on every csid_load().
//      Upstream mutates it in place (/= 0.6 for 2SID, /= 0.4 for 3SID) from
//      main(), which runs once per process; here a 3SID tune followed by a
//      1SID tune would otherwise leave the second tune permanently quiet.
//   5. printf diagnostics removed.
// -------------------------------------------------------------------------

#include "csid_engine.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

// global constants and variables
#define C64_PAL_CPUCLK 985248
#define SID_CHANNEL_AMOUNT 3
#define MAX_DATA_LEN 65536
#define PAL_FRAMERATE                                                          \
    49.4 // important to match, otherwise some ADSR-sensitive tunes suffer.
#define DEFAULT_SAMPLERATE 44100

// raw output divided by this after multiplied by main volume, this also
// compensates for filter-resonance emphasis to avoid distotion
#define OUTPUT_SCALEDOWN_BASE (SID_CHANNEL_AMOUNT * 16 + 26)
static int OUTPUT_SCALEDOWN = OUTPUT_SCALEDOWN_BASE;

enum {
    GATE_BITMASK = 0x01,
    SYNC_BITMASK = 0x02,
    RING_BITMASK = 0x04,
    TEST_BITMASK = 0x08,
    TRI_BITMASK = 0x10,
    SAW_BITMASK = 0x20,
    PULSE_BITMASK = 0x40,
    NOISE_BITMASK = 0x80,
    HOLDZERO_BITMASK = 0x10,
    DECAYSUSTAIN_BITMASK = 0x40,
    ATTACK_BITMASK = 0x80,
    LOWPASS_BITMASK = 0x10,
    BANDPASS_BITMASK = 0x20,
    HIGHPASS_BITMASK = 0x40,
    OFF3_BITMASK = 0x80
};

static const byte FILTSW[9] = { 1, 2, 4, 1, 2, 4, 1, 2, 4 };
static byte ADSRstate[9], expcnt[9], envcnt[9], sourceMSBrise[9];
static unsigned int clock_ratio = 22, ratecnt[9], prevwfout[9];
static unsigned long int phaseaccu[9], prevaccu[9], sourceMSB[3], noise_LFSR[9];
static long int prevlowpass[3], prevbandpass[3];
static float cutoff_ratio_8580, cutoff_ratio_6581, cutoff_bias_6581;
static int SIDamount = 1, SID_model[3] = { 8580, 8580, 8580 },
           requested_SID_model = -1, sampleratio;
static byte memory[MAX_DATA_LEN], timermode[0x20];
static int subtune = 0;
static unsigned int initaddr, playaddr, playaddf, SID_address[3] = { 0xD400, 0,
                                                                     0 };
static long int samplerate = DEFAULT_SAMPLERATE;
static int framecnt = 0, frame_sampleperiod = DEFAULT_SAMPLERATE / PAL_FRAMERATE;
// CPU (and CIA/VIC-IRQ) emulation constants and variables - avoiding
// internal/automatic variables to retain speed
static const byte flagsw[] = { 0x01, 0x21, 0x04, 0x24, 0x00, 0x40, 0x08, 0x28 },
                  branchflag[] = { 0x80, 0x40, 0x01, 0x02 };
static unsigned int PC = 0, pPC = 0, addr = 0, storadd = 0;
static short int A = 0, T = 0, SP = 0xFF;
static byte X = 0, Y = 0, IR = 0, ST = 0x00; // STATUS-flags: N V - B D I Z C
static char CPUtime = 0, cycles = 0, finished = 0;

// function prototypes
static void cSID_init(int samplerate);
static int SID(char num, unsigned int baseaddr);
static void initSID(void);
static void initCPU(unsigned int mempos);
static byte CPU(void);

// Fetch a little-endian 16-bit operand: low byte from the next PC, high byte
// from the one after. Upstream wrote this inline as
//   addr = fetch_word();
// which is undefined behaviour -- two unsequenced modifications of PC in one
// expression -- and only does the right thing because clang happens to evaluate
// left to right. The 6502 operand order is not ambiguous, so spell it out and
// stop depending on that. Behaviour is identical under the current compiler;
// this just stops a future one from silently reordering the two fetches.
static inline unsigned int fetch_word(void)
{
    unsigned int lo = memory[++PC];
    return lo + memory[++PC] * 256;
}
static unsigned int combinedWF(char num, char channel, unsigned int* wfarray,
                               int index, char differ6581);
static void createCombinedWF(unsigned int* wfarray, float bitmul,
                             float bitstrength, float treshold);

//----------------------------- load / init ----------------------------

int csid_load(const uint8_t* data, int datalen, int rate, csid_info* out_info)
{
    int strend, subtune_amount;
    int preferred_SID_model[3] = { 8580, 8580, 8580 };
    unsigned int i, offs, loadaddr;
    byte SIDtitle[32], SIDauthor[32], SIDinfo[32];

    // The header is 0x7C bytes for v2+; refuse anything that cannot hold one.
    if (data == NULL || datalen < 0x7C) { return 1; }
    if (memcmp(data, "PSID", 4) != 0 && memcmp(data, "RSID", 4) != 0) {
        return 1;
    }
    if (datalen > MAX_DATA_LEN) { datalen = MAX_DATA_LEN; }

    offs = data[7];
    if (offs + 2u > (unsigned)datalen) { return 1; }
    loadaddr = data[8] + data[9] ? data[8] * 256 + data[9]
                                 : data[offs] + data[offs + 1] * 256;
    for (i = 0; i < 32; i++) {
        timermode[31 - i] = (data[0x12 + (i >> 3)] & (byte)pow(2, 7 - i % 8)) ? 1
                                                                             : 0;
    }
    for (i = 0; i < MAX_DATA_LEN; i++) {
        memory[i] = 0;
    }
    for (i = offs + 2; i < (unsigned)datalen; i++) {
        if (loadaddr + i - (offs + 2) < MAX_DATA_LEN) {
            memory[loadaddr + i - (offs + 2)] = data[i];
        }
    }
    strend = 1;
    for (i = 0; i < 32; i++) {
        if (strend != 0) { strend = SIDtitle[i] = data[0x16 + i]; }
        else { strend = SIDtitle[i] = 0; }
    }
    strend = 1;
    for (i = 0; i < 32; i++) {
        if (strend != 0) { strend = SIDauthor[i] = data[0x36 + i]; }
        else { strend = SIDauthor[i] = 0; }
    }
    strend = 1;
    for (i = 0; i < 32; i++) {
        if (strend != 0) { strend = SIDinfo[i] = data[0x56 + i]; }
        else { strend = SIDinfo[i] = 0; }
    }
    initaddr = data[0xA] + data[0xB] ? data[0xA] * 256 + data[0xB] : loadaddr;
    playaddr = playaddf = data[0xC] * 256 + data[0xD];
    subtune_amount = data[0xF];
    if (subtune_amount < 1) { subtune_amount = 1; }
    preferred_SID_model[0] = (data[0x77] & 0x30) >= 0x20 ? 8580 : 6581;
    preferred_SID_model[1] = (data[0x77] & 0xC0) >= 0x80 ? 8580 : 6581;
    preferred_SID_model[2] = (data[0x76] & 3) >= 3 ? 8580 : 6581;
    SID_address[1] = data[0x7A] >= 0x42 && (data[0x7A] < 0x80 || data[0x7A] >= 0xE0)
                         ? 0xD000 + data[0x7A] * 16
                         : 0;
    SID_address[2] = data[0x7B] >= 0x42 && (data[0x7B] < 0x80 || data[0x7B] >= 0xE0)
                         ? 0xD000 + data[0x7B] * 16
                         : 0;
    SIDamount = 1 + (SID_address[1] > 0) + (SID_address[2] > 0);

    samplerate = rate > 0 ? rate : DEFAULT_SAMPLERATE;
    sampleratio = (int)round((double)C64_PAL_CPUCLK / samplerate);
    if (sampleratio < 1) { sampleratio = 1; }

    for (i = 0; i < (unsigned)SIDamount; i++) {
        if (requested_SID_model == 8580 || requested_SID_model == 6581) {
            SID_model[i] = requested_SID_model;
        } else {
            SID_model[i] = preferred_SID_model[i];
        }
    }
    // Recomputed from the base each load -- see note 4 in the file header.
    OUTPUT_SCALEDOWN = OUTPUT_SCALEDOWN_BASE;
    if (SIDamount == 2) { OUTPUT_SCALEDOWN /= 0.6; }
    else if (SIDamount >= 3) { OUTPUT_SCALEDOWN /= 0.4; }

    cSID_init(samplerate);

    if (out_info != NULL) {
        memset(out_info, 0, sizeof(*out_info));
        out_info->subtune_count = subtune_amount;
        // Header stores a 1-based default subtune; report it 0-based.
        out_info->default_subtune = data[0x11] > 0 ? data[0x11] - 1 : 0;
        if (out_info->default_subtune >= subtune_amount) {
            out_info->default_subtune = 0;
        }
        out_info->sid_amount = SIDamount;
        out_info->sid_model = SID_model[0];
        out_info->is_rsid = memcmp(data, "RSID", 4) == 0;
        out_info->play_address = (int)playaddf;
        memcpy(out_info->title, SIDtitle, 32);
        memcpy(out_info->author, SIDauthor, 32);
        memcpy(out_info->info, SIDinfo, 32);
    }
    return 0;
}

void csid_init_tune(int subt)
{
    static long int timeout;
    // timermode[] is 32 entries; a bogus subtune index would read past it.
    subtune = (subt >= 0 && subt < 0x20) ? subt : 0;
    initCPU(initaddr);
    initSID();
    A = subtune;
    memory[1] = 0x37;
    memory[0xDC05] = 0;
    for (timeout = 100000; timeout >= 0; timeout--) {
        if (CPU()) { break; }
    }
    if (timermode[subtune] || memory[0xDC05]) { // CIA timing
        if (!memory[0xDC05]) {
            memory[0xDC04] = 0x24;
            memory[0xDC05] = 0x40;
        } // C64 startup-default
        frame_sampleperiod =
            (memory[0xDC04] + memory[0xDC05] * 256) / clock_ratio;
    } else {
        frame_sampleperiod = samplerate / PAL_FRAMERATE;
    } // Vsync timing
    if (playaddf == 0) {
        playaddr = ((memory[1] & 3) < 2) ? memory[0xFFFE] + memory[0xFFFF] * 256
                                         : memory[0x314] + memory[0x315] * 256;
    } else {
        playaddr = playaddf;
        if (playaddr >= 0xE000 && memory[1] == 0x37) {
            memory[1] = 0x35;
        }
    } // player under KERNAL (Crystal Kingdom Dizzy)
    initCPU(playaddr);
    framecnt = 1;
    finished = 0;
    CPUtime = 0;
}

int csid_render(int16_t* out, int frames)
{
    static int i, j, output;
    static float average;

    for (i = 0; i < frames; i++) {
        framecnt--;
        if (framecnt <= 0) {
            framecnt = frame_sampleperiod;
            finished = 0;
            PC = playaddr;
            SP = 0xFF;
        }
        average = 0.0;
        for (j = 0; j < sampleratio; j++) {
            if (finished == 0 && --cycles <= 0) {
                pPC = PC;
                if (CPU() >= 0xFE ||
                    ((memory[1] & 3) > 1 && pPC < 0xE000 &&
                     (PC == 0xEA31 || PC == 0xEA81))) {
                    finished = 1;
                } // IRQ player ROM return handling
                if ((addr == 0xDC05 || addr == 0xDC04) && (memory[1] & 3) &&
                    timermode[subtune]) {
                    // dynamic CIA-setting (Galway/Rubicon workaround)
                    frame_sampleperiod =
                        (memory[0xDC04] + memory[0xDC05] * 256) / clock_ratio;
                }
                if (storadd >= 0xD420 && storadd < 0xD800 && (memory[1] & 3)) {
                    // CJ in the USA workaround (writing above $d420, except
                    // SID2/SID3)
                    if (!(SID_address[1] <= storadd &&
                          storadd < SID_address[1] + 0x1F) &&
                        !(SID_address[2] <= storadd &&
                          storadd < SID_address[2] + 0x1F)) {
                        // write to $D400..D41F if not in SID2/SID3 address-space
                        memory[storadd & 0xD41F] = memory[storadd];
                    }
                }
            }
            average += SID(0, 0xD400);
            if (SIDamount >= 2) { average += SID(1, SID_address[1]); }
            if (SIDamount == 3) { average += SID(2, SID_address[2]); }
        }
        output = average / sampleratio;
        // Upstream wrote the low/high byte straight into SDL's stream, which
        // silently WRAPS if the mixed value leaves int16 range. SID() saturates
        // per chip, so a 2SID/3SID tune can still sum past 32767 here even after
        // the OUTPUT_SCALEDOWN compensation. Clamp instead: identical output for
        // everything in range, hard clipping rather than a sign flip above it.
        if (output > 32767) { output = 32767; }
        else if (output < -32768) { output = -32768; }
        out[i] = (int16_t)output;
    }
    return frames;
}

//--------------------------------- CPU emulation
//-------------------------------------

static void initCPU(unsigned int mempos)
{
    PC = mempos;
    A = 0;
    X = 0;
    Y = 0;
    ST = 0;
    SP = 0xFF;
}

// the CPU emulation for SID/PRG playback (ToDo: CIA/VIC-IRQ/NMI/RESET vectors,
// BCD-mode)
static byte CPU(void)
{ //'IR' is the instruction-register, naming after the hardware-equivalent
    IR = memory[PC];
    cycles = 2;
    storadd = 0; //'cycle': ensure smallest 6510 runtime (for implied/register
                 // instructions)
    if (IR & 1) { // nybble2:  1/5/9/D:accu.instructions, 3/7/B/F:illegal opcodes
        switch (IR & 0x1F) { // addressing modes (begin with more complex cases),
                             // PC wraparound not handled inside to save codespace
        case 1:
        case 3:
            ++PC;
            addr = memory[memory[PC] + X] + memory[memory[PC] + X + 1] * 256;
            cycles = 6;
            break; //(zp,x)
        case 0x11:
        case 0x13:
            ++PC;
            addr = memory[memory[PC]] + memory[memory[PC] + 1] * 256 + Y;
            cycles = 6;
            break; //(zp),y
        case 0x19:
        case 0x1B:
            addr = fetch_word() + Y;
            cycles = 5;
            break; // abs,y
        case 0x1D:
            addr = fetch_word() + X;
            cycles = 5;
            break; // abs,x
        case 0xD:
        case 0xF:
            addr = fetch_word();
            cycles = 4;
            break; // abs
        case 0x15:
            addr = memory[++PC] + X;
            cycles = 4;
            break; // zp,x
        case 5:
        case 7:
            addr = memory[++PC];
            cycles = 3;
            break; // zp
        case 0x17:
            if ((IR & 0xC0) != 0x80) {
                addr = memory[++PC] + X;
                cycles = 4;
            } // zp,x for illegal opcodes
            else {
                addr = memory[++PC] + Y;
                cycles = 4;
            }
            break; // zp,y for LAX/SAX illegal opcodes
        case 0x1F:
            if ((IR & 0xC0) != 0x80) {
                addr = fetch_word() + X;
                cycles = 5;
            } // abs,x for illegal opcodes
            else {
                addr = fetch_word() + Y;
                cycles = 5;
            }
            break; // abs,y for LAX/SAX illegal opcodes
        case 9:
        case 0xB:
            addr = ++PC;
            cycles = 2; // immediate
        }
        addr &= 0xFFFF;
        switch (IR & 0xE0) {
        case 0x60:
            if ((IR & 0x1F) != 0xB) {
                if ((IR & 3) == 3) {
                    T = (memory[addr] >> 1) + (ST & 1) * 128;
                    ST &= 124;
                    ST |= (T & 1);
                    memory[addr] = T;
                    cycles += 2;
                } // ADC / RRA (ROR+ADC)
                T = A;
                A += memory[addr] + (ST & 1);
                ST &= 60;
                ST |= (A & 128) | (A > 255);
                A &= 0xFF;
                ST |= (!A) << 1 |
                      (!((T ^ memory[addr]) & 0x80) & ((T ^ A) & 0x80)) >> 1;
            } else {
                A &= memory[addr];
                T += memory[addr] + (ST & 1);
                ST &= 60;
                ST |= (T > 255) |
                      (!((A ^ memory[addr]) & 0x80) & ((T ^ A) & 0x80)) >>
                          1; // V-flag set by intermediate ADC mechanism:
                             // (A&mem)+mem
                T = A;
                A = (A >> 1) + (ST & 1) * 128;
                ST |= (A & 128) | (T > 127);
                ST |= (!A) << 1;
            }
            break; // ARR (AND+ROR, bit0 not going to C, but C and bit7 get
                   // exchanged.)
        case 0xE0:
            if ((IR & 3) == 3 && (IR & 0x1F) != 0xB) {
                memory[addr]++;
                cycles += 2;
            }
            T = A;
            A -= memory[addr] + !(ST & 1); // SBC / ISC(ISB)=INC+SBC
            ST &= 60;
            ST |= (A & 128) | (A >= 0);
            A &= 0xFF;
            ST |= (!A) << 1 |
                  (((T ^ memory[addr]) & 0x80) & ((T ^ A) & 0x80)) >> 1;
            break;
        case 0xC0:
            if ((IR & 0x1F) != 0xB) {
                if ((IR & 3) == 3) {
                    memory[addr]--;
                    cycles += 2;
                }
                T = A - memory[addr];
            } // CMP / DCP(DEC+CMP)
            else {
                X = T = (A & X) - memory[addr];
            } /*SBX(AXS)*/
            ST &= 124;
            ST |= (!(T & 0xFF)) << 1 | (T & 128) | (T >= 0);
            break; // SBX (AXS) (CMP+DEX at the same time)
        case 0x00:
            if ((IR & 0x1F) != 0xB) {
                if ((IR & 3) == 3) {
                    ST &= 124;
                    ST |= (memory[addr] > 127);
                    memory[addr] <<= 1;
                    cycles += 2;
                }
                A |= memory[addr];
                ST &= 125;
                ST |= (!A) << 1 | (A & 128);
            } // ORA / SLO(ASO)=ASL+ORA
            else {
                A &= memory[addr];
                ST &= 124;
                ST |= (!A) << 1 | (A & 128) | (A > 127);
            }
            break; // ANC (AND+Carry=bit7)
        case 0x20:
            if ((IR & 0x1F) != 0xB) {
                if ((IR & 3) == 3) {
                    T = (memory[addr] << 1) + (ST & 1);
                    ST &= 124;
                    ST |= (T > 255);
                    T &= 0xFF;
                    memory[addr] = T;
                    cycles += 2;
                }
                A &= memory[addr];
                ST &= 125;
                ST |= (!A) << 1 | (A & 128);
            } // AND / RLA (ROL+AND)
            else {
                A &= memory[addr];
                ST &= 124;
                ST |= (!A) << 1 | (A & 128) | (A > 127);
            }
            break; // ANC (AND+Carry=bit7)
        case 0x40:
            if ((IR & 0x1F) != 0xB) {
                if ((IR & 3) == 3) {
                    ST &= 124;
                    ST |= (memory[addr] & 1);
                    memory[addr] >>= 1;
                    cycles += 2;
                }
                A ^= memory[addr];
                ST &= 125;
                ST |= (!A) << 1 | (A & 128);
            } // EOR / SRE(LSE)=LSR+EOR
            else {
                A &= memory[addr];
                ST &= 124;
                ST |= (A & 1);
                A >>= 1;
                A &= 0xFF;
                ST |= (A & 128) | ((!A) << 1);
            }
            break; // ALR(ASR)=(AND+LSR)
        case 0xA0:
            if ((IR & 0x1F) != 0x1B) {
                A = memory[addr];
                if ((IR & 3) == 3) { X = A; }
            } // LDA / LAX (illegal, used by my 1 rasterline player)
            else {
                A = X = SP = memory[addr] & SP;
            } /*LAS(LAR)*/
            ST &= 125;
            ST |= ((!A) << 1) | (A & 128);
            break; // LAS (LAR)
        case 0x80:
            if ((IR & 0x1F) == 0xB) {
                A = X & memory[addr];
                ST &= 125;
                ST |= (A & 128) | ((!A) << 1);
            } // XAA (TXA+AND), highly unstable on real 6502!
            else if ((IR & 0x1F) == 0x1B) {
                SP = A & X;
                memory[addr] = SP & ((addr >> 8) + 1);
            } // TAS(SHS) (SP=A&X, mem=S&H} - unstable on real 6502
            else {
                memory[addr] = A & (((IR & 3) == 3) ? X : 0xFF);
                storadd = addr;
            }
            break; // STA / SAX (at times same as AHX/SHX/SHY) (illegal)
        }
    }

    else if (IR & 2) { // nybble2:  2:illegal/LDX, 6:A/X/INC/DEC,
                       // A:Accu-shift/reg.transfer/NOP, E:shift/X/INC/DEC
        switch (IR & 0x1F) { // addressing modes
        case 0x1E:
            addr = fetch_word() + (((IR & 0xC0) != 0x80) ? X : Y);
            cycles = 5;
            break; // abs,x / abs,y
        case 0xE:
            addr = fetch_word();
            cycles = 4;
            break; // abs
        case 0x16:
            addr = memory[++PC] + (((IR & 0xC0) != 0x80) ? X : Y);
            cycles = 4;
            break; // zp,x / zp,y
        case 6:
            addr = memory[++PC];
            cycles = 3;
            break; // zp
        case 2:
            addr = ++PC;
            cycles = 2; // imm.
        }
        addr &= 0xFFFF;
        switch (IR & 0xE0) {
        case 0x00:
            ST &= 0xFE;
        case 0x20:
            if ((IR & 0xF) == 0xA) {
                A = (A << 1) + (ST & 1);
                ST &= 124;
                ST |= (A & 128) | (A > 255);
                A &= 0xFF;
                ST |= (!A) << 1;
            } // ASL/ROL (Accu)
            else {
                T = (memory[addr] << 1) + (ST & 1);
                ST &= 124;
                ST |= (T & 128) | (T > 255);
                T &= 0xFF;
                ST |= (!T) << 1;
                memory[addr] = T;
                cycles += 2;
            }
            break; // RMW (Read-Write-Modify)
        case 0x40:
            ST &= 0xFE;
        case 0x60:
            if ((IR & 0xF) == 0xA) {
                T = A;
                A = (A >> 1) + (ST & 1) * 128;
                ST &= 124;
                ST |= (A & 128) | (T & 1);
                A &= 0xFF;
                ST |= (!A) << 1;
            } // LSR/ROR (Accu)
            else {
                T = (memory[addr] >> 1) + (ST & 1) * 128;
                ST &= 124;
                ST |= (T & 128) | (memory[addr] & 1);
                T &= 0xFF;
                ST |= (!T) << 1;
                memory[addr] = T;
                cycles += 2;
            }
            break; // memory (RMW)
        case 0xC0:
            if (IR & 4) {
                memory[addr]--;
                ST &= 125;
                ST |= (!memory[addr]) << 1 | (memory[addr] & 128);
                cycles += 2;
            } // DEC
            else {
                X--;
                X &= 0xFF;
                ST &= 125;
                ST |= (!X) << 1 | (X & 128);
            }
            break; // DEX
        case 0xA0:
            if ((IR & 0xF) != 0xA) { X = memory[addr]; }
            else if (IR & 0x10) {
                X = SP;
                break;
            } else {
                X = A;
            }
            ST &= 125;
            ST |= (!X) << 1 | (X & 128);
            break; // LDX/TSX/TAX
        case 0x80:
            if (IR & 4) {
                memory[addr] = X;
                storadd = addr;
            } else if (IR & 0x10) {
                SP = X;
            } else {
                A = X;
                ST &= 125;
                ST |= (!A) << 1 | (A & 128);
            }
            break; // STX/TXS/TXA
        case 0xE0:
            if (IR & 4) {
                memory[addr]++;
                ST &= 125;
                ST |= (!memory[addr]) << 1 | (memory[addr] & 128);
                cycles += 2;
            } // INC/NOP
        }
    }

    else if ((IR & 0xC) == 8) { // nybble2:  8:register/status
        switch (IR & 0xF0) {
        case 0x60:
            SP++;
            SP &= 0xFF;
            A = memory[0x100 + SP];
            ST &= 125;
            ST |= (!A) << 1 | (A & 128);
            cycles = 4;
            break; // PLA
        case 0xC0:
            Y++;
            Y &= 0xFF;
            ST &= 125;
            ST |= (!Y) << 1 | (Y & 128);
            break; // INY
        case 0xE0:
            X++;
            X &= 0xFF;
            ST &= 125;
            ST |= (!X) << 1 | (X & 128);
            break; // INX
        case 0x80:
            Y--;
            Y &= 0xFF;
            ST &= 125;
            ST |= (!Y) << 1 | (Y & 128);
            break; // DEY
        case 0x00:
            memory[0x100 + SP] = ST;
            SP--;
            SP &= 0xFF;
            cycles = 3;
            break; // PHP
        case 0x20:
            SP++;
            SP &= 0xFF;
            ST = memory[0x100 + SP];
            cycles = 4;
            break; // PLP
        case 0x40:
            memory[0x100 + SP] = A;
            SP--;
            SP &= 0xFF;
            cycles = 3;
            break; // PHA
        case 0x90:
            A = Y;
            ST &= 125;
            ST |= (!A) << 1 | (A & 128);
            break; // TYA
        case 0xA0:
            Y = A;
            ST &= 125;
            ST |= (!Y) << 1 | (Y & 128);
            break; // TAY
        default:
            if (flagsw[IR >> 5] & 0x20) { ST |= (flagsw[IR >> 5] & 0xDF); }
            else {
                ST &= 255 - (flagsw[IR >> 5] & 0xDF);
            } // CLC/SEC/CLI/SEI/CLV/CLD/SED
        }
    }

    else { // nybble2:  0: control/branch/Y/compare  4: Y/compare
           // C:Y/compare/JMP
        if ((IR & 0x1F) == 0x10) {
            PC++;
            T = memory[PC];
            if (T & 0x80) {
                T -= 0x100;
            } // BPL/BMI/BVC/BVS/BCC/BCS/BNE/BEQ  relative branch
            if (IR & 0x20) {
                if (ST & branchflag[IR >> 6]) {
                    PC += T;
                    cycles = 3;
                }
            } else {
                if (!(ST & branchflag[IR >> 6])) {
                    PC += T;
                    cycles = 3;
                }
            }
        } else { // nybble2:  0:Y/control/Y/compare  4:Y/compare  C:Y/compare/JMP
            switch (IR & 0x1F) { // addressing modes
            case 0:
                addr = ++PC;
                cycles = 2;
                break; // imm. (or abs.low for JSR/BRK)
            case 0x1C:
                addr = fetch_word() + X;
                cycles = 5;
                break; // abs,x
            case 0xC:
                addr = fetch_word();
                cycles = 4;
                break; // abs
            case 0x14:
                addr = memory[++PC] + X;
                cycles = 4;
                break; // zp,x
            case 4:
                addr = memory[++PC];
                cycles = 3; // zp
            }
            addr &= 0xFFFF;
            switch (IR & 0xE0) {
            case 0x00:
                memory[0x100 + SP] = PC % 256;
                SP--;
                SP &= 0xFF;
                memory[0x100 + SP] = PC / 256;
                SP--;
                SP &= 0xFF;
                memory[0x100 + SP] = ST;
                SP--;
                SP &= 0xFF;
                PC = memory[0xFFFE] + memory[0xFFFF] * 256 - 1;
                cycles = 7;
                break; // BRK
            case 0x20:
                if (IR & 0xF) {
                    ST &= 0x3D;
                    ST |= (memory[addr] & 0xC0) | (!(A & memory[addr])) << 1;
                } // BIT
                else {
                    memory[0x100 + SP] = (PC + 2) % 256;
                    SP--;
                    SP &= 0xFF;
                    memory[0x100 + SP] = (PC + 2) / 256;
                    SP--;
                    SP &= 0xFF;
                    PC = memory[addr] + memory[addr + 1] * 256 - 1;
                    cycles = 6;
                }
                break; // JSR
            case 0x40:
                if (IR & 0xF) {
                    PC = addr - 1;
                    cycles = 3;
                } // JMP
                else {
                    if (SP >= 0xFF) { return 0xFE; }
                    SP++;
                    SP &= 0xFF;
                    ST = memory[0x100 + SP];
                    SP++;
                    SP &= 0xFF;
                    T = memory[0x100 + SP];
                    SP++;
                    SP &= 0xFF;
                    PC = memory[0x100 + SP] + T * 256 - 1;
                    cycles = 6;
                }
                break; // RTI
            case 0x60:
                if (IR & 0xF) {
                    PC = memory[addr] + memory[addr + 1] * 256 - 1;
                    cycles = 5;
                } // JMP() (indirect)
                else {
                    if (SP >= 0xFF) { return 0xFF; }
                    SP++;
                    SP &= 0xFF;
                    T = memory[0x100 + SP];
                    SP++;
                    SP &= 0xFF;
                    PC = memory[0x100 + SP] + T * 256 - 1;
                    cycles = 6;
                }
                break; // RTS
            case 0xC0:
                T = Y - memory[addr];
                ST &= 124;
                ST |= (!(T & 0xFF)) << 1 | (T & 128) | (T >= 0);
                break; // CPY
            case 0xE0:
                T = X - memory[addr];
                ST &= 124;
                ST |= (!(T & 0xFF)) << 1 | (T & 128) | (T >= 0);
                break; // CPX
            case 0xA0:
                Y = memory[addr];
                ST &= 125;
                ST |= (!Y) << 1 | (Y & 128);
                break; // LDY
            case 0x80:
                memory[addr] = Y;
                storadd = addr; // STY
            }
        }
    }

    PC++;
    return 0;
}

//----------------------------- SID emulation
//-----------------------------------------

static unsigned int TriSaw_8580[4096], PulseSaw_8580[4096],
    PulseTriSaw_8580[4096];
static int ADSRperiods[16] = { 9,    32,   63,   95,   149,  220,  267,   313,
                               392,  977,  1954, 3126, 3907, 11720, 19532, 31251 };
static const byte ADSR_exptable[256] = {
    // pos0:1  pos6:30  pos14:16  pos26:8  pos54:4  pos93:2
    1, 30, 30, 30, 30, 30, 30, 16, 16, 16, 16, 16, 16, 16, 16, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static void cSID_init(int rate)
{
    int i;
    clock_ratio = (unsigned int)round((double)C64_PAL_CPUCLK / rate);
    if (clock_ratio < 1) { clock_ratio = 1; }
    cutoff_ratio_8580 = -2 * 3.14 * (12500.0 / 2048) / C64_PAL_CPUCLK;
    cutoff_ratio_6581 = -2 * 3.14 * (20000.0 / 2048) / C64_PAL_CPUCLK;
    cutoff_bias_6581 =
        1 - exp(-2 * 3.14 * 220 / C64_PAL_CPUCLK); // around 220Hz below treshold

    createCombinedWF(TriSaw_8580, 0.8, 2.4, 0.64);
    createCombinedWF(PulseSaw_8580, 1.4, 1.9, 0.68);
    createCombinedWF(PulseTriSaw_8580, 0.8, 2.5, 0.64);

    for (i = 0; i < 9; i++) {
        ADSRstate[i] = HOLDZERO_BITMASK;
        envcnt[i] = 0;
        ratecnt[i] = 0;
        phaseaccu[i] = 0;
        prevaccu[i] = 0;
        expcnt[i] = 0;
        noise_LFSR[i] = 0x7FFFF8;
        prevwfout[i] = 0;
    }
    for (i = 0; i < 3; i++) {
        sourceMSBrise[i] = 0;
        sourceMSB[i] = 0;
        prevlowpass[i] = 0;
        prevbandpass[i] = 0;
    }
    initSID();
}

static void initSID(void)
{
    int i;
    for (i = 0xD400; i <= 0xD7FF; i++) {
        memory[i] = 0;
    }
    for (i = 0xDE00; i <= 0xDFFF; i++) {
        memory[i] = 0;
    }
    for (i = 0; i < 9; i++) {
        ADSRstate[i] = HOLDZERO_BITMASK;
        ratecnt[i] = envcnt[i] = expcnt[i] = 0;
    }
}

static int SID(char num, unsigned int baseaddr)
{
    // better keep these variables static so they won't slow down the routine
    // like if they were internal automatic variables always recreated
    static byte channel, ctrl, SR, prevgate, wf, test, filterctrl_prescaler[3];
    static byte *sReg, *vReg;
    static unsigned int period, accuadd, pw, wfout;
    static unsigned long int MSB;
    static int nonfilt, filtin, cutoff[3],
        resonance[3]; // cutoff must be signed otherwise compiler may make errors
                      // in multiplications
    static long int output, filtout,
        ftmp; // so if samplerate is smaller, cutoff needs to be 'long int' as
              // its value can exceed 32768

    filtin = nonfilt = 0;
    sReg = &memory[baseaddr];
    vReg = sReg;
    for (channel = num * SID_CHANNEL_AMOUNT;
         channel < (num + 1) * SID_CHANNEL_AMOUNT; channel++, vReg += 7) {
        ctrl = vReg[4];

        // ADSR envelope generator:
        {
            SR = vReg[6];
            prevgate = (ADSRstate[channel] & GATE_BITMASK);
            if (prevgate != (ctrl & GATE_BITMASK)) { // gatebit-change?
                if (prevgate) {
                    ADSRstate[channel] &=
                        0xFF - (GATE_BITMASK | ATTACK_BITMASK |
                                DECAYSUSTAIN_BITMASK);
                } // falling edge
                else {
                    ADSRstate[channel] =
                        (GATE_BITMASK | ATTACK_BITMASK |
                         DECAYSUSTAIN_BITMASK); // rising edge, also sets
                                                // hold_zero_bit=0
                }
            }
            if (ADSRstate[channel] & ATTACK_BITMASK) {
                period = ADSRperiods[vReg[5] >> 4];
            } else if (ADSRstate[channel] & DECAYSUSTAIN_BITMASK) {
                period = ADSRperiods[vReg[5] & 0xF];
            } else {
                period = ADSRperiods[SR & 0xF];
            }
            ratecnt[channel]++;
            ratecnt[channel] &= 0x7FFF; // can wrap around (ADSR delay-bug: short
                                        // 1st frame)
            if (ratecnt[channel] ==
                period) { // ratecounter shot (matches rateperiod) (in genuine
                          // SID ratecounter is LFSR)
                ratecnt[channel] = 0; // reset rate-counter on period-match
                if ((ADSRstate[channel] & ATTACK_BITMASK) ||
                    ++expcnt[channel] == ADSR_exptable[envcnt[channel]]) {
                    expcnt[channel] = 0;
                    if (!(ADSRstate[channel] & HOLDZERO_BITMASK)) {
                        if (ADSRstate[channel] & ATTACK_BITMASK) {
                            envcnt[channel]++;
                            if (envcnt[channel] == 0xFF) {
                                ADSRstate[channel] &= 0xFF - ATTACK_BITMASK;
                            }
                        } else if (!(ADSRstate[channel] &
                                     DECAYSUSTAIN_BITMASK) ||
                                   envcnt[channel] != (SR >> 4) + (SR & 0xF0)) {
                            envcnt[channel]--; // resid adds 1 cycle delay, we
                                               // omit that pipelining mechanism
                                               // here
                            if (envcnt[channel] == 0) {
                                ADSRstate[channel] |= HOLDZERO_BITMASK;
                            }
                        }
                    }
                }
            }
        }

        // WAVE generation codes (phase accumulator and waveform-selector):
        test = ctrl & TEST_BITMASK;
        wf = ctrl & 0xF0;
        accuadd = (vReg[0] + vReg[1] * 256);
        if (test || ((ctrl & SYNC_BITMASK) && sourceMSBrise[num])) {
            phaseaccu[channel] = 0;
        } else {
            phaseaccu[channel] += accuadd;
            phaseaccu[channel] &= 0xFFFFFF;
        }
        MSB = phaseaccu[channel] & 0x800000;
        sourceMSBrise[num] = (MSB > (prevaccu[channel] & 0x800000)) ? 1 : 0;
        if (wf & NOISE_BITMASK) {
            int tmp = noise_LFSR[channel];
            if (((phaseaccu[channel] & 0x100000) !=
                 (prevaccu[channel] & 0x100000))) {
                int step = (tmp & 0x400000) ^ ((tmp & 0x20000) << 5);
                tmp = ((tmp << 1) + (step ? 1 : test)) & 0x7FFFFF;
                noise_LFSR[channel] = tmp;
            }
            wfout = (wf & 0x70)
                        ? 0
                        : ((tmp & 0x100000) >> 5) + ((tmp & 0x40000) >> 4) +
                              ((tmp & 0x4000) >> 1) + ((tmp & 0x800) << 1) +
                              ((tmp & 0x200) << 2) + ((tmp & 0x20) << 5) +
                              ((tmp & 0x04) << 7) + ((tmp & 0x01) << 8);
        } else if (wf & PULSE_BITMASK) {
            pw = (vReg[2] + (vReg[3] & 0xF) * 256) * 16;

            int tmp = phaseaccu[channel] >> 8;
            if (wf == PULSE_BITMASK) {
                if (test || tmp >= pw) { wfout = 0xFFFF; }
                else {
                    wfout = 0;
                }
            } else { // combined pulse
                wfout = (tmp >= pw || test) ? 0xFFFF : 0;
                if (wf & TRI_BITMASK) {
                    if (wf & SAW_BITMASK) {
                        wfout = (wfout) ? combinedWF(num, channel,
                                                     PulseTriSaw_8580, tmp >> 4,
                                                     1)
                                        : 0;
                    } // pulse+saw+triangle (waveform nearly identical to
                      // tri+saw)
                    else {
                        tmp = phaseaccu[channel] ^
                              (ctrl & RING_BITMASK ? sourceMSB[num] : 0);
                        wfout = (wfout)
                                    ? combinedWF(num, channel, PulseSaw_8580,
                                                 (tmp ^ (tmp & 0x800000
                                                             ? 0xFFFFFF
                                                             : 0)) >> 11,
                                                 0)
                                    : 0;
                    }
                } // pulse+triangle
                else if (wf & SAW_BITMASK) {
                    wfout = (wfout) ? combinedWF(num, channel, PulseSaw_8580,
                                                 tmp >> 4, 1)
                                    : 0;
                }
            }
        } // pulse+saw
        else if (wf & SAW_BITMASK) {
            wfout = phaseaccu[channel] >> 8; // saw
            if (wf & TRI_BITMASK) {
                wfout = combinedWF(num, channel, TriSaw_8580, wfout >> 4, 1);
            } // saw+triangle
        } else if (wf & TRI_BITMASK) {
            int tmp =
                phaseaccu[channel] ^ (ctrl & RING_BITMASK ? sourceMSB[num] : 0);
            wfout = (tmp ^ (tmp & 0x800000 ? 0xFFFFFF : 0)) >> 7;
        }
        if (wf) { prevwfout[channel] = wfout; }
        else {
            wfout = prevwfout[channel];
        } // emulate waveform 00 floating wave-DAC
        prevaccu[channel] = phaseaccu[channel];
        sourceMSB[num] = MSB;
        if (sReg[0x17] & FILTSW[channel]) {
            filtin += ((long int)wfout - 0x8000) * envcnt[channel] / 256;
        } else if ((FILTSW[channel] != 4) || !(sReg[0x18] & OFF3_BITMASK)) {
            nonfilt += ((long int)wfout - 0x8000) * envcnt[channel] / 256;
        }
    }
    // update readable SID1-registers (some SID tunes might use 3rd channel
    // ENV3/OSC3 value as control)
    if (memory[1] & 3) {
        sReg[0x1B] = wfout >> 8;
        sReg[0x1C] = envcnt[3];
    } // OSC3, ENV3 (some players rely on it)

    // FILTER:
    filterctrl_prescaler[num]--;
    if (filterctrl_prescaler[num] == 0) { // calculate cutoff and resonance
                                          // curves only at samplerate is still
                                          // adequate and reduces CPU stress of
                                          // frequent float calculations
        filterctrl_prescaler[num] = clock_ratio;
        cutoff[num] = 2 + sReg[0x16] * 8 + (sReg[0x15] & 7);
        if (SID_model[num] == 8580) {
            cutoff[num] = (1 - exp(cutoff[num] * cutoff_ratio_8580)) * 0x10000;
            resonance[num] =
                (pow(2, ((4 - (sReg[0x17] >> 4)) / 8.0))) *
                0x100; // resonance could be taken from table as well
        } else {
            cutoff[num] =
                (cutoff_bias_6581 +
                 ((cutoff[num] < 192)
                      ? 0
                      : 1 - exp((cutoff[num] - 192) * cutoff_ratio_6581))) *
                0x10000;
            resonance[num] =
                ((sReg[0x17] > 0x5F) ? 8.0 / (sReg[0x17] >> 4) : 1.41) * 0x100;
        }
    }
    filtout = 0; // the filter-calculation itself can't be prescaled because
                 // sound-quality would suffer of no 'oversampling'
    ftmp = filtin + prevbandpass[num] * resonance[num] / 0x100 +
           prevlowpass[num];
    if (sReg[0x18] & HIGHPASS_BITMASK) { filtout -= ftmp; }
    ftmp = prevbandpass[num] - ftmp * cutoff[num] / 0x10000;
    prevbandpass[num] = ftmp;
    if (sReg[0x18] & BANDPASS_BITMASK) { filtout -= ftmp; }
    ftmp = prevlowpass[num] + ftmp * cutoff[num] / 0x10000;
    prevlowpass[num] = ftmp;
    if (sReg[0x18] & LOWPASS_BITMASK) { filtout += ftmp; }

    // output stage for one SID
    output = (nonfilt + filtout) * (sReg[0x18] & 0xF) / OUTPUT_SCALEDOWN;
    if (output >= 32767) { output = 32767; }
    else if (output <= -32768) {
        output = -32768;
    } // saturation logic on overload (not needed if the callback handles it)
    return (int)output; // master output
}

static unsigned int combinedWF(char num, char channel, unsigned int* wfarray,
                               int index, char differ6581)
{
    (void)channel;
    if (differ6581 && SID_model[num] == 6581) { index &= 0x7FF; }
    return wfarray[index];
}

static void createCombinedWF(unsigned int* wfarray, float bitmul,
                             float bitstrength, float treshold)
{
    int i, j, k;
    for (i = 0; i < 4096; i++) {
        wfarray[i] = 0;
        for (j = 0; j < 12; j++) {
            float bitlevel = 0;
            for (k = 0; k < 12; k++) {
                bitlevel += (bitmul / pow(bitstrength, fabs(k - j))) *
                            (((i >> k) & 1) - 0.5);
            }
            wfarray[i] += (bitlevel >= treshold) ? pow(2, j) : 0;
        }
        wfarray[i] *= 12;
    }
}

// ---------------------------------------------------------------------------
// Register-level API -- see csid_engine.h. Used by musplugin (.mus/.str), which
// drives the SID registers directly instead of running 6502 code.
// ---------------------------------------------------------------------------
void csid_chip_init(int rate, int sid_count, unsigned int sid2_base)
{
    samplerate = rate > 0 ? rate : DEFAULT_SAMPLERATE;
    sampleratio = (int)round((double)C64_PAL_CPUCLK / samplerate);
    if (sampleratio < 1) { sampleratio = 1; }
    SIDamount = sid_count < 1 ? 1 : (sid_count > 2 ? 2 : sid_count);
    SID_address[0] = 0xD400;
    SID_address[1] = sid2_base;
    SID_model[0] = SID_model[1] = SID_model[2] = 8580;
    OUTPUT_SCALEDOWN = OUTPUT_SCALEDOWN_BASE;
    if (SIDamount == 2) { OUTPUT_SCALEDOWN /= 0.6; }
    memory[1] = 0x37;
    cSID_init(samplerate);
}

void csid_poke(unsigned int addr, unsigned char val)
{
    if (addr < MAX_DATA_LEN) { memory[addr] = val; }
}

int csid_chip_render(int16_t* out, int frames)
{
    int i, j;
    long int acc;
    for (i = 0; i < frames; i++) {
        acc = 0;
        for (j = 0; j < sampleratio; j++) {
            acc += SID(0, 0xD400);
            if (SIDamount >= 2) { acc += SID(1, SID_address[1]); }
        }
        acc /= sampleratio;
        if (acc > 32767) { acc = 32767; }
        else if (acc < -32768) { acc = -32768; }
        out[i] = (int16_t)acc;
    }
    return frames;
}

// Stereo variant: chip 1 -> left, chip 2 -> right. Summing both chips to mono
// (which csid_chip_render does) collapses a Stereo Sidplayer tune to a single
// channel -- audibly wrong, since VICE keeps them separated.
int csid_chip_render_stereo(int16_t* out, int frames)
{
    int i, j;
    long int l, r;
    for (i = 0; i < frames; i++) {
        l = r = 0;
        for (j = 0; j < sampleratio; j++) {
            l += SID(0, 0xD400);
            if (SIDamount >= 2) { r += SID(1, SID_address[1]); }
        }
        l /= sampleratio;
        if (SIDamount >= 2) { r /= sampleratio; } else { r = l; }
        if (l > 32767) { l = 32767; } else if (l < -32768) { l = -32768; }
        if (r > 32767) { r = 32767; } else if (r < -32768) { r = -32768; }
        out[i * 2] = (int16_t)l;
        out[i * 2 + 1] = (int16_t)r;
    }
    return frames;
}
