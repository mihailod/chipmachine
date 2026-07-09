/*
 * ned_engine.c -- headless NerdTracker II (.ned) playback engine.
 *
 * This is the player/loader core of "NerdTracker 2 SDL port" v1.0 by thefox
 * <thefox@aspekt.fi> (http://thefox.aspekt.fi/nt2-sdl-1.0.zip), which is itself
 * a port of NerdTracker II by Michel Iwaniec ("Bananmos").  Everything here is
 * lifted verbatim from that port's nt2.c -- only the MS-DOS/SDL editor UI, the
 * Win32 glue and main() were dropped, and a small C integration API was added
 * at the bottom.  The 2A03 APU itself is emulated by blargg's Nes_Snd_Emu
 * (LGPL); see ned/Nes_Snd_Emu-0.1.7/.
 *
 * Register writes go out through Write2SoundReg()/ReadSoundReg() in apuwrap.c,
 * which forward to the Simple_Apu (PAL) wrapper in blarggAPU.cpp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "typedefs.h"
#include "nt2.h"
#include "dephs.h"
#include "apuwrap.h"
#include "file.h"

/* SDLKey is only referenced by a couple of (unused) editor globals below. */
typedef int SDLKey;

/* Provided by apuwrap.c / blarggAPU.cpp (the APU side of the port). */
extern void Write2SoundReg(int RegAddr, int Val);
extern int  ReadSoundReg(int RegAddr);
extern void (*CallInsideMixer)(void);
extern int  NesessInitialize(void);
extern int  NesessShutdown(void);
extern void Mix(void *Buffer, int Length, int Is16Bit);
extern int  VBlankSpeed, PlayBackRate;

/* Tempo stored in the module header; captured by the loaders (see below) and
 * applied by ned_engine_start(). The port's editor never copied this into
 * curr_speed on load, so we do it here. */
static int g_ned_init_tempo = 6;

/* Forward declarations (the editor's nt2.c declared these up top; we only keep
 * the ones the replay/loader path actually calls). */
void  PlayNote(int _Chn, int Inst, int Note, int Oct);
void  ResetEditors(void);
int   LoadNed20(char *FileName);
int   LoadNed10(char *FileName);
void  ConvertNed10Pat2ChnPat(BYTE *Ned10Pat, ChannelPattern *P_A, ChannelPattern *P_B, ChannelPattern *P_C, ChannelPattern *P_D);
void  ReadSampleChunk(void *SampleChunkData, InfoHeader *Info);
DWORD UnPackPattern20(BYTE *PackedData, ChannelPattern *NT2_Pattern, int Chn);
DWORD UnPackInstrument20(NedInstStrucNew *Src, struct inst_struc *Dest);
DWORD UnPackInstrument10(struct ned_inst_struc *Src, struct inst_struc *Dest);
DWORD UnPackInstrumentDPCM20(BYTE *Src, InstDPCM *TheInst);
DWORD UnPackOrderEntry20(BYTE *Src, BYTE *Dest);
void  ClearPattern(ChannelPattern *Pat);
int   PatternIsEmpty(ChannelPattern *Pat);
void  ClearOrderEntry(int Entry);
void  ClearOrder2Max(int StartEntry);

/* nt2.c effect-translation tables (originally just past the block below). */
char    eph2nedeph[16]={0, 4, 5, 6, 0, 0, 0, 7, 0, 0, 0, 0, 1, 3, 0, 2};
char    nedeph2eph[8]={0, 0xC, 15, 0xD, 0x1, 0x2, 0x3, 7};

/* ===================================================================== */
/* == verbatim from nt2.c: module-level data, tables and player state == */
/* ===================================================================== */
struct  ned_header_struc ned_header;

struct  inst_struc      inst[MAX_INSTRUMENTS];

InstDPCM        SampleInst[MAX_INSTRUMENTS_DPCM];

ChannelPattern  *ChnPattern[MAX_CHANNELS]={NULL, NULL, NULL, NULL, NULL};
//PatternDPCM     *SamplePattern=NULL;

char    InstName[MAX_INSTRUMENTS_DPCM][DPCM_INAME_LENGTH];
char    SampleName[MAX_INSTRUMENTS_DPCM][MAX_DPCM_SAMPLES][DPCM_SNAME_LENGTH];

BYTE    NoteTable[MAX_INSTRUMENTS_DPCM][MAX_DPCM_OCTAVES][12];

float   note_phreq[8][13];

/* int     Note2PeriodTab[96]={ 1614, 1523, 1438, 1357, 1281, 1209, 1141, 1077, 1016, 959, 905, 855,
                                807, 761, 719, 678, 640, 604, 570, 538, 508, 479, 452, 427,
                                403, 380, 359, 339, 320, 302, 285, 269, 254, 239, 226, 213, 
                                201, 190, 179, 169, 160, 151, 142, 134, 127, 119, 113, 106, 
                                100, 95, 89, 84, 80, 75, 71, 67, 63, 59, 56, 53,
                                50, 47, 44, 42, 40, 37, 35, 33, 31, 29, 28, 26, 
                                25, 23, 22, 21, 20, 18, 17, 16, 15, 14, 14, 13, 
                                12, 11, 11, 10, 10, 9, 8, 8, 7, 7, 7, 6};*/
                            
/*int     Note2PeriodTab[108]={ 2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047, 2034,1920,1812,
                                1710, 1614, 1524, 1438, 1358, 1281, 1209, 1142, 1077, 1017, 960, 906,
                                855,  807,  762,  719,  679,  641,  605,  571,  539,  509,  480, 453,
                                428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240, 227,
                                214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120, 113,
                                107,  101,  95,   90,   85,   80,   76,   71,   67,   64,   60,  57,
                                53,   50,   48,   45,   42,   40,   38,   36,   34,   32,   30,  28,
                                27,   25,   24,   22,   21,   20,   19,   18,   17,   16,   15,  14,
                                13,   13,   12,   11,   11,   10,   9,    9,    8,    8,    7,   7};*/

int     Note2PeriodTab[108]={   3420, 3228, 3048, 2876, 2716, 2607, 2418, 2284, 2047, 2034,1920,1812,
                                1710, 1614, 1524, 1438, 1358, 1281, 1209, 1142, 1077, 1017, 960, 906,
                                855,  807,  762,  719,  679,  641,  605,  571,  539,  509,  480, 453,
                                428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240, 227,
                                214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120, 113,
                                107,  101,  95,   90,   85,   80,   76,   71,   67,   64,   60,  57,
                                53,   50,   48,   45,   42,   40,   38,   36,   34,   32,   30,  28,
                                27,   25,   24,   22,   21,   20,   19,   18,   17,   16,   15,  14,
                                13,   13,   12,   11,   11,   10,   9,    9,    8,    8,    7,   7};

sBYTE   LCounterTable[32]={5, 127, 10, 1, 20, 2, 40, 3,
                           80, 4, 30, 5, 7, 6, 13, 7,
                           6, 8, 12, 9, 24, 10, 48, 11,
                           96, 12, 36, 13, 8, 14, 16, 15};


char    lnote[13]={0, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3};

/* char    *note_char[14]={"--", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "C-", "OF"};*/
char    *note_char[14]={"--", "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "OF"};

char    BuggyNote2Normal[12]={1,2,3,4,5,6,7,8,9,10,11,0};

char    real_colpos[NUMBER_OPH_COLS]={0, 2, 4, 5, 7, 8, 9};

#define NUMBER_OPH_WARNS        9

char    *exit_warns[NUMBER_OPH_WARNS]= {
        "Noo! DON'T EXIT! I promise the next version will be better!",
        "Hello, nerd!",
        "Don't go! The NES-scene needs you! Err.. it does exist? Right?..",
        "The Real NERDS 'R' US rule? [y/n]",
        "REALLY exit NT2? [(N)o/(N)o/(M)aybe later/(O)ops]",
        "This program has caused an illegal operation and will be terminated? [y/n]",
        "The document could not be registered. Perhaps it is already open? [y/n]",
        "Out of memory. Please close some applications before trying again.",
        "How do we print a bluescreen???"};


BYTE    Reg4015=0;
long    curr_orderpos=0;
long    curr_row=0;
long    curr_chn=0;
long    curr_chncol=0;
long    curr_pat=0;
long    curr_octave=0;
long    curr_inst=1;
int     CurrSampleInst=1;
int     CurrSample=0;
long    edit_state=EDIT_PAT;
int     Editor=PATTERN_EDITOR;
int     SampleEditorState=EDIT_SAMPLE_NAMES;
long    curr_speed=6;
long    order_tot=1;
int     curr_instrow=0;
int     curr_order=0;
int     RestartPosition=0;

int     CurrOrderChn=0;
int     CurrSampleSetting=0;
int     InameWindowLine=0;
int     FirstIname=0;

int     CurrNoteTableNote=0;
int     CurrNoteTableOctave=0;
int     NoteTableSmpOrFreq=0;

long    nt_sound_delay=0;
unsigned short  kb_shipht_state;


boolean done=0;

SDLKey    key_pressed;


const int SineWave[32]={0,   24, 49, 74, 97,120,141,161,
                        180,197,212,224,235,244,250,253,
                        255,253,250,244,235,224,212,197,
                        180,161,141,120, 97, 74, 49, 24};


char    hex_tab[17]="0123456789ABCDEF";
char    *boo_tab[2]={"OFF", "ON!"};


void    nt_UpdatePosInstNames(void);
struct  ned_inst_struc ned_inst[31];

signed char    order_list[128];
BYTE    OrderList[NT_MAX_ORDER][MAX_CHANNELS];

unsigned short pat_ophph[MAX_PATTERNS];

char    *ned_phile=NULL;

// thefox: size changed from 20 to 256
char    ned_philename[256];

#define PERIOD_LIMIT    10

typedef struct {
int     Period;
int     LastTone;
int     LastInst;
int     Tone;
int     Inst;
int     Effect;              
int     EffParm;
int     PortaTone;
int     PortaSpeed;
int     AutoPorta;
// int     VolumeFlag;
// int     PeriodFlag;
int     Reg0;
int     Reg1;
int     Reg3;
int     Volume;
int     VolumeSlide;
int     AutoVolumeSlide;
int     Arpeggio;
int     AutoArpX;
int     AutoArpY;
int     AutoArpZ;
int     VibratoPos;
int     AutoVibratoSpeed;
int     AutoVibratoDepth;
int     VibratoSpeed;
int     VibratoDepth;
int     TremoloPos;
int     AutoTremoloSpeed;
int     AutoTremoloDepth;
int     TremoloSpeed;
int     TremoloDepth;
int     LoopedNoise;
int     VblanksLeft;
int     BigTick;
int     ReversedArpeggio;
int     NonLoopedArpeggio;
} SChn;

SChn    Chn[MAX_CHANNELS];


int     CurrTick=0;
//long    BigTick=0;
int     IsPlaying=0;
int     EmulateNTSC=0;
int     PalCounter=1;

// added for SetPeriod --thefox
int OldPeriodHi[4] = {-1,-1,-1,-1};

/* ============ verbatim from nt2.c: the replay engine ============ */
void    ResetPlayer(void)
        {
        memset(&Chn[0], 0, sizeof(SChn)*MAX_CHANNELS);

        // thefox added this (and it didn't help...)
        OldPeriodHi[0] =
            OldPeriodHi[1] = OldPeriodHi[2] = OldPeriodHi[3] = -1;
        }

void    GetSNote(int C)
        {
        int     Pat;
        struct snote_struc *SNote;
        Pat=OrderList[curr_order][C];
        SNote=&(ChnPattern[C][Pat].Row[curr_row]);

        if((*SNote).note==NOTE_OPHPH)
          Chn[C].Tone=97;
        else
          Chn[C].Tone=((*SNote).octave<<3)+((*SNote).octave<<2)+(*SNote).note;
        if((C==NOSWAV_CHN) && Chn[C].Tone)
          Chn[C].Tone=((15-(Chn[C].Tone-1)) & 0xF)+1;
        Chn[C].Inst=((*SNote).inst_num1<<4)+(*SNote).inst_num2;
        Chn[C].Effect=(*SNote).ephphect;
        Chn[C].EffParm=((*SNote).e_parm1<<4)+(*SNote).e_parm2;
        }



#define tEFF_PORTAUP    0x1
#define tEFF_PORTADOWN  0x2
#define tEFF_TONEPORTA  0x3
#define tEFF_VIBRATO    0x4
#define tEFF_TREMOLO    0x7
#define tEFF_ARPEGGIO   0x8
#define tEFF_VOLSLIDE   0xA
#define tEFF_PATTERNJMP 0xB
#define tEFF_MODVOLUME  0xC
#define tEFF_PATTERNBRK 0xD
#define tEFF_SETSPEED   0xF



void    SetVolume(int C, int Volume)
        {
        if(C!=TRIWAV_CHN) {
          if(Volume<0) Volume=0;
          if(Volume>63) Volume=63;
          Write2SoundReg(0x4000+C*4, (Chn[C].Reg0 | (Volume>>2)));
          }
        }

void    SetPeriod(int C, int Period, int TimeLength)
        {
            int PeriodHi;

            // QUICK DIRTY FIX
            // because 4015 channel enable needs to be set before setting
            // some other stuff, or the first note in the channel won't
            // be played! --thefox
        if(!(ReadSoundReg(0x4015) & 0x10))
            Reg4015&=0xF;
        Write2SoundReg(0x4015, Reg4015);

        if(C==NOSWAV_CHN) {
          Write2SoundReg(0x400E, (Period & 0xF)+(Chn[C].LoopedNoise<<7));
          Write2SoundReg(0x400F, 8);
          }
        else {
          if(Period<0)
            Period=0;
          if(Period>=2048)
            Period=2047;
          Write2SoundReg(0x4002+C*4, (Period & 0xFF));

/*
From nt2 6502 player source... let's do the same thing here
lda     #$08    ; Compensates for the bug Matt found
sta     $4001,Y
*/
          if(C==SQRWAV1_CHN||C==SQRWAV2_CHN)
              Write2SoundReg(0x4001+C*4, 8);

          // we don't write the top 3 bits of frequency unless they've
          // changed, because the write will restart the sequencer
          // and envelope count --thefox
          PeriodHi = (Period >> 8);
          if(PeriodHi != OldPeriodHi[C])
          {
             Write2SoundReg(0x4003+C*4, PeriodHi|8);
             OldPeriodHi[C] = PeriodHi;
          }
          }
        }


void    DoVibrato(int C, int Speed, int Depth)
        {
        int     SinePos;
        int     Delta;
        int     Period;

        if(Chn[C].VibratoPos>=0)
          SinePos=Chn[C].VibratoPos & 31;
        else
          SinePos=(~Chn[C].VibratoPos) & 31;
        Delta=(SineWave[SinePos]*Depth)>>7;

        if(Chn[C].VibratoPos>=0)
          Period=Chn[C].Period+Delta;
        else
          Period=Chn[C].Period-Delta;

        SetPeriod(C, Period, 8);
        // Chn[C].PeriodFlag=0;

        Chn[C].VibratoPos+=Speed;
        if(Chn[C].VibratoPos>=32) Chn[C].VibratoPos-=64;
        }


void    DoTremolo(int C, int Speed, int Depth)
        {
        int     SinePos;
        int     Delta;
        int     Volume;

        if(Chn[C].TremoloPos>=0)
          SinePos=Chn[C].TremoloPos & 31;
        else
          SinePos=(~Chn[C].TremoloPos) & 31;
        Delta=(SineWave[SinePos]*Depth)>>6;

        if(Chn[C].TremoloPos>=0)
          Volume=Chn[C].Volume+Delta;
        else
          Volume=Chn[C].Volume-Delta;

        SetVolume(C, Volume);

        Chn[C].TremoloPos+=Speed;
        if(Chn[C].TremoloPos>=32) Chn[C].TremoloPos-=64;
        }


void    DoEffsT0(int C)
        {
        // Portamento
        if((Chn[C].Effect==tEFF_PORTAUP) || (Chn[C].Effect==tEFF_PORTADOWN)) {
          if(Chn[C].EffParm)
            Chn[C].PortaSpeed=Chn[C].EffParm;
          }
        // Tone Portamento
        if(Chn[C].Effect==tEFF_TONEPORTA) {
          if(Chn[C].EffParm)
            Chn[C].PortaSpeed=Chn[C].EffParm;
          if(Chn[C].Tone)
            Chn[C].PortaTone=Chn[C].Tone;
          }
        // Vibrato
        if(Chn[C].Effect==tEFF_VIBRATO)
          if(Chn[C].EffParm) {
            Chn[C].VibratoSpeed=(Chn[C].EffParm>>4) & 0xF;
            Chn[C].VibratoDepth=Chn[C].EffParm & 0xF;
            }
          


        // Set Volume
        if(Chn[C].Effect==tEFF_MODVOLUME) {
            Chn[C].Volume=Chn[C].EffParm & 0x3F;
          }
        // Volume Slide
        if(Chn[C].Effect==tEFF_VOLSLIDE)
          if(Chn[C].EffParm) {
            Chn[C].VolumeSlide=Chn[C].EffParm;
            }
        // Tremolo
        if(Chn[C].Effect==tEFF_TREMOLO)
          if(Chn[C].EffParm) {
            Chn[C].TremoloSpeed=(Chn[C].EffParm>>4) & 0xF;
            Chn[C].TremoloDepth=Chn[C].EffParm & 0xF;
            }
        // Arpeggio
        if(Chn[C].Effect==tEFF_ARPEGGIO) {
          if(Chn[C].EffParm)
            Chn[C].Arpeggio=Chn[C].EffParm;
          //Chn[C].PeriodFlag=1;
          Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone-1)%96];
          SetPeriod(C, Chn[C].Period, 8);
          goto SkipPeriodSet;
          }
        else {
          if(Chn[C].LastInst) { // Do AutoArpeggio
            if((Chn[C].BigTick<4) || (!Chn[C].NonLoopedArpeggio)) {
                int   ArpX=Chn[C].AutoArpX,
                      ArpY=Chn[C].AutoArpY,
                      ArpZ=Chn[C].AutoArpZ;

                if(ArpX || ArpY || ArpZ) {
                        //Chn[C].PeriodFlag=1;
                        if((Chn[C].BigTick & 3)==0)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone-1)%96];
                        if((Chn[C].BigTick & 3)==1)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpX-1)%96];
                        if((Chn[C].BigTick & 3)==2)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpY-1)%96];
                        if((Chn[C].BigTick & 3)==3)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpZ-1)%96];
                        SetPeriod(C, Chn[C].Period, 8);
                        goto SkipPeriodSet;

                        }
                
                }
              }
          }


if((Chn[C].Tone) && (Chn[C].Effect!=tEFF_TONEPORTA)){
    // maybe this works ?
    //OldPeriodHi[C] = -1;

        SetPeriod(C, Chn[C].Period, 8);
}
SkipPeriodSet:

        // Set speed
        if(Chn[C].Effect==tEFF_SETSPEED)
          curr_speed=Chn[C].EffParm;

        // Pattern Break
        if(Chn[C].Effect==tEFF_PATTERNBRK) {
          curr_row=(Chn[C].EffParm-1);
          if((++curr_order)>=order_tot)
            curr_order=RestartPosition;
          }

        }
         
void    DoEffsTx(int C)
        {


          if(!(Chn[C].VblanksLeft & 0x80)) {
            if((--Chn[C].VblanksLeft)<0) {
              Chn[C].VblanksLeft=0;
              Reg4015&=(~(1<<C));
              }
            }


        // Volume Slide (has priority over Auto Volume Slide)
        if((Chn[C].Effect==tEFF_VOLSLIDE)) {
            Chn[C].Volume+=(Chn[C].VolumeSlide>>4) & 0xF;
            Chn[C].Volume-=Chn[C].VolumeSlide & 0xF;
          }
        // AutoVolumeSlide
        else {
          if(Chn[C].AutoVolumeSlide!=0) {
            Chn[C].Volume+=Chn[C].AutoVolumeSlide;
            }
          }
        // Tremolo (has priority over AutoTremolo)
        if(Chn[C].Effect==tEFF_TREMOLO) {
          DoTremolo(C, Chn[C].TremoloSpeed, Chn[C].TremoloDepth);
          goto SkipVolumeSet;
          }
        // AutoTremolo
        else {
          if((Chn[C].AutoTremoloSpeed) || (Chn[C].AutoTremoloDepth)) {
            DoTremolo(C, Chn[C].AutoTremoloSpeed, Chn[C].AutoTremoloDepth);
            goto SkipVolumeSet;
            }
          }


        if(Chn[C].Volume<0) Chn[C].Volume=0;
        if(Chn[C].Volume>63) Chn[C].Volume=63;

          SetVolume(C, Chn[C].Volume);


SkipVolumeSet:

        // Arpeggio (has priority over AutoArpeggio)
        if(Chn[C].Effect==tEFF_ARPEGGIO) {
          int   ParmX=(Chn[C].Arpeggio>>4) & 0xF;
          int   ParmY=Chn[C].Arpeggio & 0xF;
          // Chn[C].PeriodFlag=1;

          if((CurrTick%3)==0)
                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone-1)%96];
          if((CurrTick%3)==1)
                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ParmX-1)%96];
          if((CurrTick%3)==2)
                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ParmY-1)%96];
          }
        else {
          if(Chn[C].LastInst) { // Do AutoArpeggio
            if((Chn[C].BigTick<4) || (!Chn[C].NonLoopedArpeggio)) {
                int   ArpX=Chn[C].AutoArpX,
                      ArpY=Chn[C].AutoArpY,
                      ArpZ=Chn[C].AutoArpZ;

                if(ArpX || ArpY || ArpZ) {
                        // Chn[C].PeriodFlag=1;
                        if((Chn[C].BigTick & 3)==0)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone-1)%96];
                        if((Chn[C].BigTick & 3)==1)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpX-1)%96];
                        if((Chn[C].BigTick & 3)==2)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpY-1)%96];
                        if((Chn[C].BigTick & 3)==3)
                                Chn[C].Period=Note2PeriodTab[(Chn[C].LastTone+ArpZ-1)%96];
                        }
              }
                }
          }
        // Portamento Up
        if(Chn[C].Effect==tEFF_PORTAUP) {
          // Chn[C].PeriodFlag=1;
          Chn[C].Period-=Chn[C].PortaSpeed;
          goto PortaDone;
          }
        // Portamento Down
        if(Chn[C].Effect==tEFF_PORTADOWN) {
          // Chn[C].PeriodFlag=1;
          Chn[C].Period+=Chn[C].PortaSpeed;
          goto PortaDone;
          }
        // Tone Portamento
        if(Chn[C].Effect==tEFF_TONEPORTA) {
          int   PTonePeriod=Note2PeriodTab[Chn[C].PortaTone-1];

          if(Chn[C].Period<PTonePeriod) {
            Chn[C].Period+=Chn[C].PortaSpeed;
            if(Chn[C].Period>PTonePeriod)
              Chn[C].Period=PTonePeriod;
            // Chn[C].PeriodFlag=1;
            }
          if(Chn[C].Period>PTonePeriod) {
            Chn[C].Period-=Chn[C].PortaSpeed;
            if(Chn[C].Period<PTonePeriod)
              Chn[C].Period=PTonePeriod;
            // Chn[C].PeriodFlag=1;
            }
          goto PortaDone;
          }

        // Auto Portamento
        if(Chn[C].AutoPorta!=0) {
          // Chn[C].PeriodFlag=1;
          Chn[C].Period+=Chn[C].AutoPorta;
          }

PortaDone:

        // Vibrato (has priority over AutoVibrato)
        if(Chn[C].Effect==tEFF_VIBRATO) {
          DoVibrato(C, Chn[C].VibratoSpeed, Chn[C].VibratoDepth);
          goto SkipPeriodSet;
          }
        // Autovibrato
        else {
          if((Chn[C].AutoVibratoSpeed) || (Chn[C].AutoVibratoDepth)) {
            DoVibrato(C, Chn[C].AutoVibratoSpeed, Chn[C].AutoVibratoDepth);
            goto SkipPeriodSet;
            }
          }

          //if(C==NOSWAV_CHN)
          //  SetPeriod(C, Chn[C].Period, 8);
          //else
          //  if(Chn[C].PeriodFlag)
              SetPeriod(C, Chn[C].Period, 8);

SkipPeriodSet:

          if(Chn[C].VblanksLeft!=0) {
            if(C==TRIWAV_CHN)
              Write2SoundReg(0x4008, 0xFF);
            }
        }


void    UpdateT0(void)
        {
// thefox: j, k unreferenced
        int i;//, j, k;

        for(i=0; i<MAX_CHANNELS; i++)
          GetSNote(i);

        for(i=0; i<MAX_A_CHANNELS; i++) {

          if(Chn[i].Inst) {
            Chn[i].LastInst=Chn[i].Inst;

            if(inst[Chn[i].LastInst-1].hold_note)
              Chn[i].VblanksLeft=0x80;
            else
              Chn[i].VblanksLeft=inst[Chn[i].LastInst-1].active_tlength;

            Chn[i].Reg0=(inst[Chn[i].LastInst-1].duty_cycle<<6)+(inst[Chn[i].LastInst-1].hold_note<<5)+
                        (inst[Chn[i].LastInst-1].envelope_phixed<<4);

            Reg4015|=(1<<i);

            Chn[i].Volume=(inst[Chn[i].LastInst-1].volume)<<2;
            Chn[i].AutoVolumeSlide=(-inst[Chn[i].LastInst-1].VolumeSlide);
            if(inst[Chn[i].LastInst-1].VolumeSlideDir)
              Chn[i].AutoVolumeSlide=(-Chn[i].AutoVolumeSlide);

            

            Chn[i].NonLoopedArpeggio=inst[Chn[i].LastInst-1].NonLoopedArpeggio;
            if(Chn[i].ReversedArpeggio=inst[Chn[i].LastInst-1].ReversedArpeggio) {
              Chn[i].AutoArpX=-inst[Chn[i].LastInst-1].ArpX;
              Chn[i].AutoArpY=-inst[Chn[i].LastInst-1].ArpY;
              Chn[i].AutoArpZ=-inst[Chn[i].LastInst-1].ArpZ;
              }
            else {  
              Chn[i].AutoArpX=inst[Chn[i].LastInst-1].ArpX;
              Chn[i].AutoArpY=inst[Chn[i].LastInst-1].ArpY;
              Chn[i].AutoArpZ=inst[Chn[i].LastInst-1].ArpZ;
              }
            if(inst[Chn[i].LastInst-1].PortaDir)
              Chn[i].AutoPorta=-inst[Chn[i].LastInst-1].Porta;
            else
              Chn[i].AutoPorta=inst[Chn[i].LastInst-1].Porta;

            Chn[i].AutoTremoloSpeed=inst[Chn[i].LastInst-1].TremoloSpeed;
            Chn[i].AutoTremoloDepth=inst[Chn[i].LastInst-1].TremoloDepth;

            Chn[i].AutoVibratoSpeed=inst[Chn[i].LastInst-1].VibratoSpeed;
            Chn[i].AutoVibratoDepth=inst[Chn[i].LastInst-1].VibratoDepth;

            Chn[i].LoopedNoise=inst[Chn[i].LastInst-1].LoopedNoise;

            if(inst[Chn[i].LastInst-1].hold_note)
              Chn[i].VblanksLeft=0x80;
            else
              Chn[i].VblanksLeft=inst[Chn[i].LastInst-1].active_tlength;
            }

          if(Chn[i].Tone==97)
            Reg4015&=(~(1<<i));
            //Chn[i].VblanksLeft=0;


          if((Chn[i].Tone) && (Chn[i].Tone!=97)) {
            Chn[i].BigTick=0;
            if(inst[Chn[i].LastInst-1].hold_note)
              Chn[i].VblanksLeft=0x80;
            else
              Chn[i].VblanksLeft=inst[Chn[i].LastInst-1].active_tlength;

            Chn[i].LastTone=Chn[i].Tone;
            Reg4015|=(1<<i);

            Chn[i].TremoloPos=0;

            if(Chn[i].Effect!=tEFF_TONEPORTA) {

                // just might work ? ;-) --thefox
                // same as in 6502 code
                // is this needed here??
                OldPeriodHi[i] = -1;

              Chn[i].VibratoPos=0;
              if(i==NOSWAV_CHN) {
                Chn[i].Period=Chn[i].LastTone-1;
                  //Write2SoundReg(0x400F, 8);
                }
              else
                Chn[i].Period=Note2PeriodTab[Chn[i].Tone-1];
              //Chn[i].PeriodFlag=1;
              }
            }

 
          DoEffsT0(i);
          }
        //GetSNote(DPCM_CHN);

          if(Chn[DPCM_CHN].Tone || Chn[DPCM_CHN].Inst) // SHIT...
          {
          int Oct=(Chn[DPCM_CHN].Tone-1)/12,
              Note=((int)((Chn[DPCM_CHN].Tone-1)-Oct*12))+1;
          PlayNote(DPCM_CHN, Chn[DPCM_CHN].Inst, Note, Oct);
          }
        }



void    PlayNED(void)
        {
// thefox: j, k unreferenced
        int i;//, j, k;


        if(!IsPlaying) return;

        if(EmulateNTSC) 
        if((--PalCounter)<=0) {
          PalCounter=6;
          return;
          }

        if(curr_speed) {

        // Reset update flags
        for(i=0; i<MAX_A_CHANNELS; i++)
          //Chn[i].PeriodFlag=0;


          if((++CurrTick)>=curr_speed) {
            CurrTick=0;
            if(curr_row>=64) {
              curr_row=0;
              if((++curr_order)>=order_tot)
                        curr_order=RestartPosition;
              }
            UpdateT0();
            curr_row++;

        for(i=0; i<MAX_A_CHANNELS; i++) {

          if(!(Chn[i].VblanksLeft & 0x80)) {
            if((--Chn[i].VblanksLeft)<0) {
              Reg4015&=(~(1<<i));
              Chn[i].VblanksLeft=0;
              }
            }


          if(Chn[i].VblanksLeft!=0) {
            if(i==TRIWAV_CHN)
              Write2SoundReg(0x4008, 0xFF);
            }

              if((Chn[i].Effect==tEFF_MODVOLUME) || (Chn[i].Inst)) {
                if(Chn[i].Volume<0) Chn[i].Volume=0;
                if(Chn[i].Volume>63) Chn[i].Volume=63;

                SetVolume(i, Chn[i].Volume);
                }

        //if(i==NOSWAV_CHN)
        //  SetPeriod(i, Chn[i].Period, 8);
        //else
          //if(Chn[i].PeriodFlag)
            //SetPeriod(i, Chn[i].Period, 8);

          }
            }
          else {
            for(i=0; i<MAX_A_CHANNELS; i++) {
              DoEffsTx(i);
              }
            }

          {
          if(!(ReadSoundReg(0x4015) & 0x10))
            Reg4015&=0xF;
          Write2SoundReg(0x4015, Reg4015);
          }

        }  // end if(curr_speed)
        Chn[0].BigTick++;
        Chn[1].BigTick++;
        Chn[2].BigTick++;
        Chn[3].BigTick++;
        }
/* ============ verbatim from nt2.c: PlayNote (note trigger) ======= */
void    PlayNote(int _Chn, int Inst, int Note, int Oct)
{

    if(_Chn==DPCM_CHN) {
        BYTE    r4010, r4011, r4012, r4013;

        int Samp=(NoteTable[Inst-1][Oct][Note-1]>>4) & 0x7;
        if(!SampleInst[Inst-1].Sample[Samp].SamplePtr) return;;
        UpperPrgPage[0]=(void *)SampleInst[Inst-1].Sample[Samp].SamplePtr;
        UpperPrgPage[1]=UpperPrgPage[0];
        UpperPrgPage[2]=UpperPrgPage[0];
        UpperPrgPage[3]=UpperPrgPage[0];
        r4010=(NoteTable[Inst-1][Oct][Note-1] & 0xF) | (SampleInst[Inst-1].Sample[Samp].LoopFlag<<6);
        r4011=SampleInst[Inst-1].Sample[Samp].Volume;
        r4012=0x00;
        r4013=SampleInst[Inst-1].Sample[Samp].Length;

        Write2SoundReg(0x4010, r4010);
        Write2SoundReg(0x4011, r4011);
        Write2SoundReg(0x4012, r4012);
        Write2SoundReg(0x4013, r4013);

        Write2SoundReg(0x4015, (Reg4015 & 0xF));
        Reg4015|=0x10;
        Write2SoundReg(0x4015, Reg4015);
    }
    // thefox added (clean me up!)
    else if(_Chn == NOSWAV_CHN) {
        struct inst_struc *I = &inst[Inst-1];
        int Period;
        BYTE SndReg0, SndReg1, SndReg2, SndReg3;

        int  Tone=(Oct<<3)+(Oct<<2)+Note;

        Tone=((15-(Tone)) & 0xF)+1;

        SndReg0=(inst[Inst-1].duty_cycle<<6)+(inst[Inst-1].hold_note<<5)+
            (inst[Inst-1].envelope_phixed<<4)+(inst[Inst-1].volume);
        SndReg1=(inst[Inst-1].variable_phreq<<7)+(inst[Inst-1].phq_changespd<<4)+
            (inst[Inst-1].ishi2lo<<3)+(inst[Inst-1].phq_range);
        SndReg2=Note2PeriodTab[Tone-1] & 0xFF;
        SndReg3=(inst[Inst-1].active_tlength<<3) | (Note2PeriodTab[Tone-1]>>8);

        Reg4015|=(1<<_Chn);
        Write2SoundReg(0x4015, Reg4015);

        Write2SoundReg(0x4000+_Chn*4, SndReg0);
        Write2SoundReg(0x4001+_Chn*4, SndReg1);

        //Period = Note2PeriodTab[Tone-1] & 0xFF;
        Period = Tone;

        Write2SoundReg(0x400E, (Period & 0xF)+(I->LoopedNoise<<7));
        Write2SoundReg(0x400F, 8);
    }

    else {
        BYTE SndReg0, SndReg1, SndReg2, SndReg3;
        int  Tone=(Oct<<3)+(Oct<<2)+Note;
        SndReg0=(inst[Inst-1].duty_cycle<<6)+(inst[Inst-1].hold_note<<5)+
            (inst[Inst-1].envelope_phixed<<4)+(inst[Inst-1].volume);
        SndReg1=(inst[Inst-1].variable_phreq<<7)+(inst[Inst-1].phq_changespd<<4)+
            (inst[Inst-1].ishi2lo<<3)+(inst[Inst-1].phq_range);
        SndReg2=Note2PeriodTab[Tone-1] & 0xFF;
        SndReg3=(inst[Inst-1].active_tlength<<3) | (Note2PeriodTab[Tone-1]>>8);

        Reg4015|=(1<<_Chn);
        Write2SoundReg(0x4015, Reg4015);

        Write2SoundReg(0x4000+_Chn*4, SndReg0);
        Write2SoundReg(0x4001+_Chn*4, SndReg1);
        Write2SoundReg(0x4002+_Chn*4, SndReg2);
        Write2SoundReg(0x4003+_Chn*4, SndReg3);
    }

    // Write2SoundReg(0x4000, 0x0F);
    // Write2SoundReg(0x4001, 0x00);
    // Write2SoundReg(0x4002, 0x5F);
    // Write2SoundReg(0x4003, 0x01);

}
/* ============ verbatim from nt2.c: .ned loaders ================= */
int     LoadNed(char *FileName)
        {
        int     FileOk=1;
        int     i;
        InfoHeader      Info;
        DataHeader      Data;
        char    ID_Check[5]={'N','E','D',':',' '};
        char    ID_Check10[4]={'N','E','D',0x10};
        if(!readfile(FileName, 0, sizeof(InfoHeader), &Info)) return 0;

        if((Info.Version==0x20) || (Info.Version==0x21))
          readfile(FileName, Info.HeaderSize+Info.NumSampleInstrumentsNTSC*sizeof(SampInstInfo)+Info.SampleDataSizeNTSC, sizeof(DataHeader), &Data);
        else
          readfile(FileName, Info.HeaderSize, sizeof(DataHeader), &Data);
                // Test if header is ok...
        for(i=0; i<5; i++)
          if(Info.ID[i]!=ID_Check[i]) FileOk=0;
        if(FileOk) {

          if(Data.Version==0x20) {
                ResetEditors();
                return(LoadNed20(FileName));}
          return(0);
          }
                // If not, test if it's a version 1.0 NED
        FileOk=1;
        for(i=0; i<4; i++)
          if(Info.ID[i]!=ID_Check10[i]) FileOk=0;
        if(FileOk) {
                ResetEditors();
                return(LoadNed10(FileName));}
        return(0);
        }



int     LoadNed10(char *FileName)
        {
        struct ned_header_struc NedHeader10;
// thefox: EmptyFlag unref.
        int     TotalPatterns=1, //EmptyFlag,
                TotalInstruments=0,
                OrderLength=1;
// thefox: k unref.
        int     i, j;//, k;
        DWORD   FileOffset=0;
        DWORD   TempOffset=0;
        BYTE    *PackedPatternData=NULL,
                TotalPackedPatternSize=0;
// thefox: PackedPatternSize unref.
//        WORD    PackedPatternSize[MAX_PATTERNS];
        WORD    PackedPatternOffset[MAX_PATTERNS];
        BYTE    PackedPattern[64*4*3];
        BYTE    OrderList10[128];

        FileOffset=readfile(FileName,FileOffset,sizeof(NedHeader10),&NedHeader10);
        

                // Load and unpack instruments
        FileOffset=readfile(FileName,FileOffset,sizeof(struct ned_inst_struc)*NedHeader10.inst_tot,ned_inst);
        for(i=0; i<NedHeader10.inst_tot; i++)
          UnPackInstrument10(&ned_inst[i], &inst[i]);

        FileOffset=readfile(FileName,FileOffset,NedHeader10.order_tot,OrderList10);
        FileOffset=readfile(FileName,FileOffset,NedHeader10.pat_tot*sizeof(WORD),PackedPatternOffset);

        for(i=0; i<NedHeader10.pat_tot; i++) {
          readfile(FileName,PackedPatternOffset[i],64*4*3,PackedPattern);
          ConvertNed10Pat2ChnPat(PackedPattern,&ChnPattern[0][i],&ChnPattern[1][i],
                                 &ChnPattern[2][i],&ChnPattern[3][i]);
          for(j=0; j<NedHeader10.order_tot; j++)
            if(OrderList10[j]==i) {
                OrderList[j][0]=i;
                OrderList[j][1]=i;
                OrderList[j][2]=i;
                OrderList[j][3]=i;
                OrderList[j][4]=0;
                }
          }
        order_tot=NedHeader10.order_tot;
        RestartPosition=0;
        g_ned_init_tempo=NedHeader10.init_speed; /* added: keep the tune's tempo */
        ClearOrder2Max(order_tot);
        return(1);
        }

void    ConvertNed10Pat2ChnPat(BYTE *Ned10Pat, ChannelPattern *P_A, ChannelPattern *P_B, ChannelPattern *P_C, ChannelPattern *P_D)
        {
        int     i, j;
        ChannelPattern  *CPat[4];
// thefox: Oct, EffParm, Note, Eff, Inst, PackedSnote unref.
        //DWORD   PackedSnote;
        //int     Note, Oct, Inst, Eff, EffParm;
        BYTE    sn1, sn2, sn3;
        int     TempNote, TempOct;

        CPat[0]=P_A;
        CPat[1]=P_B;
        CPat[2]=P_C;
        CPat[3]=P_D;

        for(i=0; i<64; i++)
          for(j=0; j<MAX_A_CHANNELS; j++) {
            sn1=Ned10Pat[(4*3)*i+3*j];
            sn2=Ned10Pat[(4*3)*i+3*j+1];
            sn3=Ned10Pat[(4*3)*i+3*j+2];

            if(!sn1) CPat[j]->Row[i].note=0;
            else
                {
                sn1--;
                TempOct=(int)(sn1/12);
                TempNote=(int)(sn1-(TempOct*12));
                if(TempNote==11) {
                  TempNote=BuggyNote2Normal[TempNote%12];
                  TempOct++;
                  }
                else
                  TempNote=BuggyNote2Normal[TempNote%12];
                TempNote++;
                TempOct++;
                CPat[j]->Row[i].note=TempNote;
                CPat[j]->Row[i].octave=TempOct;
            CPat[j]->Row[i].inst_num1=NIB_HI(sn2 & 0x1F);
            CPat[j]->Row[i].inst_num2=NIB_LO(sn2 & 0x1F);;
                }
            CPat[j]->Row[i].ephphect=nedeph2eph[(sn2 & 0xE0)>>5];
            CPat[j]->Row[i].e_parm1=NIB_HI(sn3);
            CPat[j]->Row[i].e_parm2=NIB_LO(sn3);
            if(CPat[j]->Row[i].ephphect==7)
              CPat[j]->Row[i].ephphect=8;
            if(CPat[j]->Row[i].ephphect==0xC) {
              if(CPat[j]->Row[i].e_parm1==0) {
                int     Parm=CPat[j]->Row[i].e_parm2<<2;
                CPat[j]->Row[i].e_parm1=(Parm>>4) & 0xF;
                CPat[j]->Row[i].e_parm2=(Parm & 0xF);
                }
              else
                if(CPat[j]->Row[i].e_parm1==1) {
                  CPat[j]->Row[i].ephphect=0xA;
                  CPat[j]->Row[i].e_parm1=CPat[j]->Row[i].e_parm2;
                  CPat[j]->Row[i].e_parm2=0;
                  }
              else
                if(CPat[j]->Row[i].e_parm1==2) {
                  CPat[j]->Row[i].ephphect=0xA;
                  CPat[j]->Row[i].e_parm1=0;
                  }
              }
            }
        }


void    ReadSampleChunk(void *SampleChunkData, InfoHeader *Info)
        {
        int             i, j;
        BYTE            *BytePtr=(BYTE *)SampleChunkData,
                        *OldBytePtr=(BYTE *)SampleChunkData;
        SampleInstInfo  *InstPtr=(SampleInstInfo *)BytePtr;
        int             NumInstruments;
        DWORD           TotalSampleSize;
        DWORD           ChunkSize=0;
        DWORD           SampleSize[MAX_INSTRUMENTS_DPCM*MAX_DPCM_SAMPLES+1];
        SampInstInfo    *InstP=(SampInstInfo *)SampleChunkData;

        TotalSampleSize=(*Info).SampleDataSizeNTSC;
        NumInstruments=(*Info).NumSampleInstrumentsNTSC;
        BytePtr+=NumInstruments*sizeof(SampInstInfo);
        
        for(i=0; i<NumInstruments; i++) 
          for(j=0; j<8; j++) 
            SampleSize[i*MAX_DPCM_SAMPLES+j]=InstP[i].Sample[j].Offset;
        SampleSize[NumInstruments*MAX_DPCM_SAMPLES]=TotalSampleSize;

        for(i=0; i<(NumInstruments*MAX_DPCM_SAMPLES); i++) {
          if(SampleSize[i]==0xFFFFFFFF)
            SampleSize[i]=0;
          else {
            j=1;
            while(SampleSize[i+j]==0xFFFFFFFF) j++;
            SampleSize[i]=SampleSize[i+j]-SampleSize[i];
            }
          }
       
          // Fill in instrument info
        for(i=0; i<NumInstruments; i++) {
          memcpy(NoteTable[i], InstP[i].NoteTable, MAX_DPCM_OCTAVES*12);
          if(strlen(InstP[i].Name))
            memcpy(InstName[i], InstP[i].Name, DPCM_INAME_LENGTH);
          for(j=0; j<MAX_DPCM_SAMPLES; j++) {
            if(strlen(InstP[i].Sample[j].Name))
              memcpy(SampleName[i][j],InstP[i].Sample[j].Name,DPCM_SNAME_LENGTH);
            if(InstP[i].Sample[j].Offset!=0xFFFFFFFF) {
                if(SampleInst[i].Sample[j].SamplePtr) free(SampleInst[i].Sample[j].SamplePtr);
                SampleInst[i].Sample[j].SamplePtr=(BYTE *)malloc(SampleSize[i*MAX_DPCM_SAMPLES+j]);
                memcpy(SampleInst[i].Sample[j].SamplePtr, &BytePtr[InstP[i].Sample[j].Offset], SampleSize[i*MAX_DPCM_SAMPLES+j]);
                }
            SampleInst[i].Sample[j].Length=InstP[i].Sample[j].Length;
            SampleInst[i].Sample[j].LoopStart=InstP[i].Sample[j].LoopStart;
            SampleInst[i].Sample[j].LoopFlag=InstP[i].Sample[j].LoopFlag;
            SampleInst[i].Sample[j].Volume=InstP[i].Sample[j].InitAmp;
            SampleInst[i].Sample[j].LoopAmp=InstP[i].Sample[j].LoopAmp;
            }
          }
        }


int     LoadNed20(char *FileName)
        {
        InfoHeader      Info;
        DataHeader      Data;
// thefox: EmptyFlag unref.
        int     TotalPatterns=1, //EmptyFlag,
                TotalInstruments=0,
                OrderLength=1;
// thefox: k unref.
        int     i, j;//, k;
        DWORD   FileOffset=0;
        DWORD   TempOffset=0;
        DWORD   TotalPackedPatternSize=0;
// thefox: PackedPatternSize unref.
//        WORD    PackedPatternSize[MAX_PATTERNS];
        WORD    PackedPatternOffset[MAX_CHANNELS][MAX_PATTERNS];
//        BYTE    PackedOrderList[NT_MAX_ORDER][3];
        BYTE    PackedOrderList[NT_MAX_ORDER][4];
        char    ID_Check[5]={'N','E','D',':',' '};
        BYTE    PackedPattern[256];
        BYTE    PackedInstDPCM[15][17];
        BYTE    *SampleChunk=NULL;
        //NedInstStruc    NedInst[16];
        NedInstStrucNew   NedInst[16];

        //readfile(FileName, 0, sizeof(InfoHeader), &Info);
        
        FileOffset=readfile(FileName,FileOffset,sizeof(InfoHeader),&Info);
        FileOffset=Info.HeaderSize;

        // MAY BE UNNECESSARY
        for(i=0; i<MAX_INSTRUMENTS_DPCM; i++)
          for(j=0; j<MAX_DPCM_SAMPLES; j++)
                if(SampleInst[i].Sample[j].SamplePtr) free(SampleInst[i].Sample[j].SamplePtr);

        SampleChunk=(BYTE *)malloc(Info.NumSampleInstrumentsNTSC*sizeof(SampInstInfo)+Info.SampleDataSizeNTSC);

        if((Info.HeaderSize==sizeof(InfoHeader)) && ((Info.Version==0x20) || (Info.Version==0x21))) {
          if(Info.NumSampleInstrumentsNTSC || Info.SampleDataSizeNTSC) {
            FileOffset=readfile(FileName,Info.HeaderSize,Info.NumSampleInstrumentsNTSC*sizeof(SampInstInfo)+Info.SampleDataSizeNTSC,SampleChunk);
            ReadSampleChunk(SampleChunk, &Info);
            /* printf("HRRRM!!\n"); -- silenced for headless playback */
            }
          }


        //FileOffset=readfile(FileName,FileOffset,sizeof(DataHeader),&Data);
        readfile(FileName,FileOffset,sizeof(DataHeader),&Data);
        FileOffset+=Data.HeaderSize;

                         // Load and unpack instruments
        if(Data.NumInstruments) {
//          FileOffset=readfile(FileName,FileOffset,sizeof(NedInstStruc)*Data.NumInstruments, NedInst);
          FileOffset=readfile(FileName,FileOffset,sizeof(NedInstStrucNew)*Data.NumInstruments, NedInst);
          for(i=0; i<Data.NumInstruments; i++)
                UnPackInstrument20(&NedInst[i], &inst[i]);
          }

                        // Load and unpack DPCM instruments
        if(Data.NumInstrumentsDPCM) {
          FileOffset=readfile(FileName,FileOffset,Data.NumInstrumentsDPCM*17,PackedInstDPCM);
          for(i=0; i<Data.NumInstrumentsDPCM; i++)
                UnPackInstrumentDPCM20(PackedInstDPCM[i], &SampleInst[i]);
          }

        free(SampleChunk);

               // Load and unpack order list
//        FileOffset=readfile(FileName,FileOffset,3*(Data.SongLength),PackedOrderList);
        FileOffset=readfile(FileName,FileOffset,4*(Data.SongLength),PackedOrderList);
        for(i=0; i<(Data.SongLength); i++)
                UnPackOrderEntry20(PackedOrderList[i], OrderList[i]);

                    // Load and unpack patterns
        for(i=0; i<MAX_CHANNELS; i++) {
          int TempoLino;
          if(i==DPCM_CHN)
            TempoLino=Data.NumPatterns[i];
          else
            TempoLino=Data.NumPatternsDPCM;
          //if(Data.NumPatterns[i]) {
          if(TempoLino) {
            FileOffset=readfile(FileName,FileOffset,sizeof(WORD)*Data.NumPatterns[i],PackedPatternOffset[i]);
            for(j=0; j<Data.NumPatterns[i]; j++) {
                  if(PackedPatternOffset[i][j]) {
                    if(Info.Version==0x21)
                        readfile(FileName,(PackedPatternOffset[i][j] & 0x7FFF)+Info.HeaderSize+Info.NumSampleInstrumentsNTSC*sizeof(SampInstInfo)+Info.SampleDataSizeNTSC, 256, PackedPattern);
                    else
                        readfile(FileName,(PackedPatternOffset[i][j] & 0x7FFF)+Info.HeaderSize, 256, PackedPattern);
                    // !!
                    UnPackPattern20(PackedPattern, &ChnPattern[i][j], i);
                    }
                  else ClearPattern(&ChnPattern[i][j]);
                  }
            }
          }
                    // Load and unpack DPCM patterns
          /* if(Data.NumPatternsDPCM) {
            FileOffset=readfile(FileName,FileOffset,sizeof(WORD)*Data.NumPatternsDPCM,PackedPatternOffset[DPCM_CHN]);
            for(j=0; j<Data.NumPatternsDPCM; j++) {
                  if(PackedPatternOffset[DPCM_CHN][j]) {
                    readfile(FileName,(PackedPatternOffset[DPCM_CHN][j] & 0x7FFF)+Info.HeaderSize, 64*3, PackedPattern);
                    UnPackPatternDPCM20(PackedPattern, &ChnPattern[DPCM_CHN][j]);
                    }
                  else ClearPattern(&ChnPattern[DPCM_CHN][j]);
                  }
            } */
        

        order_tot=Data.SongLength;
        ClearOrder2Max(order_tot);
        curr_order=0;
        RestartPosition=Data.RestartPosition;
        g_ned_init_tempo=Data.InitTempo; /* added: keep the tune's tempo */
        return(1);
        }

/* ============ verbatim from nt2.c: unpack helpers =============== */
DWORD   UnPackPattern20(BYTE *PackedData, ChannelPattern *NT2_Pattern, int Chn)
        {
// thefox: SmpNum unref.
        int     Note, Oct, Inst, Eff, EffParm, Tone;//, SmpNum;
// thefox: j unref.
        int     i;//, j;
        DWORD   PackedSize=0;
// thefox: PackedSnote unref.
//        DWORD   PackedSnote;
        int     NibPos=0;
        int     StartRow=PackedData[0],
                EndRow=PackedData[1];
        BYTE    *BitEntry,
                *PatData;

        ClearPattern(NT2_Pattern);

        BitEntry=PackedData+2;
        PatData=PackedData+2+((((EndRow+1)-StartRow)+1)>>1);

        for(i=StartRow; i<=EndRow; i++) {
          int   EntryPos=i-StartRow;
          int   ByteOffs=(EntryPos>>1),
                OddNib=(EntryPos & 1);

          Note=0;
          Oct=0;
          Tone=0;
          Inst=0;
          Eff=0;
          EffParm=0;

          if((BitEntry[ByteOffs]>>(4*OddNib)) & 1) {
            if(Chn==NOSWAV_CHN) {
              Tone|=(15-(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF))+1;
              NibPos++;
              }
            else {
              if((NibPos & 1)==0) {
                Tone|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF);
                //Tone|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
                NibPos++;
                Tone|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF)<<4;
                //Tone|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
                NibPos++;
                }
              else {
                Tone|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
                //Tone|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
                NibPos++;
                Tone|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
                //Tone|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
                NibPos++;
                }
                
              }
            }
          if((BitEntry[ByteOffs]>>(4*OddNib)) & 2) {
            Inst=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF)+1;
            NibPos++;
            }
          if((BitEntry[ByteOffs]>>(4*OddNib)) & 4) {
            Eff=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
            NibPos++;
            }
          if((BitEntry[ByteOffs]>>(4*OddNib)) & 8) {
            if((NibPos & 1)==0) {
              EffParm|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF);
              //EffParm|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
              NibPos++;
              EffParm|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF)<<4;
              //EffParm|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
              NibPos++;
              }
            else {
              EffParm|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
              //EffParm|=(PatData[(NibPos>>1)]>>(4*(NibPos & 1)) & 0xF)<<4;
              NibPos++;
              EffParm|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
              //EffParm|=((PatData[(NibPos>>1)]>>(4*(NibPos & 1))) & 0xF);
              NibPos++;
              }
            }

          if(Tone) {
            if(Tone==97) {
              Oct=0;
              Note=NOTE_OPHPH;
              }
            else {
              Oct=(int)((Tone-1)/12);
              Note=(int)((Tone-1)-Oct*12)+1;
              }
            }
          else
            {
            Note=0;
            Oct=0;
            }

          NT2_Pattern->Row[i].note=Note;
          NT2_Pattern->Row[i].octave=Oct;
          NT2_Pattern->Row[i].inst_num1=(Inst & 0x10)>>4;
          NT2_Pattern->Row[i].inst_num2=(Inst & 0x0F);
          NT2_Pattern->Row[i].ephphect=Eff;
          NT2_Pattern->Row[i].e_parm1=(EffParm & 0xF0)>>4;
          NT2_Pattern->Row[i].e_parm2=(EffParm & 0x0F);
          }
        return(0);
        }


DWORD   UnPackPatternDPCM20(BYTE *PackedData, ChannelPattern *NT2_Pattern)
        {
        int     Note=0, Oct=0, Inst, Eff, EffParm, Tone, SmpNum;
        int     i, j, k;
        DWORD   PackedSize=0;
        DWORD   PackedSnote;



       for(i=0; i<64; i++) {
          PackedSnote=PackedData[0]+(PackedData[1]<<8)+(PackedData[2]<<16);

          Tone=(PackedSnote & 0xF);
          SmpNum=((PackedSnote>>4) & 0x7);
          Inst=((PackedSnote>>8) & 0x0F);
          Eff=((PackedSnote>>12) & 0x07);
          EffParm=((PackedSnote>>16) & 0xFF);

          
          

          if(Inst) {
              Oct=0;
              Note=1;

            for(j=0; j<MAX_DPCM_OCTAVES; j++)
              for(k=0; k<12; k++) {
                if(NoteTable[Inst-1][j][k]==(Tone+(SmpNum<<4))) {
                  Oct=j;
                  Note=k+1;}
                }
            }
          else
            {
            Note=0;
            Oct=0;
            }

          NT2_Pattern->Row[i].note=Note;
          NT2_Pattern->Row[i].octave=Oct;
          NT2_Pattern->Row[i].inst_num1=0;
          NT2_Pattern->Row[i].inst_num2=(Inst & 0x0F);
          NT2_Pattern->Row[i].ephphect=Eff;
          NT2_Pattern->Row[i].e_parm1=(EffParm & 0xF0)>>4;
          NT2_Pattern->Row[i].e_parm2=(EffParm & 0x0F);

          PackedData+=3;
          PackedSize+=3;
          }
        return(PackedSize);
        }


DWORD   UnPackInstrument20(NedInstStrucNew *Src, struct inst_struc *Dest)
        {

        (*Dest).ArpX=(*Src).ArpXY & 0xF;
        (*Dest).ArpY=((*Src).ArpXY>>4) & 0xF;
        (*Dest).ArpZ=(*Src).ArpZAndMisc & 0xF;
        (*Dest).duty_cycle=(*Src).CR0>>6;
        (*Dest).hold_note=((*Src).CR0>>5) & 1;
        (*Dest).envelope_phixed=((*Src).CR0>>4) & 1;
        (*Dest).VibratoDepth=(*Src).AutoVibrato & 0xF;
        (*Dest).VibratoSpeed=((*Src).AutoVibrato>>4) & 0xF;
        (*Dest).TremoloDepth=(*Src).AutoTremolo & 0xF;
        (*Dest).TremoloSpeed=((*Src).AutoTremolo>>4) & 0xF;
        (*Dest).volume=((*Src).VolumeAndVolSlide>>4) & 0xF;
        (*Dest).VolumeSlide=(*Src).VolumeAndVolSlide & 0xF;
        (*Dest).VolumeSlideDir=((*Src).TimeLengthAndVSlideDir>>7) & 1;
        (*Dest).active_tlength=(*Src).TimeLengthAndVSlideDir & 0x7F;
        (*Dest).LoopedNoise=((*Src).ArpZAndMisc>>6) & 1;
        (*Dest).ReversedArpeggio=((*Src).ArpZAndMisc>>7) & 1;
        (*Dest).NonLoopedArpeggio=((*Src).ArpZAndMisc>>5) & 1;

        (*Dest).PortaDir=((*Src).Porta>>1) & 1;
        if((*Dest).PortaDir)
          (*Dest).Porta=-((*Src).Porta & 0x7F);
        else
          (*Dest).Porta=(*Src).Porta & 0x7F;

        return(8);
        }

DWORD   UnPackInstrument10(struct ned_inst_struc *Src, struct inst_struc *Dest)
        {
        // Control reg #1
        Dest->duty_cycle=(Src->creg1>>6);
        Dest->hold_note=((Src->creg1 & 0x20)>>5);
        Dest->envelope_phixed=((Src->creg1 & 0x10)>>4);
        Dest->volume=(Src->creg1 & 0x0f);
        // Control reg #2
        Dest->variable_phreq=(Src->creg2>>7);
        Dest->phq_changespd=((Src->creg2 & 0x70)>>4);
        Dest->ishi2lo=((Src->creg2 & 0x08)>>3);
        Dest->phq_range=(Src->creg2 & 0x07);
        // Control reg #4
        Dest->active_tlength=LCounterTable[((Src->creg4 & 0xf8)>>3)];

        (*Dest).ArpX=0;
        (*Dest).ArpY=0;
        (*Dest).ArpZ=0;
        (*Dest).VibratoDepth=0;
        (*Dest).VibratoSpeed=0;
        (*Dest).TremoloDepth=0;
        (*Dest).TremoloSpeed=0;
        (*Dest).VolumeSlide=0;
        (*Dest).VolumeSlideDir=0;
        return(3);
        }

DWORD   UnPackInstrumentDPCM20(BYTE *Src, InstDPCM *TheInst)
        {
// thefox: j unref.
        int     i;//, j;
        //TheInst->Length=Src[0];
        //TheInst->LoopStart=Src[1];
        //TheInst->LoopLength=Src[2];
        //TheInst->Volume=Src[3];

        //TheInst->Volume=Src[0];
        for(i=0; i<MAX_DPCM_SAMPLES; i++) {
          TheInst->Sample[i].Volume=Src[0];
          TheInst->Sample[i].Length=Src[1+i*2];
          TheInst->Sample[i].LoopStart=(Src[1+i*2+1] & 0x3F);
          TheInst->Sample[i].LoopFlag=Src[1+i*2+1]>>7;
          }
        return(17);
        }

DWORD   UnPackOrderEntry20(BYTE *Src, BYTE *Dest)
        {
// thefox: i, j unref.
        //int     i, j;
        DWORD   PackedOrderEntry=Src[0]+(Src[1]<<8)+(Src[2]<<16)+(Src[3]<<24);
// thefox: DWORD -> BYTE conversion warning
        Dest[SQRWAV1_CHN]=(BYTE)(PackedOrderEntry & 0x1F);
        Dest[SQRWAV2_CHN]=(BYTE)((PackedOrderEntry>>5) & 0x1F);
        Dest[TRIWAV_CHN]=(BYTE)((PackedOrderEntry>>10) & 0x1F);
        Dest[NOSWAV_CHN]=(BYTE)((PackedOrderEntry>>15) & 0x1F);
        Dest[DPCM_CHN]=(BYTE)((PackedOrderEntry>>20) & 0x0F);
        return(3);
        }

/* ============ verbatim from nt2.c: order/pattern helpers ======== */
void    CopyOrderEntry(int Src, int Dest)
        {
        int     i;
        for(i=0; i<MAX_CHANNELS; i++) OrderList[Dest][i]=OrderList[Src][i];
        }

void    ClearOrderEntry(int Entry)
        {
        memset(OrderList[Entry], 0, 5);
        }

void    CopyPattern(ChannelPattern *Src, ChannelPattern *Dest)
        {
        memcpy(Dest, Src, sizeof(ChannelPattern));
        }

void    ClearPattern(ChannelPattern *Pat)
        {
        memset(Pat, 0, sizeof(ChannelPattern));
        }

int     PatternIsEmpty(ChannelPattern *Pat)
        {
// thefox: j unref.
        int     i;//, j;
        struct  snote_struc *S;
        for(i=0; i<64; i++) {
          S=&Pat->Row[i];
          if(S->note || S->inst_num1 || S->inst_num2 || S->ephphect)
                return(0);
          }
        return(1);
        }


void    ClearOrder2Max(int StartEntry)
        {
        int     i;
// thefox: i used w/o intializing, changed i -> StartEntry
		if(StartEntry>=NT_MAX_ORDER) return;
        for(i=StartEntry; i<NT_MAX_ORDER; i++)
                ClearOrderEntry(i);
        }


/* ============ verbatim from nt2.c: ResetEditors ================= */
void    ResetEditors(void)
        {
        int     i, j;

        // Reset patterns
        for(i=0; i<MAX_A_CHANNELS; i++)
                for(j=0; j<MAX_PATTERNS; j++)
                        ClearPattern(&ChnPattern[i][j]);
                for(j=0; j<MAX_PATTERNS_DPCM; j++)
                        ClearPattern(&ChnPattern[DPCM_CHN][j]);

        // Reset Order List
        for(i=0; i<NT_MAX_ORDER; i++)
                ClearOrderEntry(i);

        // Reset Synth Instruments
        for(i=0; i<MAX_INSTRUMENTS; i++)
                memset(inst, 0, MAX_INSTRUMENTS*sizeof(struct inst_struc));
       
        // Reset DPCM Instruments
        for(i=0; i<MAX_INSTRUMENTS_DPCM; i++)
          for(j=0; j<MAX_DPCM_SAMPLES; j++)
                if(SampleInst[i].Sample[j].SamplePtr) free(SampleInst[i].Sample[j].SamplePtr);
        memset(SampleInst, 0, MAX_INSTRUMENTS_DPCM*sizeof(InstDPCM));

        memset(InstName, 0, MAX_INSTRUMENTS_DPCM*DPCM_INAME_LENGTH);
        memset(SampleName, 0, MAX_INSTRUMENTS_DPCM*MAX_DPCM_SAMPLES*DPCM_INAME_LENGTH);
        // Reset DPCM Note Tables
        memset(NoteTable, 0, MAX_INSTRUMENTS_DPCM*MAX_DPCM_OCTAVES*12);
        }

/* ===================================================================== */
/* == chipmachine integration API (new code) ========================== */
/* ===================================================================== */

/* In the SDL port these opened/closed the SDL audio device. We pull samples on
 * demand via Mix(), so they are no-ops here. apuwrap.c calls them from
 * NesessInitialize()/NesessShutdown(). */
int SND_SoundSetup(void)    { return 1; }
int SND_SoundShutdown(void) { return 1; }

static int ned_inited = 0;

/* Allocate/clear the song buffers the loaders write into. Mirrors the parts of
 * the port's nt_misc_init() that the replay engine actually depends on. */
static void ned_engine_init_once(void)
{
    int i, j;
    if (ned_inited) return;
    for (i = 0; i < MAX_CHANNELS; i++) {
        ChnPattern[i] = (ChannelPattern *)malloc(sizeof(struct snote_struc) * 64 * MAX_PATTERNS);
        memset(ChnPattern[i], 0, sizeof(struct snote_struc) * 64 * MAX_PATTERNS);
    }
    for (i = 0; i < 8; i++)
        for (j = 0; j < 13; j++)
            note_phreq[i][j] = (float)(((double)BASE_PHREQ * pow(2, (double)i) *
                                        pow(HTONE_CONST, (double)(j - 1))));
    memset(inst, 0, sizeof(inst));
    memset(OrderList, 0, sizeof(OrderList));
    ned_inited = 1;
}

/* Parse a .ned module from disk. Returns 1 on success, 0 otherwise. */
int ned_engine_load(const char *path)
{
    ned_engine_init_once();
    g_ned_init_tempo = 6;
    EmulateNTSC = 0;            /* the tracker (and this port) play back as PAL */
    if (!LoadNed((char *)path)) return 0;
    return 1;
}

/* Start playback from the top of the order list (mirrors the right-shift
 * "play from start" handler in the editor). */
int ned_engine_start(void)
{
    CallInsideMixer = PlayNED;
    if (!NesessInitialize()) return 0;

    if (g_ned_init_tempo > 0) curr_speed = g_ned_init_tempo;
    curr_order = 0;
    curr_row   = 0;
    CurrTick   = curr_speed;
    ResetPlayer();
    Reg4015 = 0x0;
    Write2SoundReg(0x4000, 0x10);
    Write2SoundReg(0x4004, 0x10);
    Write2SoundReg(0x4008, 0x00);
    Write2SoundReg(0x400C, 0x10);
    Write2SoundReg(0x4015, 0x0);
    IsPlaying = 1;
    return 1;
}

/* Render 'frames' mono 16-bit samples (the engine/APU are mono). */
void ned_engine_render(short *buf, int frames)
{
    Mix(buf, frames, 1);
}

void ned_engine_shutdown(void)
{
    NesessShutdown();
}
