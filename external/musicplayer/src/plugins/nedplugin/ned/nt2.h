#include "typedefs.h"

#define MAGIC_NUMBER    (float)111860.78
#define HTONE_CONST     (double)1.059463094
#define BASE_PHREQ      (double)69.3


#define MAX_CHANNELS            5
#define MAX_A_CHANNELS          4
#define MAX_PATTERNS            32
#define MAX_PATTERNS_DPCM       16
#define NT_MAX_ORDER            80
#define MAX_INSTRUMENTS         16

#define MAX_INSTRUMENTS_DPCM    15
#define MAX_DPCM_SAMPLES        8

#define DPCM_INAME_LENGTH       12
#define DPCM_SNAME_LENGTH       12
#define NUM_VISIBLE_INAMES      7

#define MAX_DPCM_OCTAVES        4
#define NOTE_TABLE_X            60
#define NOTE_TABLE_Y            2

#define NUMBER_OPH_COLS 7

#define SQRWAV1_CHN     0
#define SQRWAV2_CHN     1
#define TRIWAV_CHN      2
#define NOSWAV_CHN      3
#define DPCM_CHN        4

#define NOTE_COL        0
#define OCTAVE_COL      1
#define INST_COL1       2
#define INST_COL2       3
#define EPHPHECT_COL    4
#define EPARM1_COL      5
#define EPARM2_COL      6

#define NOTE_POS        0
#define OCTAVE_POS      2
#define INST1_POS       4
#define INST2_POS       5
#define EFF_POS         7
#define EPARM1_POS      8
#define EPARM2_POS      9

#define EDIT_PAT        0
#define EDIT_INST       1
#define EDIT_ORDER      2

#define PATTERN_EDITOR  0
#define SAMPLE_EDITOR   1

#define EDIT_SAMPLE_NAMES       0
#define EDIT_SAMPLE_SNAMES      1
#define EDIT_SAMPLE_NOTETABLE   2
#define EDIT_SAMPLE_SETTINGS    3


#define INAMES_X                26
#define INAMES_Y                4

#define SNAMES_X                41
#define SNAMES_Y                2


#define SAMPLE_SETTINGS_X       2
#define SAMPLE_SETTINGS_Y       4

#define SAMPLE_SETTING_VOLUME   0
#define SAMPLE_SETTING_LENGTH   1
#define SAMPLE_SETTING_REPEAT   2
#define SAMPLE_SETTING_LOOPFLAG 3
//#define SAMPLE_SETTING_REPLEN   x


/* #define KEY_NOTE_Cn0    44      // Z
#define KEY_NOTE_Dn1    45      // X
#define KEY_NOTE_En1    46      // C
#define KEY_NOTE_PHn1   47      // V
#define KEY_NOTE_Gn1    48      // B
#define KEY_NOTE_An1    49      // N
#define KEY_NOTE_Bn1    50      // M
#define KEY_NOTE_Cs1    31      // S
#define KEY_NOTE_Ds1    32      // D
#define KEY_NOTE_PHs1   34      // G
#define KEY_NOTE_Gs1    35      // H
#define KEY_NOTE_As1    36      // J
#define KEY_NOTE_Cn1    16      // Q
#define KEY_NOTE_Dn2    17      // W
#define KEY_NOTE_En2    18      // E
#define KEY_NOTE_PHn2   19      // R
#define KEY_NOTE_Gn2    20      // T
#define KEY_NOTE_An2    21      // Y
#define KEY_NOTE_Bn2    22      // U
#define KEY_NOTE_Cn2    23      // I
#define KEY_NOTE_Dn3    24      // O
#define KEY_NOTE_En3    25      // P
#define KEY_NOTE_Cs2    3       // 2
#define KEY_NOTE_Ds2    4       // 3
#define KEY_NOTE_PHs2   6       // 5
#define KEY_NOTE_Gs2    7       // 6
#define KEY_NOTE_As2    8       // 7
#define KEY_NOTE_Cs3    10      // 9
#define KEY_NOTE_Ds3    11      // 0*/

#define KEY_NOTE_Cn1    KEY_Z      // Z
#define KEY_NOTE_Cs1    KEY_S      // S
#define KEY_NOTE_Dn1    KEY_X      // X
#define KEY_NOTE_Ds1    KEY_D      // D
#define KEY_NOTE_En1    KEY_C      // C
#define KEY_NOTE_PHn1   KEY_V      // V
#define KEY_NOTE_PHs1   KEY_G      // G
#define KEY_NOTE_Gn1    KEY_B      // B
#define KEY_NOTE_Gs1    KEY_H      // H
#define KEY_NOTE_An1    KEY_N      // N
#define KEY_NOTE_As1    KEY_J      // J
#define KEY_NOTE_Bn1    KEY_M      // M

#define KEY_NOTE_Cn2    KEY_Q      // Q
#define KEY_NOTE_Cs2    KEY_2       // 2
#define KEY_NOTE_Dn2    KEY_W      // W
#define KEY_NOTE_Ds2    KEY_3       // 3
#define KEY_NOTE_En2    KEY_E      // E
#define KEY_NOTE_PHn2   KEY_R      // R
#define KEY_NOTE_PHs2   KEY_5       // 5
#define KEY_NOTE_Gn2    KEY_T      // T
#define KEY_NOTE_Gs2    KEY_6       // 6
#define KEY_NOTE_An2    KEY_Y      // Y
#define KEY_NOTE_As2    KEY_7       // 7
#define KEY_NOTE_Bn2    KEY_U      // U

#define KEY_NOTE_Cn3    KEY_I      // I
#define KEY_NOTE_Cs3    KEY_9      // 9
#define KEY_NOTE_Dn3    KEY_O      // O
#define KEY_NOTE_Ds3    KEY_0      // 0
#define KEY_NOTE_En3    KEY_P      // P



// not used by nt2.c... --thefox
#define KEY_NOTE_NO     57              // 0

#define KEY_NOTE_OPHPH  KEY_SPACE       // Space

/* #define NOTE_Cs         1
#define NOTE_D          2
#define NOTE_Ds         3
#define NOTE_E          4
#define NOTE_PH         5
#define NOTE_PHs        6
#define NOTE_G          7
#define NOTE_Gs         8
#define NOTE_A          9
#define NOTE_As         10
#define NOTE_B          11
#define NOTE_C          12 */ 


#define NOTE_C          1
#define NOTE_Cs         2
#define NOTE_D          3
#define NOTE_Ds         4
#define NOTE_E          5
#define NOTE_PH         6
#define NOTE_PHs        7
#define NOTE_G          8
#define NOTE_Gs         9
#define NOTE_A          10
#define NOTE_As         11
#define NOTE_B          12


#define NOTE_NO         0
#define NOTE_OPHPH      13

#define DEC_ROW(x)      if((curr_row-=x)<0) curr_row=0
#define INC_ROW(x)      if((curr_row+=x)>=64) curr_row=63




#define DEC_COL(x)      if((curr_chncol-=x)<0)                { if((--curr_chn)<0) { curr_chn=(MAX_CHANNELS-1); curr_chncol=NUMBER_OPH_COLS-1;                 } else    curr_chncol=NUMBER_OPH_COLS-1; }
#define INC_COL(x)      if((curr_chncol+=x)>=NUMBER_OPH_COLS) { if((++curr_chn)>(MAX_CHANNELS-1))  { curr_chn=0;  curr_chncol=0; } else    curr_chncol=0;                 }



#define II_COL    77
#define II_ROW    20

#define IIBK_COLOR      3
#define IIPH_COLOR      7

#define OLPH_COLOR      7
#define OLPH_COLOR2     8
#define OLBK_COLOR      1


#define OL_COL          2
#define OL_ROW          7


#define ALERT_LINE      25


#define PATLINES_START_SCR      19L
#define PATLINES_END_SCR        46L
#define CURSOR_LINE_SCR         32L
#define SNOTE_LENGTH_SCR        10L
#define SNOTE_DPCM_LENGTH_SCR   10L

#define EMPTY_COLOR             0x01
#define NOTE_COLOR              0x0A
#define OCT_COLOR               0x0A
#define INST_COLOR              0x02
#define EFF_COLOR               0x0A
#define EPARM_COLOR             0x0A
#define EMPTY_LCOLOR            0x09
#define NOTE_LCOLOR             0x0F
#define OCT_LCOLOR              0x0F
#define INST_LCOLOR             0x0E
#define EFF_LCOLOR              0x0F
#define EPARM_LCOLOR            0x0F

#define BLOCKCHAR_UPPER         0xDF
#define BLOCKCHAR_LOWER         0xDC

#define WaveFormX1              0
#define WaveFormY1              40
#define WaveFormX2              79
#define WaveFormY2              49


#define DPCMLength2Normal(x)    (int)((int)x)*16+1
#define Normal2DPCMLength(x)    (int)((int)(x-1)/16)

typedef signed char boolean;

struct  snote_struc {
        unsigned        char    note;
        unsigned        char    octave;
        unsigned        char    inst_num1;      // Instrument's high nibble
        unsigned        char    inst_num2;      // Instrument's low nibble
        unsigned        char    ephphect;
        unsigned        char    e_parm1;        // Ephphect's high nibble
        unsigned        char    e_parm2;        // Ephphect's low nibble
        };

struct  patrow_struc {
        struct  snote_struc snote[MAX_CHANNELS];
        };

/*typedef _Packed struct {
        BYTE    Note;
        BYTE    InstNum;
        BYTE    Effect;
        BYTE    EffParm1;
        BYTE    EffParm2;
        } SnoteDPCM;*/

struct  pat_struc {
        struct patrow_struc patrow[64];
        };

// thefox: no _Packed in MSVC -> changed to #pragma
#define _Packed
#pragma pack(push, 1)
typedef _Packed struct {
        struct  snote_struc Row[64];
        } ChannelPattern;
#pragma pack(pop)

/*typedef _Packed struct {
        SnoteDPCM       Row[64];
        } PatternDPCM;*/


struct  inst_struc {
        signed          char    duty_cycle;
        signed          char    hold_note;
        signed          char    envelope_phixed;
        signed          char    volume;
        signed          char    variable_phreq;
        signed          char    phq_changespd;
        signed          char    ishi2lo;
        signed          char    phq_range;
        signed          char    active_tlength;
        signed          char    LoopedNoise;

        signed          char    ArpX;
        signed          char    ArpY;
        signed          char    ArpZ;
        signed          char    VibratoSpeed;
        signed          char    VibratoDepth;
        signed          char    TremoloSpeed;
        signed          char    TremoloDepth;
        signed          char    VolumeSlide;
        signed          char    VolumeSlideDir;

        signed          char    Porta;
        signed          char    PortaDir;
        signed          char    ReversedArpeggio;
        signed          char    NonLoopedArpeggio;
        };
// thefox: _Packed -> #pragma
#pragma pack(push, 1)
typedef _Packed struct {
        BYTE    ArpXY;
        BYTE    ArpZAndCR0;
        BYTE    AutoVibrato;
        BYTE    AutoTremolo;
        BYTE    VolumeAndVolSlide;
        BYTE    TimeLengthAndVSlideDir;
        BYTE    Porta;
        BYTE    CR1;
        } NedInstStruc;


typedef _Packed struct {
        BYTE    CR0;
        BYTE    Porta;
        BYTE    AutoVibrato;
        BYTE    AutoTremolo;

        BYTE    TimeLengthAndVSlideDir;
        BYTE    VolumeAndVolSlide;
        BYTE    ArpZAndMisc;
        BYTE    ArpXY;
        } NedInstStrucNew;



_Packed struct  ned_header_struc {
        unsigned        char    id_byte1;       // 'N'
        unsigned        char    id_byte2;       // 'E'
        unsigned        char    id_byte3;       // 'D'
        unsigned        char    version;        // BCD, always 1.0
        unsigned        char    init_speed;     // Initial speed oph tune
        unsigned        char    inst_tot;       // Total # oph instruments
        unsigned        char    pat_tot;        // Total # oph patterns
        unsigned        char    order_tot;      // Length oph order list
        };

_Packed struct  ned_inst_struc {
        unsigned        char    creg1;          // NS control reg #1
        unsigned        char    creg2;          // NS control reg #2
        unsigned        char    creg4;          // NS control reg #4
        };                              // (bits 0-2 not used)


typedef _Packed struct {
        BYTE    *SamplePtr;
        BYTE    Length,
                LoopStart,
                LoopFlag,
                Volume,
                LoopAmp;
        } SampleDPCM;

typedef _Packed struct {
        SampleDPCM      Sample[MAX_DPCM_SAMPLES];
        //BYTE            Volume;
        } InstDPCM;


typedef _Packed struct {
        char    ID[5];
        char    ModuleName[29];
        char    Terminator;
        DWORD   HeaderSize;
        BYTE    Version;
        WORD    Flags;
        BYTE    NumSampleInstrumentsNTSC;
        BYTE    NumSampleInstrumentsPAL;
        DWORD   SampleDataSizeNTSC;
        DWORD   SampleDataSizePAL;
        } InfoHeader;

typedef _Packed struct {
        BYTE    Version;
        BYTE    HeaderSize;
        BYTE    Flags;
        BYTE    InitTempo;
        BYTE    RestartPosition;
        BYTE    NumInstruments;
        BYTE    NumInstrumentsDPCM;
        BYTE    SongLength;
        BYTE    NumPatterns[MAX_A_CHANNELS];
        BYTE    NumPatternsDPCM;
        BYTE    Reserved[3];
        } DataHeader;

typedef _Packed struct {
        BYTE    NoteTable[MAX_DPCM_OCTAVES][12];
        char    Name[DPCM_INAME_LENGTH];
        char    SampleName[MAX_DPCM_SAMPLES][DPCM_SNAME_LENGTH];
        DWORD   SampleOffset[MAX_DPCM_SAMPLES];
        } SampleInstInfo;

typedef _Packed struct {
        char    Name[DPCM_SNAME_LENGTH];
        DWORD   Offset;
        BYTE    InitAmp;
        BYTE    LoopAmp;
        BYTE    Length;
        BYTE    LoopStart;
        BYTE    LoopFlag;
        } SampleInfo;


typedef _Packed struct {
        BYTE            NoteTable[MAX_DPCM_OCTAVES][12];
        char            Name[DPCM_INAME_LENGTH];
        BYTE            NumSamples;
        SampleInfo      Sample[MAX_DPCM_SAMPLES];
        } SampInstInfo;
#pragma pack(pop)

int             nt_misc_init(void);

void            nt_update_pos(void);
