#pragma once
/* This player module was ported from:
 AY-3-8910/12 Emulator
 Version 3.0 for Windows 95
 Author Sergey Vladimirovich Bulba
 (c)1999-2004 S.V.Bulba

 Fuxoft AY Language (.fxm) player. The format ("FXSM" magic) is the compiled
 output of Frantisek Fuka's AY music language; see Bulba's AY_Emul, whose
 Players.pas FXM routines this is a faithful C++ transliteration of, and
 fuxoft/fxmasm (format spec + disassembled Z80 playroutine).

 An .fxm file is a 6-byte header ('FXSM', then a little-endian origin/load
 address) followed by the module image. All pointers inside the image are
 absolute Spectrum addresses assuming the image is loaded at the origin, so
 we rebuild a 64K memory image and place the data at that origin -- exactly
 like the original Z80 playroutine and AY_Emul do.
 */
#include <cstring>
#include <map>
#include <string>
#include <vector>

/* Fuxoft AY Language tone table (indices 0..0x53) */
static const unsigned short FXM_Table[] = {
    0xfbf, 0xedc, 0xe07, 0xd3d, 0xc7f, 0xbcc, 0xb22, 0xa82, 0x9eb, 0x95d,
    0x8d6, 0x857, 0x7df, 0x76e, 0x703, 0x69f, 0x640, 0x5e6, 0x591, 0x541,
    0x4f6, 0x4ae, 0x46b, 0x42c, 0x3f0, 0x3b7, 0x382, 0x34f, 0x320, 0x2f3,
    0x2c8, 0x2a1, 0x27b, 0x257, 0x236, 0x216, 0x1f8, 0x1dc, 0x1c1, 0x1a8,
    0x190, 0x179, 0x164, 0x150, 0x13d, 0x12c, 0x11b, 0x10b, 0xfc,  0xee,
    0xe0,  0xd4,  0xc8,  0xbd,  0xb2,  0xa8,  0x9f,  0x96,  0x8d,  0x85,
    0x7e,  0x77,  0x70,  0x6a,  0x64,  0x5e,  0x59,  0x54,  0x4f,  0x4b,
    0x47,  0x43,  0x3f,  0x3b,  0x38,  0x35,  0x32,  0x2f,  0x2d,  0x2a,
    0x28,  0x25,  0x23,  0x21};

struct FXM_Channel_Parameters
{
    unsigned short Address_In_Pattern, Point_In_Sample, SamplePointer,
        Point_In_Ornament, OrnamentPointer, Ton;
    unsigned char FXM_Mixer, Note, Volume, Amplitude;
    signed char Transposit, Note_Skip_Counter, Sample_Tik_Counter;
    bool b0e, b1e, b2e, b3e;
    std::vector<unsigned short> Stek;
};

struct FXM_SongInfo
{
    unsigned char Noise_Base;
    unsigned char amad_andsix;
    FXM_Channel_Parameters A, B, C;
};

#define FXM_S ((FXM_SongInfo *)info.data)

/* little-endian word read with 64K wraparound (the image is exactly 64K) */
static inline unsigned short fxm_rdw(const unsigned char *m, unsigned int a)
{
    return (unsigned short)(m[a & 0xffff] | (m[(a + 1) & 0xffff] << 8));
}

/* Rebuild the 64K Spectrum memory image from the raw .fxm file and place the
   module body at its origin. Returns the origin address. */
static unsigned short FXM_BuildImage(unsigned char *dst, const unsigned char *file,
                                     unsigned long file_len)
{
    memset(dst, 0, 65536);
    if(file_len < 6)
        return 0;
    unsigned short origin = (unsigned short)(file[4] | (file[5] << 8));
    unsigned long len = file_len - 6;
    if(len > (unsigned long)(65536 - origin))
        len = 65536 - origin;
    memcpy(dst + origin, file + 6, len);
    return origin;
}

static void FXM_InitState(FXM_SongInfo *s, const unsigned char *module,
                          unsigned short origin)
{
    s->Noise_Base = 0;
    s->amad_andsix = 0x1f; /* FormatSpec for standalone .fxm files */
    FXM_Channel_Parameters *ch[3] = {&s->A, &s->B, &s->C};
    for(int i = 0; i < 3; i++)
    {
        FXM_Channel_Parameters &c = *ch[i];
        c.Address_In_Pattern = fxm_rdw(module, origin + i * 2);
        c.Point_In_Sample = 0;
        c.SamplePointer = 0;
        c.Point_In_Ornament = 0;
        c.OrnamentPointer = 0;
        c.Ton = 0;
        c.FXM_Mixer = 8;
        c.Note = 0;
        c.Volume = 0;
        c.Amplitude = 0;
        c.Transposit = 0;
        c.Note_Skip_Counter = 1;
        c.Sample_Tik_Counter = 0;
        c.b0e = c.b1e = c.b2e = c.b3e = false;
        c.Stek.clear();
    }
}

static inline void FXM_RealGetRegisters(FXM_Channel_Parameters &c)
{
    c.b2e = false;
    c.Amplitude = (c.Ton != 0) ? (c.Volume & 15) : 0;
}

static void FXM_GetRegisters(const unsigned char *m, FXM_Channel_Parameters &c)
{
    c.Sample_Tik_Counter--;
    if(c.Sample_Tik_Counter == 0)
    {
        for(;;)
        {
            unsigned char v = m[c.Point_In_Sample & 0xffff];
            if(v <= 0x1d)
            {
                c.Volume = v;
                c.Point_In_Sample++;
                c.Sample_Tik_Counter = (signed char)m[c.Point_In_Sample & 0xffff];
                c.Point_In_Sample++;
                break;
            }
            else if(v == 0x80)
            {
                c.Point_In_Sample = fxm_rdw(m, c.Point_In_Sample + 1);
            }
            else
            {
                c.Volume = (unsigned char)(v - 0x32);
                c.Point_In_Sample++;
                c.Sample_Tik_Counter = 1;
                break;
            }
        }
    }
    if(c.Ton != 0 && !c.b2e)
    {
        for(;;)
        {
            unsigned char v = m[c.Point_In_Ornament & 0xffff];
            if(v == 0x80)
            {
                c.Point_In_Ornament = fxm_rdw(m, c.Point_In_Ornament + 1);
            }
            else if(v == 0x82)
            {
                c.Point_In_Ornament++;
                c.b3e = true;
            }
            else if(v == 0x83)
            {
                c.Point_In_Ornament++;
                c.b3e = false;
            }
            else if(v == 0x84)
            {
                c.Point_In_Ornament++;
                c.FXM_Mixer ^= 9;
            }
            else
            {
                if(c.b3e)
                {
                    c.Note = (unsigned char)(c.Note + v);
                    unsigned char b = (c.Note > 0x53) ? 0x53 : c.Note;
                    c.Ton = FXM_Table[b];
                }
                else
                {
                    c.Ton = (unsigned short)(c.Ton + (signed char)v);
                }
                c.Point_In_Ornament++;
                break;
            }
        }
    }
    FXM_RealGetRegisters(c);
}

static void FXM_PatternInterpreter(const unsigned char *m, FXM_SongInfo *s,
                                   FXM_Channel_Parameters &c)
{
    c.Note_Skip_Counter--;
    if(c.Note_Skip_Counter != 0)
    {
        FXM_GetRegisters(m, c);
        return;
    }
    int guard = 0;
    for(;;)
    {
        /* guard against malformed modules with no note / unbalanced loops */
        if(++guard > 100000 || c.Stek.size() > 64)
            return;
        unsigned char v = m[c.Address_In_Pattern & 0xffff];
        if(v <= 0x7f)
        {
            if(v != 0)
            {
                c.Note = (unsigned char)(v - 1 + c.Transposit);
                unsigned char b = (c.Note > 0x53) ? 0x53 : c.Note;
                c.Ton = FXM_Table[b];
                c.b3e = false;
            }
            else
            {
                c.Ton = 0;
            }
            c.Address_In_Pattern++;
            c.Note_Skip_Counter = (signed char)m[c.Address_In_Pattern & 0xffff];
            c.Address_In_Pattern++;
            c.Point_In_Ornament = c.OrnamentPointer;
            if(!c.b1e)
            {
                c.b1e = c.b0e;
                c.Point_In_Sample = c.SamplePointer;
                c.Volume = m[c.Point_In_Sample & 0xffff];
                c.Point_In_Sample++;
                c.Sample_Tik_Counter = (signed char)m[c.Point_In_Sample & 0xffff];
                c.Point_In_Sample++;
                FXM_RealGetRegisters(c);
            }
            else
            {
                FXM_GetRegisters(m, c);
            }
            return;
        }
        switch(v)
        {
        case 0x80:
            c.Address_In_Pattern = fxm_rdw(m, c.Address_In_Pattern + 1);
            break;
        case 0x81:
            c.Stek.push_back((unsigned short)(c.Address_In_Pattern + 3));
            c.Address_In_Pattern = fxm_rdw(m, c.Address_In_Pattern + 1);
            break;
        case 0x82:
        {
            c.Address_In_Pattern++;
            unsigned short cnt = m[c.Address_In_Pattern & 0xffff];
            c.Address_In_Pattern++;
            c.Stek.push_back(cnt);
            c.Stek.push_back(c.Address_In_Pattern);
            break;
        }
        case 0x83:
        {
            size_t i = c.Stek.size();
            if(i < 2)
                return;
            c.Stek[i - 2] = (unsigned short)(c.Stek[i - 2] - 1);
            if((c.Stek[i - 2] & 255) != 0)
                c.Address_In_Pattern = c.Stek[i - 1];
            else
            {
                c.Stek.resize(i - 2);
                c.Address_In_Pattern++;
            }
            break;
        }
        case 0x84:
            c.Address_In_Pattern++;
            s->Noise_Base = m[c.Address_In_Pattern & 0xffff];
            c.Address_In_Pattern++;
            break;
        case 0x85:
            c.Address_In_Pattern++;
            c.FXM_Mixer = m[c.Address_In_Pattern & 0xffff];
            c.Address_In_Pattern++;
            break;
        case 0x86:
            c.Address_In_Pattern++;
            c.OrnamentPointer = fxm_rdw(m, c.Address_In_Pattern);
            c.Address_In_Pattern += 2;
            break;
        case 0x87:
            c.Address_In_Pattern++;
            c.SamplePointer = fxm_rdw(m, c.Address_In_Pattern);
            c.Address_In_Pattern += 2;
            break;
        case 0x88:
            c.Address_In_Pattern++;
            c.Transposit = (signed char)m[c.Address_In_Pattern & 0xffff];
            c.Address_In_Pattern++;
            break;
        case 0x89:
        {
            size_t i = c.Stek.size();
            if(i < 1)
                return;
            c.Address_In_Pattern = c.Stek[i - 1];
            c.Stek.resize(i - 1);
            break;
        }
        case 0x8a:
            c.Address_In_Pattern++;
            c.b0e = true;
            c.b1e = false;
            break;
        case 0x8b:
            c.Address_In_Pattern++;
            c.b0e = false;
            c.b1e = false;
            break;
        case 0x8c:
            c.Address_In_Pattern += 3;
            break;
        case 0x8d:
            c.Address_In_Pattern++;
            s->Noise_Base = (unsigned char)((s->Noise_Base +
                                             m[c.Address_In_Pattern & 0xffff]) &
                                            s->amad_andsix);
            c.Address_In_Pattern++;
            break;
        case 0x8e:
            c.Address_In_Pattern++;
            c.Transposit =
                (signed char)(c.Transposit + m[c.Address_In_Pattern & 0xffff]);
            c.Address_In_Pattern++;
            break;
        case 0x8f:
            c.Stek.push_back((unsigned short)(short)c.Transposit);
            c.Address_In_Pattern++;
            break;
        case 0x90:
        {
            size_t i = c.Stek.size();
            if(i < 1)
                return;
            c.Transposit = (signed char)c.Stek[i - 1];
            c.Stek.resize(i - 1);
            c.Address_In_Pattern++;
            break;
        }
        default:
            c.Address_In_Pattern++;
            break;
        }
    }
}

static void FXM_Step(const unsigned char *m, FXM_SongInfo *s)
{
    FXM_PatternInterpreter(m, s, s->A);
    FXM_PatternInterpreter(m, s, s->B);
    FXM_PatternInterpreter(m, s, s->C);
}

void FXM_Init(AYSongInfo &info)
{
    if(info.data)
    {
        delete(FXM_SongInfo *)info.data;
        info.data = 0;
    }
    info.data = (void *)new FXM_SongInfo;
    if(!info.data)
        return;
    unsigned short origin =
        FXM_BuildImage(info.module, info.file_data, info.file_len);
    FXM_InitState(FXM_S, info.module, origin);
}

void FXM_Play(AYSongInfo &info)
{
    unsigned char *m = info.module;
    FXM_SongInfo *s = FXM_S;

    FXM_Step(m, s);

    ay_writeay(&info, AY_NOISE_PERIOD, s->Noise_Base & 31);
    ay_writeay(&info, AY_CHNL_A_FINE, s->A.Ton & 0xff);
    ay_writeay(&info, AY_CHNL_A_COARSE, (s->A.Ton >> 8) & 0xf);
    ay_writeay(&info, AY_CHNL_B_FINE, s->B.Ton & 0xff);
    ay_writeay(&info, AY_CHNL_B_COARSE, (s->B.Ton >> 8) & 0xf);
    ay_writeay(&info, AY_CHNL_C_FINE, s->C.Ton & 0xff);
    ay_writeay(&info, AY_CHNL_C_COARSE, (s->C.Ton >> 8) & 0xf);
    ay_writeay(&info, AY_CHNL_A_VOL, s->A.Amplitude);
    ay_writeay(&info, AY_CHNL_B_VOL, s->B.Amplitude);
    ay_writeay(&info, AY_CHNL_C_VOL, s->C.Amplitude);

    unsigned char mixer = (unsigned char)((s->A.FXM_Mixer |
                                           (s->B.FXM_Mixer << 1) |
                                           (s->C.FXM_Mixer << 2)) &
                                          0x3f);
    ay_writeay(&info, AY_MIXER, mixer);
}

static void FXM_AppendSig(std::string &sig, const FXM_Channel_Parameters &c)
{
    /* serialize everything that influences future playback */
    sig.append((const char *)&c.Address_In_Pattern, sizeof(c.Address_In_Pattern));
    sig.append((const char *)&c.Point_In_Sample, sizeof(c.Point_In_Sample));
    sig.append((const char *)&c.SamplePointer, sizeof(c.SamplePointer));
    sig.append((const char *)&c.Point_In_Ornament, sizeof(c.Point_In_Ornament));
    sig.append((const char *)&c.OrnamentPointer, sizeof(c.OrnamentPointer));
    sig.append((const char *)&c.Ton, sizeof(c.Ton));
    sig.push_back((char)c.FXM_Mixer);
    sig.push_back((char)c.Note);
    sig.push_back((char)c.Volume);
    sig.push_back((char)c.Transposit);
    sig.push_back((char)c.Note_Skip_Counter);
    sig.push_back((char)c.Sample_Tik_Counter);
    sig.push_back((char)((c.b0e ? 1 : 0) | (c.b1e ? 2 : 0) | (c.b3e ? 8 : 0)));
    for(size_t i = 0; i < c.Stek.size(); i++)
        sig.append((const char *)&c.Stek[i], sizeof(unsigned short));
    sig.push_back('|');
}

void FXM_GetInfo(AYSongInfo &info)
{
    /* Length/loop detection: simulate playback on a private copy and find the
       first full-state repeat -- that is the loop boundary. */
    std::vector<unsigned char> img(65536);
    unsigned short origin =
        FXM_BuildImage(img.data(), info.file_data, info.file_len);
    FXM_SongInfo st;
    FXM_InitState(&st, img.data(), origin);

    std::map<std::string, long> seen;
    const long cap = 60000; /* 1/50s frames == 1200s */
    long frame = 0;
    info.Loop = 0;
    for(; frame < cap; frame++)
    {
        std::string sig;
        sig.push_back((char)st.Noise_Base);
        FXM_AppendSig(sig, st.A);
        FXM_AppendSig(sig, st.B);
        FXM_AppendSig(sig, st.C);
        std::map<std::string, long>::iterator it = seen.find(sig);
        if(it != seen.end())
        {
            info.Loop = it->second;
            break;
        }
        seen[sig] = frame;
        FXM_Step(img.data(), &st);
    }
    info.Length = frame;
}

void FXM_Cleanup(AYSongInfo &info)
{
    if(info.data)
    {
        delete(FXM_SongInfo *)info.data;
        info.data = 0;
    }
}

bool FXM_Detect(unsigned char *module, unsigned long length)
{
    if(length < 8)
        return false;
    return module[0] == 'F' && module[1] == 'X' && module[2] == 'S' &&
           module[3] == 'M';
}
