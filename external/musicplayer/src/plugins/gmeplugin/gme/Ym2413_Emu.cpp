
// Real YM2413 (OPLL) FM sound chip emulator. Wraps the vendored emu2413 core
// (Digital Sound Antiques / Okazaki, copied from libkss). This replaces the
// former no-op stub, so SMS FM (.sgc) and VGM/VGZ FM voices now render.
//
// The OPLL_* symbols are renamed to GME_OPLL_* at compile time (see gmeplugin
// CMakeLists.txt) to avoid clashing with s98plugin's / famitracker's copies of
// the same core at final static link.

// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/

#include "Ym2413_Emu.h"

#include "emu2413.h"

Ym2413_Emu::Ym2413_Emu() : opll( 0 ) { }

Ym2413_Emu::~Ym2413_Emu()
{
	if ( opll )
		OPLL_delete( opll );
}

int Ym2413_Emu::set_rate( double sample_rate, double clock_rate )
{
	if ( opll )
	{
		OPLL_delete( opll );
		opll = 0;
	}

	opll = OPLL_new( (uint32_t) (clock_rate + 0.5), (uint32_t) (sample_rate + 0.5) );
	if ( !opll )
		return 1;

	OPLL_setChipType( opll, 0 ); // 0 = YM2413 (SMS/MSX), not VRC7
	OPLL_reset( opll );
	return 0;
}

void Ym2413_Emu::reset()
{
	OPLL_reset( opll );
}

void Ym2413_Emu::write( int addr, int data )
{
	OPLL_writeReg( opll, addr, data );
}

void Ym2413_Emu::mute_voices( int mask )
{
	// GME and emu2413 share the same convention: bit n masks voice n
	// (bits 0..8 tone channels, 9..13 rhythm).
	OPLL_setMask( opll, mask );
}

void Ym2413_Emu::run( int pair_count, sample_t* out )
{
	while ( pair_count-- )
	{
		int32_t s [2];
		OPLL_calcStereo( opll, s );

		int l = s [0];
		int r = s [1];
		if ( l < -32768 ) l = -32768; else if ( l > 32767 ) l = 32767;
		if ( r < -32768 ) r = -32768; else if ( r > 32767 ) r = 32767;

		out [0] = (sample_t) l;
		out [1] = (sample_t) r;
		out += 2;
	}
}
