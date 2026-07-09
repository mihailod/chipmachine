/* This player module was ported from:
 AY-3-8910/12 Emulator
 Version 2.8 for Windows 95
 Author Sergey Vladimirovich Bulba
 (c)1999-2005 S.V.Bulba

 AY Amadeus (.amad) player.

 Modland's "AY Amadeus" tunes (Frantisek Fuka aka Fuxoft, Patrik Rak) are
 stored in the ZXAY container with the 'AMAD' type tag. Per Bulba's AY_Emul,
 the AMAD type is the direct analog of FXM (Fuxoft AY Language): the body is a
 Fuxoft-AY-language module that is interpreted exactly like a standalone .fxm,
 so this reuses the whole FXMPlay.h engine. Only the container wrapper and the
 per-file noise AND-mask (FormatSpec / amad_andsix) differ; AY_Emul forces that
 mask to 0x1f for standalone .fxm, whereas an .amad carries it in the file.

 Container layout (AY pointers are signed 16-bit big-endian, each relative to
 the file offset of the field that holds it), transliterated from AY_Emul's
 Players.pas (OpenAYFile + LoadTrackerModule):
   +0x00  'ZXAY'
   +0x04  'AMAD'
   +0x10  NumOfSongs (count-1), FirstSong
   +0x12  PSongsStructure -> SongStructure[]
   SongStructure (4 bytes): PSongName, PSongData
   SongData (AMAD):
     +0  Address    (big-endian Spectrum load origin)
     +2  FormatSpec (1)  -> amad_andsix noise mask
     +3  Speed      (1)
     +4  Time       (word)
   The module body is file[SongData + 14 .. EOF] and is loaded into the 64K
   image at Address; the three channel start pointers are then read from the
   image at Address+0/+2/+4, exactly as for .fxm. (The +14 is AY_Emul's
   SizeOf(TSongData)=14 skip, mirroring its Seek(Offset+6) after inc(Offset,8).)

 We play song FirstSong (modland .amad modules are single-song).
 */
#include "FXMPlay.h"

/* Resolve the AMAD container down to the load origin, noise mask and the file
   offset where the module body begins. Returns false on any structural problem. */
static bool AMAD_Parse(const unsigned char *file, unsigned long len,
                       unsigned short *out_origin, unsigned char *out_andsix,
                       unsigned long *out_body_off)
{
    if(len < 0x14)
        return false;
    if(memcmp(file, "ZXAYAMAD", 8) != 0)
        return false;
    unsigned char firstSong = file[0x11];
    /* PSongsStructure: signed big-endian rel ptr stored at field offset 0x12 */
    int sptr = (short)((file[0x12] << 8) | file[0x13]);
    long songStructs = 0x12 + sptr;
    long pSongDataField = songStructs + (long)firstSong * 4 + 2;
    if(pSongDataField < 0 || pSongDataField + 2 > (long)len)
        return false;
    int dptr = (short)((file[pSongDataField] << 8) | file[pSongDataField + 1]);
    long songData = pSongDataField + dptr;
    if(songData < 0 || songData + 14 > (long)len)
        return false;
    *out_origin = (unsigned short)((file[songData] << 8) | file[songData + 1]);
    *out_andsix = file[songData + 2];
    *out_body_off = (unsigned long)(songData + 14);
    return true;
}

/* Rebuild the 64K Spectrum image from an .amad file: place the module body at
   its load origin and report the noise mask. Returns the origin (0 on failure,
   which yields silence rather than a crash). */
static unsigned short AMAD_BuildImage(unsigned char *dst, const unsigned char *file,
                                      unsigned long len, unsigned char *andsix)
{
    memset(dst, 0, 65536);
    unsigned short origin = 0;
    unsigned char a6 = 0x1f;
    unsigned long body = 0;
    if(!AMAD_Parse(file, len, &origin, &a6, &body))
    {
        *andsix = 0x1f;
        return 0;
    }
    unsigned long blen = len - body;
    if(blen > (unsigned long)(65536 - origin))
        blen = 65536 - origin;
    memcpy(dst + origin, file + body, blen);
    *andsix = a6;
    return origin;
}

void AMAD_Init(AYSongInfo &info)
{
    if(info.data)
    {
        delete(FXM_SongInfo *)info.data;
        info.data = 0;
    }
    info.data = (void *)new FXM_SongInfo;
    if(!info.data)
        return;
    unsigned char andsix = 0x1f;
    unsigned short origin =
        AMAD_BuildImage(info.module, info.file_data, info.file_len, &andsix);
    FXM_InitState(FXM_S, info.module, origin);
    FXM_S->amad_andsix = andsix;
}

void AMAD_GetInfo(AYSongInfo &info)
{
    /* Length/loop detection: identical to FXM_GetInfo but driven from the AMAD
       container's image + per-file noise mask. */
    std::vector<unsigned char> img(65536);
    unsigned char andsix = 0x1f;
    unsigned short origin =
        AMAD_BuildImage(img.data(), info.file_data, info.file_len, &andsix);
    FXM_SongInfo st;
    FXM_InitState(&st, img.data(), origin);
    st.amad_andsix = andsix;

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

bool AMAD_Detect(unsigned char *module, unsigned long length)
{
    if(length < 8)
        return false;
    return memcmp(module, "ZXAYAMAD", 8) == 0;
}
