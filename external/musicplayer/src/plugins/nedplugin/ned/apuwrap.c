// these functions call the actual APU functions
// function names are equivalent to  those in "Nesess"

#include "blarggAPU.h"

// Some variables, filled in by nt2.c...

#define NUM_CHANNELS        5

void    (*CallInsideMixer)();
// NOTE: NT2 uses PAL only playback in the tracker --thefox
// so we won't be using these.. ;-)
int     VBlankSpeed;
int     PlayBackRate;
int     ChannelAmplitude[NUM_CHANNELS];
void    *UpperPrgPage[4];

int SND_SoundSetup(void);
int SND_SoundShutdown(void);

// Note: make sure this is called *after* CallInsideMixer is set
int NesessInitialize(void)
{
    apuSetup(CallInsideMixer);
    SND_SoundSetup();
    return 1;
}

int NesessShutdown(void)
{
    apuShutdown();
    SND_SoundShutdown();
    return 1;
}

void Mix(void *Buffer, int Length, int Is16Bit)
{
    apuRender(Buffer, Length);
}

void Write2SoundReg(int RegAddr, int Val)
{
    apuWriteReg(RegAddr, Val);
}

int ReadSoundReg(int RegAddr)
{
    return apuReadReg(RegAddr);
}
