/*
 * GoatTracker v2 song loader + engine globals for the chipmachine plugin.
 *
 * The upstream loader (gsong.c loadsong) is entangled with the whole editor
 * model (pattern/table/order/instrument modules, the file browser and BME), so
 * instead of vendoring all of that we reproduce just the modern-format parse
 * path here, reading from memory into the same global song arrays the vendored
 * sequencer (gplay.c) consumes. Only GTS3/GTS4/GTS5 are handled -- the byte
 * layout is identical across those three -- which covers the modland
 * "GoatTracker 2" corpus. Legacy GTS!/GTS2 (which need makespeedtable table
 * conversion) are intentionally not supported.
 *
 * This file also owns the song-data array definitions, the player-config
 * globals (fixed at GoatTracker's defaults) and small stubs for the BME timing
 * / SDL flush hooks the sequencer calls.
 */

#include <stdio.h>
#include <string.h>

#include "gcommon.h"
#include "gsong.h"
#include "gplay.h"
#include "gsid.h"
#include "gt2_engine.h"

// --- Song data (upstream: defined in gsong.c) ---------------------------
INSTR instr[MAX_INSTR];
unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
unsigned char songorder[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];
char songname[MAX_STR];
char authorname[MAX_STR];
char copyrightname[MAX_STR];
int pattlen[MAX_PATT];
int songlen[MAX_SONGS][MAX_CHN];
int highestusedpattern;
int highestusedinstr;

// --- Player configuration (upstream: goattrk2.c), fixed at GT2 defaults --
unsigned multiplier = 1;
unsigned adparam = 0x0f00;
unsigned ntsc = 0;
unsigned sidmodel = 0;
unsigned finevibrato = 1;
unsigned optimizepulse = 1;
unsigned interpolate = 0;
unsigned residdelay = 0;
unsigned optimizerealtime = 1;
int followplay = 0;

// --- Editor play-position state (sequencer reads these; zero for playback) --
int espos[MAX_CHN];
int esend[MAX_CHN];
int epnum[MAX_CHN];

// --- BME timing / SDL flush hooks the sequencer calls -------------------
// In the editor these drive the raster timer and the HardSID flush thread;
// with reSID rendered inline they are no-ops.
void incrementtime(void) {}
void resettime(void) {}
void sound_suspend(void) {}
void sound_flush(void) {}

// --- countpatternlengths: verbatim from gsong.c -------------------------
// Computes pattlen[] (rows before ENDPATT), songlen[][] (order entries before a
// LOOPSONG/transpose marker) and highestusedpattern/instr.
void countpatternlengths(void)
{
  int c, d, e;

  highestusedpattern = 0;
  highestusedinstr = 0;
  for (c = 0; c < MAX_PATT; c++)
  {
    for (d = 0; d <= MAX_PATTROWS; d++)
    {
      if (pattern[c][d*4] == ENDPATT) break;
      if ((pattern[c][d*4] != REST) || (pattern[c][d*4+1]) || (pattern[c][d*4+2]) || (pattern[c][d*4+3]))
        highestusedpattern = c;
      if (pattern[c][d*4+1] > highestusedinstr) highestusedinstr = pattern[c][d*4+1];
    }
    pattlen[c] = d;
  }

  for (e = 0; e < MAX_SONGS; e++)
  {
    for (c = 0; c < MAX_CHN; c++)
    {
      for (d = 0; d < MAX_SONGLEN; d++)
      {
        if (songorder[e][c][d] >= LOOPSONG) break;
        if ((songorder[e][c][d] < REPEAT) && (songorder[e][c][d] > highestusedpattern))
          highestusedpattern = songorder[e][c][d];
      }
      songlen[e][c] = d;
    }
  }
}

// --- makespeedtable: adapted from gtable.c ------------------------------
// GTS2 (the older "3-table" format) stored portamento/vibrato/funktempo speeds
// inline in instruments and pattern commands; GTS3+ moved them into a 4th
// "speed table". On load we synthesise that speed table by interning each
// distinct speed value into ltable[STBL]/rtable[STBL] and returning its index.
// (The upstream editor-view call settableview() is dropped -- it is UI only.)
#define MST_NOFINEVIB 0
#define MST_FINEVIB   1
#define MST_FUNKTEMPO 2
#define MST_PORTAMENTO 3
#define MST_RAW       4

static int makespeedtable(unsigned data, int mode, int makenew)
{
  int c;
  unsigned char l = 0, r = 0;

  if (!data) return -1;

  switch (mode)
  {
    case MST_NOFINEVIB:
    l = (data & 0xf0) >> 4;
    r = (data & 0x0f) << 4;
    break;

    case MST_FINEVIB:
    l = (data & 0x70) >> 4;
    r = ((data & 0x0f) << 4) | ((data & 0x80) >> 4);
    break;

    case MST_FUNKTEMPO:
    l = (data & 0xf0) >> 4;
    r = data & 0x0f;
    break;

    case MST_PORTAMENTO:
    l = (data << 2) >> 8;
    r = (data << 2) & 0xff;
    break;

    case MST_RAW:
    r = data & 0xff;
    l = data >> 8;
    break;
  }

  if (makenew == 0)
  {
    for (c = 0; c < MAX_TABLELEN; c++)
    {
      if ((ltable[STBL][c] == l) && (rtable[STBL][c] == r))
        return c;
    }
  }

  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      ltable[STBL][c] = l;
      rtable[STBL][c] = r;
      return c;
    }
  }
  return -1;
}

// --- File reader (upstream: gfile.c / bme_end.c) ------------------------
static unsigned fread8(FILE* file)
{
  unsigned char b = 0;
  if (fread(&b, 1, 1, file) != 1) return 0;
  return b;
}

// loadsong_impl() reads from this path (upstream opens songfilename itself);
// loadedsongfilename is written by the tail of loadsong_impl.
char songfilename[1024];
char loadedsongfilename[1024];

// No-op editor stub referenced by the (dropped) view resets -- kept in case a
// vendored branch still calls it.
void settableview(int num, int pos) { (void)num; (void)pos; }

static void clear_arrays(void)
{
  int c, d;

  memset(instr, 0, sizeof instr);
  memset(ltable, 0, sizeof ltable);
  memset(rtable, 0, sizeof rtable);
  memset(songorder, 0, sizeof songorder);
  memset(pattern, 0, sizeof pattern);
  memset(songname, 0, sizeof songname);
  memset(authorname, 0, sizeof authorname);
  memset(copyrightname, 0, sizeof copyrightname);

  for (c = 1; c < MAX_INSTR; c++)
  {
    instr[c].gatetimer = 2 * multiplier;
    instr[c].firstwave = 0x9;
  }
  for (c = 0; c < MAX_PATT; c++)
    pattern[c][0] = ENDPATT;
  for (d = 0; d < MAX_SONGS; d++)
    for (c = 0; c < MAX_CHN; c++)
      songorder[d][c][0] = LOOPSONG;
}

// --- loadsong: vendored verbatim from gsong.c, adapted only by swapping the
// editor-model calls (clearsong -> clear_arrays, songchange dropped, the table-
// view reset dropped) and returning the success flag. Handles every GoatTracker
// song format: GTS! (v1), GTS2 (3-table), and GTS3/GTS4/GTS5, including the
// version-gated pulse-speed (<v2.4) and legato/nohr (<v2.5) fixups. -----------
static int loadsong_impl(void)
{
  int c;
  int ok = 0;
  char ident[4];
  FILE *handle;

  handle = fopen(songfilename, "rb");

  if (handle)
  {
    fread(ident, 4, 1, handle);
    if ((!memcmp(ident, "GTS3", 4)) || (!memcmp(ident, "GTS4", 4)) || (!memcmp(ident, "GTS5", 4)))
    {
      int d;
      int length;
      int amount;
      int loadsize;
      clear_arrays();
      ok = 1;

      // Read infotexts
      fread(songname, sizeof songname, 1, handle);
      fread(authorname, sizeof authorname, 1, handle);
      fread(copyrightname, sizeof copyrightname, 1, handle);

      // Read songorderlists
      amount = fread8(handle);
      for (d = 0; d < amount; d++)
      {
        for (c = 0; c < MAX_CHN; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          fread(songorder[d][c], loadsize, 1, handle);
        }
      }
      // Read instruments
      amount = fread8(handle);
      for (c = 1; c <= amount; c++)
      {
        instr[c].ad = fread8(handle);
        instr[c].sr = fread8(handle);
        instr[c].ptr[WTBL] = fread8(handle);
        instr[c].ptr[PTBL] = fread8(handle);
        instr[c].ptr[FTBL] = fread8(handle);
        instr[c].ptr[STBL] = fread8(handle);
        instr[c].vibdelay = fread8(handle);
        instr[c].gatetimer = fread8(handle);
        instr[c].firstwave = fread8(handle);
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (c = 0; c < MAX_TABLES; c++)
      {
        loadsize = fread8(handle);
        fread(ltable[c], loadsize, 1, handle);
        fread(rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (c = 0; c < amount; c++)
      {
        length = fread8(handle) * 4;
        fread(pattern[c], length, 1, handle);
      }
      countpatternlengths();
      /* songchange(): editor view state only, skipped */
    }

    // Goattracker v2.xx (3-table) import
    if (!memcmp(ident, "GTS2", 4))
    {
      int d;
      int length;
      int amount;
      int loadsize;
      clear_arrays();
      ok = 1;

      // Read infotexts
      fread(songname, sizeof songname, 1, handle);
      fread(authorname, sizeof authorname, 1, handle);
      fread(copyrightname, sizeof copyrightname, 1, handle);

      // Read songorderlists
      amount = fread8(handle);
      for (d = 0; d < amount; d++)
      {
        for (c = 0; c < MAX_CHN; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          fread(songorder[d][c], loadsize, 1, handle);
        }
      }
      // Read instruments
      amount = fread8(handle);
      for (c = 1; c <= amount; c++)
      {
        instr[c].ad = fread8(handle);
        instr[c].sr = fread8(handle);
        instr[c].ptr[WTBL] = fread8(handle);
        instr[c].ptr[PTBL] = fread8(handle);
        instr[c].ptr[FTBL] = fread8(handle);
        instr[c].vibdelay = fread8(handle);
        instr[c].ptr[STBL] = makespeedtable(fread8(handle), finevibrato, 0) + 1;
        instr[c].gatetimer = fread8(handle);
        instr[c].firstwave = fread8(handle);
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (c = 0; c < MAX_TABLES-1; c++)
      {
        loadsize = fread8(handle);
        fread(ltable[c], loadsize, 1, handle);
        fread(rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (c = 0; c < amount; c++)
      {
        int d;
        length = fread8(handle) * 4;
        fread(pattern[c], length, 1, handle);

        // Convert speedtable-requiring commands
        for (d = 0; d < length; d++)
        {
          switch (pattern[c][d*4+2])
          {
            case CMD_FUNKTEMPO:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_FUNKTEMPO, 0) + 1;
            break;

            case CMD_PORTAUP:
            case CMD_PORTADOWN:
            case CMD_TONEPORTA:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_PORTAMENTO, 0) + 1;
            break;

            case CMD_VIBRATO:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], finevibrato, 0) + 1;
            break;
          }
        }
      }
      countpatternlengths();
      /* songchange(): editor view state only, skipped */
    }
    // Goattracker 1.xx import
    if (!memcmp(ident, "GTS!", 4))
    {
      int d;
      int length;
      int amount;
      int loadsize;
      int fw = 0;
      int fp = 0;
      int ff = 0;
      int fi = 0;
      int numfilter = 0;
      unsigned char filtertable[256];
      unsigned char filtermap[64];
      int arpmap[32][256];
      unsigned char pulse[32], pulseadd[32], pulselimitlow[32], pulselimithigh[32];
      int filterjumppos[64];

      clear_arrays();
      ok = 1;

      // Read infotexts
      fread(songname, sizeof songname, 1, handle);
      fread(authorname, sizeof authorname, 1, handle);
      fread(copyrightname, sizeof copyrightname, 1, handle);

      // Read songorderlists
      amount = fread8(handle);
      for (d = 0; d < amount; d++)
      {
        for (c = 0; c < MAX_CHN; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          fread(songorder[d][c], loadsize, 1, handle);
        }
      }

      // Convert instruments
      for (c = 1; c < 32; c++)
      {
        unsigned char wavelen;

        instr[c].ad = fread8(handle);
        instr[c].sr = fread8(handle);
        pulse[c] = fread8(handle);
        pulseadd[c] = fread8(handle);
        pulselimitlow[c] = fread8(handle);
        pulselimithigh[c] = fread8(handle);
        instr[c].ptr[FTBL] = fread8(handle); // Will be converted later
        if (instr[c].ptr[FTBL] > numfilter) numfilter = instr[c].ptr[FTBL];
        if (pulse[c] & 1) instr[c].gatetimer |= 0x80; // "No hardrestart" flag
        pulse[c] &= 0xfe;
        wavelen = fread8(handle)/2;
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, handle);
        instr[c].ptr[WTBL] = fw+1;

        // Convert wavetable
        for (d = 0; d < wavelen; d++)
        {
          if (fw < MAX_TABLELEN)
          {
            ltable[WTBL][fw] = fread8(handle);
            rtable[WTBL][fw] = fread8(handle);
            if (ltable[WTBL][fw] == 0xff)
              if (rtable[WTBL][fw]) rtable[WTBL][fw] += instr[c].ptr[WTBL]-1;
            if ((ltable[WTBL][fw] >= 0x8) && (ltable[WTBL][fw] <= 0xf))
              ltable[WTBL][fw] |= 0xe0;
            fw++;
          }
          else
          {
            fread8(handle);
            fread8(handle);
          }
        }

        // Remove empty wavetable afterwards
        if ((wavelen == 2) && (!ltable[WTBL][fw-2]) && (!rtable[WTBL][fw-2]))
        {
          instr[c].ptr[WTBL] = 0;
          fw -= 2;
          ltable[WTBL][fw] = 0;
          rtable[WTBL][fw] = 0;
          ltable[WTBL][fw+1] = 0;
          rtable[WTBL][fw+1] = 0;
        }

        // Convert pulsetable
        if (pulse[c])
        {
          int pulsetime, pulsedist, hlpos;

          // Check for duplicate pulse settings
          for (d = 1; d < c; d++)
          {
            if ((pulse[d] == pulse[c]) && (pulseadd[d] == pulseadd[c]) && (pulselimitlow[d] == pulselimitlow[c]) &&
                (pulselimithigh[d] == pulselimithigh[c]))
            {
              instr[c].ptr[PTBL] = instr[d].ptr[PTBL];
              goto PULSEDONE;
            }
          }

          // Initial pulse setting
          if (fp >= MAX_TABLELEN) goto PULSEDONE;
          instr[c].ptr[PTBL] = fp+1;
          ltable[PTBL][fp] = 0x80 | (pulse[c] >> 4);
          rtable[PTBL][fp] = pulse[c] << 4;
          fp++;

          // Pulse modulation
          if (pulseadd[c])
          {
            int startpulse = pulse[c]*16;
            int currentpulse = pulse[c]*16;
            // Phase 1: From startpos to high limit
            pulsedist = pulselimithigh[c]*16 - currentpulse;
            if (pulsedist > 0)
            {
              pulsetime = pulsedist/pulseadd[c];
              currentpulse += pulsetime*pulseadd[c];
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                ltable[PTBL][fp] = acttime;
                rtable[PTBL][fp] = pulseadd[c] / 2;
                fp++;
                pulsetime -= acttime;
              }
            }

            hlpos = fp;
            // Phase 2: from high limit to low limit
            pulsedist = currentpulse - pulselimitlow[c]*16;
            if (pulsedist > 0)
            {
              pulsetime = pulsedist/pulseadd[c];
              currentpulse -= pulsetime*pulseadd[c];
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                ltable[PTBL][fp] = acttime;
                rtable[PTBL][fp] = -(pulseadd[c] / 2);
                fp++;
                pulsetime -= acttime;
              }
            }

            // Phase 3: from low limit back to startpos/high limit
            if ((startpulse < pulselimithigh[c]*16) && (startpulse > currentpulse))
            {
              pulsedist = startpulse - currentpulse;
              if (pulsedist > 0)
              {
                pulsetime = pulsedist/pulseadd[c];
                while (pulsetime)
                {
                  int acttime = pulsetime;
                  if (acttime > 127) acttime = 127;
                  if (fp >= MAX_TABLELEN) goto PULSEDONE;
                  ltable[PTBL][fp] = acttime;
                  rtable[PTBL][fp] = pulseadd[c] / 2;
                  fp++;
                  pulsetime -= acttime;
                }
              }
              // Pulse jump back to beginning
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              ltable[PTBL][fp] = 0xff;
              rtable[PTBL][fp] = instr[c].ptr[PTBL] + 1;
              fp++;
            }
            else
            {
              pulsedist = pulselimithigh[c]*16 - currentpulse;
              if (pulsedist > 0)
              {
                pulsetime = pulsedist/pulseadd[c];
                while (pulsetime)
                {
                  int acttime = pulsetime;
                  if (acttime > 127) acttime = 127;
                  if (fp >= MAX_TABLELEN) goto PULSEDONE;
                  ltable[PTBL][fp] = acttime;
                  rtable[PTBL][fp] = pulseadd[c] / 2;
                  fp++;
                  pulsetime -= acttime;
                }
              }
              // Pulse jump back to beginning
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              ltable[PTBL][fp] = 0xff;
              rtable[PTBL][fp] = hlpos + 1;
              fp++;
            }
          }
          else
          {
            // Pulse stopped
            if (fp >= MAX_TABLELEN) goto PULSEDONE;
            ltable[PTBL][fp] = 0xff;
            rtable[PTBL][fp] = 0;
            fp++;
          }
          PULSEDONE: {}
        }
      }
      // Convert patterns
      amount = fread8(handle);
      for (c = 0; c < amount; c++)
      {
        length = fread8(handle);
        for (d = 0; d < length/3; d++)
        {
          unsigned char note, cmd, data, instr;
          note = fread8(handle);
          cmd = fread8(handle);
          data = fread8(handle);
          instr = cmd >> 3;
          cmd &= 7;

          switch(note)
          {
            default:
            note += FIRSTNOTE;
            if (note > LASTNOTE) note = REST;
            break;

            case OLDKEYOFF:
            note = KEYOFF;
            break;

            case OLDREST:
            note = REST;
            break;

            case ENDPATT:
            break;
          }
          switch(cmd)
          {
            case 5:
            cmd = CMD_SETFILTERPTR;
            if (data > numfilter) numfilter = data;
            break;

            case 7:
            if (data < 0xf0)
              cmd = CMD_SETTEMPO;
            else
            {
              cmd = CMD_SETMASTERVOL;
              data &= 0x0f;
            }
            break;
          }
          pattern[c][d*4] = note;
          pattern[c][d*4+1] = instr;
          pattern[c][d*4+2] = cmd;
          pattern[c][d*4+3] = data;
        }
      }
      countpatternlengths();
      fi = highestusedinstr + 1;
      /* songchange(): editor view state only, skipped */

      // Read filtertable
      fread(filtertable, 256, 1, handle);

      // Convert filtertable
      for (c = 0; c < 64; c++)
      {
        filterjumppos[c] = -1;
        filtermap[c] = 0;
        if (filtertable[c*4+3] > numfilter) numfilter = filtertable[c*4+3];
      }

      if (numfilter > 63) numfilter = 63;

      for (c = 1; c <= numfilter; c++)
      {
        filtermap[c] = ff+1;

        if (filtertable[c*4]|filtertable[c*4+1]|filtertable[c*4+2]|filtertable[c*4+3])
        {
          // Filter set
          if (filtertable[c*4])
          {
            ltable[FTBL][ff] = 0x80 + (filtertable[c*4+1] & 0x70);
            rtable[FTBL][ff] = filtertable[c*4];
            ff++;
            if (filtertable[c*4+2])
            {
              ltable[FTBL][ff] = 0x00;
              rtable[FTBL][ff] = filtertable[c*4+2];
              ff++;
            }
          }
          else
          {
            // Filter modulation
            int time = filtertable[c*4+1];

            while (time)
            {
              int acttime = time;
              if (acttime > 127) acttime = 127;
              ltable[FTBL][ff] = acttime;
              rtable[FTBL][ff] = filtertable[c*4+2];
              ff++;
              time -= acttime;
            }
          }

          // Jump to next step: unnecessary if follows directly
          if (filtertable[c*4+3] != c+1)
          {
            filterjumppos[c] = ff;
            ltable[FTBL][ff] = 0xff;
            rtable[FTBL][ff] = filtertable[c*4+3]; // Fix the jump later
            ff++;
          }
        }
      }

      // Now fix jumps as the filterstep mapping is known
      for (c = 1; c <= numfilter; c++)
      {
        if (filterjumppos[c] != -1)
          rtable[FTBL][filterjumppos[c]] = filtermap[rtable[FTBL][filterjumppos[c]]];
      }

      // Fix filterpointers in instruments
      for (c = 1; c < 32; c++)
        instr[c].ptr[FTBL] = filtermap[instr[c].ptr[FTBL]];

      // Now fix pattern commands
      memset(arpmap, 0, sizeof arpmap);
      for (c = 0; c < MAX_PATT; c++)
      {
        unsigned char i = 0;
        for (d = 0; d <= MAX_PATTROWS; d++)
        {
          if (pattern[c][d*4+1]) i = pattern[c][d*4+1];

          // Convert portamento & vibrato
          if (pattern[c][d*4+2] == CMD_PORTAUP)
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_PORTAMENTO, 0) + 1;
          if (pattern[c][d*4+2] == CMD_PORTADOWN)
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_PORTAMENTO, 0) + 1;
          if (pattern[c][d*4+2] == CMD_TONEPORTA)
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_PORTAMENTO, 0) + 1;
          if (pattern[c][d*4+2] == CMD_VIBRATO)
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_NOFINEVIB, 0) + 1;

          // Convert filterjump
          if (pattern[c][d*4+2] == CMD_SETFILTERPTR)
            pattern[c][d*4+3] = filtermap[pattern[c][d*4+3]];

          // Convert funktempo
          if ((pattern[c][d*4+2] == CMD_SETTEMPO) && (!pattern[c][d*4+3]))
          {
            pattern[c][d*4+2] = CMD_FUNKTEMPO;
            pattern[c][d*4+3] = makespeedtable((filtertable[2] << 4) | (filtertable[3] & 0x0f), MST_FUNKTEMPO, 0) + 1;
          }
          // Convert arpeggio
          if ((pattern[c][d*4+2] == CMD_DONOTHING) && (pattern[c][d*4+3]))
          {
            // Must be in conjunction with a note
            if ((pattern[c][d*4] >= FIRSTNOTE) && (pattern[c][d*4] <= LASTNOTE))
            {
              unsigned char param = pattern[c][d*4+3];
              if (i)
              {
                // Old arpeggio
                if (arpmap[i][param])
                {
                  // As command, or as instrument?
                  if (arpmap[i][param] < 256)
                  {
                    pattern[c][d*4+2] = CMD_SETWAVEPTR;
                    pattern[c][d*4+3] = arpmap[i][param];
                  }
                  else
                  {
                    pattern[c][d*4+1] = arpmap[i][param] - 256;
                    pattern[c][d*4+3] = 0;
                  }
                }
                else
                {
                  int e;
                  unsigned char arpstart;
                  unsigned char arploop;

                  // New arpeggio
                  // Copy first the instrument's wavetable up to loop/end point
                  arpstart = fw + 1;
                  if (instr[i].ptr[WTBL])
                  {
                    for (e = instr[i].ptr[WTBL]-1;; e++)
                    {
                      if (ltable[WTBL][e] == 0xff) break;
                      if (fw < MAX_TABLELEN)
                      {
                        ltable[WTBL][fw] = ltable[WTBL][e];
                        fw++;
                      }
                    }
                  }
                  // Then make the arpeggio
                  arploop = fw + 1;
                  if (fw < MAX_TABLELEN-3)
                  {
                    ltable[WTBL][fw] = (param & 0x80) >> 7;
                    rtable[WTBL][fw] = (param  & 0x70) >> 4;
                    fw++;
                    ltable[WTBL][fw] = (param & 0x80) >> 7;
                    rtable[WTBL][fw] = (param & 0xf);
                    fw++;
                    ltable[WTBL][fw] = (param & 0x80) >> 7;
                    rtable[WTBL][fw] = 0;
                    fw++;
                    ltable[WTBL][fw] = 0xff;
                    rtable[WTBL][fw] = arploop;
                    fw++;

                    // Create new instrument if possible
                    if (fi < MAX_INSTR)
                    {
                      arpmap[i][param] = fi + 256;
                      instr[fi] = instr[i];
                      instr[fi].ptr[WTBL] = arpstart;
                      // Add arpeggio parameter to new instrument name
                      if (strlen(instr[fi].name) < MAX_INSTRNAMELEN-3)
                      {
                        char arpname[8];
                        sprintf(arpname, "0%02X", param&0x7f);
                        strcat(instr[fi].name, arpname);
                      }
                      fi++;
                    }
                    else
                    {
                      arpmap[i][param] = arpstart;
                    }
                  }

                  if (arpmap[i][param])
                  {
                    // As command, or as instrument?
                    if (arpmap[i][param] < 256)
                    {
                      pattern[c][d*4+2] = CMD_SETWAVEPTR;
                      pattern[c][d*4+3] = arpmap[i][param];
                    }
                    else
                    {
                      pattern[c][d*4+1] = arpmap[i][param] - 256;
                      pattern[c][d*4+3] = 0;
                    }
                  }
                }
              }
            }
            // If arpeggio could not be converted, databyte zero
            if (!pattern[c][d*4+2])
              pattern[c][d*4+3] = 0;
          }
        }
      }
    }
    fclose(handle);
  }
  if (ok)
  {
    strcpy(loadedsongfilename, songfilename);

    // (editor "Reset table views" loop dropped)

    // Convert pulsemodulation speed of < v2.4 songs
    if (ident[3] < '4')
    {
      for (c = 0; c < MAX_TABLELEN; c++)
      {
        if ((ltable[PTBL][c] < 0x80) && (rtable[PTBL][c]))
        {
          int speed = ((signed char)rtable[PTBL][c]);
          speed <<= 1;
          if (speed > 127) speed = 127;
          if (speed < -128) speed = -128;
          rtable[PTBL][c] = speed;
        }
      }
    }

    // Convert old legato/nohr parameters
    if (ident[3] < '5')
    {
        for (c = 1; c < MAX_INSTR; c++)
        {
            if (instr[c].firstwave >= 0x80)
            {
                instr[c].gatetimer |= 0x80;
                instr[c].firstwave &= 0x7f;
            }
            if (!instr[c].firstwave) instr[c].gatetimer |= 0x40;
        }
    }
  }

  return ok;
}

// Public entry: load a GoatTracker .sng from a file path. Returns the number of
// subsongs (order lists that carry real data) on success, 0 on failure.
int gt2_load(const char* filename)
{
  int d, n = 0;

  if (!filename) return 0;
  strncpy(songfilename, filename, sizeof(songfilename) - 1);
  songfilename[sizeof(songfilename) - 1] = 0;

  if (!loadsong_impl()) return 0;

  // countpatternlengths() (run inside every format branch) filled songlen[][];
  // subsongs are loaded contiguously from index 0, so count while channel 0 has
  // order data.
  for (d = 0; d < MAX_SONGS; d++)
  {
    if (songlen[d][0] > 0) n++;
    else break;
  }
  return n;
}
