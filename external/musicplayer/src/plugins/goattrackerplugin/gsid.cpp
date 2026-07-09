/*
 * GOATTRACKER reSID interface
 *
 * Trimmed for the chipmachine goattrackerplugin: the upstream file could drive
 * either reSID (SID) or reSID-fp (SIDFP). We only vendor plain reSID, so the
 * SIDFP path and its resid-fp dependency have been removed. Everything else is
 * the original GoatTracker v2 sid_init / sid_fillbuffer / sid_getorder logic.
 */

#define GSID_C

#include <stdlib.h>
#include <stdio.h>
#include "resid/sid.h"

extern "C" {

#include "gsid.h"
#include "gsound.h"

int clockrate;
int samplerate;
unsigned char sidreg[NUMSIDREGS];
unsigned char sidorder[] =
  {0x15,0x16,0x18,0x17,
   0x02,0x03,0x06,0x05,0x00,0x01,0x04,
   0x09,0x0a,0x0d,0x0c,0x07,0x08,0x0b,
   0x10,0x11,0x14,0x13,0x0e,0x0f,0x12};

unsigned char altsidorder[] =
  {0x15,0x16,0x18,0x17,
   0x04,0x00,0x01,0x02,0x03,0x05,0x06,
   0x0b,0x07,0x08,0x09,0x0a,0x0c,0x0d,
   0x12,0x0e,0x0f,0x10,0x11,0x13,0x14};

SID *sid = 0;

// Kept only to satisfy the extern in gsid.h; the SIDFP filter tuning it fed is
// gone with the resid-fp path.
FILTERPARAMS filterparams =
  {0.50f, 3.3e6f, 1.0e-4f,
   1147036.4394268463f, 274228796.97550374f, 1.0066634233403395f, 16125.154840564108f,
   5.5f, 20.f,
   0.9613160610660189f};

extern unsigned residdelay;
extern unsigned adparam;

void sid_init(int speed, unsigned m, unsigned ntsc, unsigned interpolate, unsigned customclockrate, unsigned usefp)
{
  int c;

  (void)usefp;

  if (ntsc) clockrate = NTSCCLOCKRATE;
    else clockrate = PALCLOCKRATE;

  if (customclockrate)
    clockrate = customclockrate;

  samplerate = speed;

  if (!sid) sid = new SID;

  switch(interpolate)
  {
    case 0:
    sid->set_sampling_parameters(clockrate, SAMPLE_FAST, speed);
    break;

    default:
    sid->set_sampling_parameters(clockrate, SAMPLE_INTERPOLATE, speed);
    break;
  }

  sid->reset();
  for (c = 0; c < NUMSIDREGS; c++)
  {
    sidreg[c] = 0x00;
  }
  if (m == 1)
    sid->set_chip_model(MOS8580);
  else
    sid->set_chip_model(MOS6581);
}

unsigned char sid_getorder(unsigned char index)
{
  if (adparam >= 0xf000)
    return altsidorder[index];
  else
    return sidorder[index];
}

int sid_fillbuffer(short *ptr, int samples)
{
  int tdelta;
  int tdelta2;
  int result = 0;
  int total = 0;
  int c;

  int badline = rand() % NUMSIDREGS;

  tdelta = clockrate * samples / samplerate;
  if (tdelta <= 0) return total;

  for (c = 0; c < NUMSIDREGS; c++)
  {
    unsigned char o = sid_getorder(c);

    // Extra delay for loading the waveform (and mt_chngate,x)
    if ((o == 4) || (o == 11) || (o == 18))
    {
      tdelta2 = SIDWAVEDELAY;
      result = sid->clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= SIDWAVEDELAY;
    }

    // Possible random badline delay once per writing
    if ((badline == c) && (residdelay))
    {
      tdelta2 = residdelay;
      result = sid->clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= residdelay;
    }

    sid->write(o, sidreg[o]);

    tdelta2 = SIDWRITEDELAY;
    result = sid->clock(tdelta2, ptr, samples);
    total += result;
    ptr += result;
    samples -= result;
    tdelta -= SIDWRITEDELAY;

    if (tdelta <= 0) return total;
  }

  result = sid->clock(tdelta, ptr, samples);
  total += result;
  ptr += result;
  samples -= result;

  // Loop extra cycles until all samples produced
  while (samples)
  {
    tdelta = clockrate * samples / samplerate;
    if (tdelta <= 0) return total;

    result = sid->clock(tdelta, ptr, samples);
    total += result;
    ptr += result;
    samples -= result;
  }

  return total;
}

}
