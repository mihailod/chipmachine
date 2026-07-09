
// Nes_Snd_Emu 0.1.7. http://www.slack.net/~ant/libs/

#include "Simple_Apu_PAL.h"

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

// Modified by thefox 3.2.2007 for PAL

static int null_dmc_reader( void*, cpu_addr_t )
{
	return 0x55; // causes dmc sample to be flat
}

Simple_Apu::Simple_Apu()
{
	time = 0;
    // chipmachine: NerdTracker II tunes are NTSC-targeted (they compile to NTSC
    // NSFs), so we clock the APU at NTSC speed for authentic pitch -- thefox's
    // v1.0 used the PAL clock below, which is why notes came out ~a semitone
    // flat. NT2's editor still advances the song at 50 Hz, so we keep a 50 Hz
    // tick by setting frame_length = NTSC clock / 50 (1789773/50).
    //     PAL (v1.0): frame_length = 33247; clock_rate(1662607)  -> 50 Hz, flat
    frame_length = 35795;          // NTSC clock / 50 Hz tick
    //frame_length = 33247;        // original PAL value
	apu.dmc_reader( null_dmc_reader, NULL );
}

Simple_Apu::~Simple_Apu()
{
}

void Simple_Apu::dmc_reader( int (*f)( void* user_data, cpu_addr_t ), void* p )
{
	assert( f );
	apu.dmc_reader( f, p );
}

blargg_err_t Simple_Apu::sample_rate( long rate )
{
	apu.output( &buf );
    buf.clock_rate( 1789773 );     // NTSC (was PAL 1662607 in thefox's v1.0)
	return buf.sample_rate( rate );
}

void Simple_Apu::write_register( cpu_addr_t addr, int data )
{
	apu.write_register( clock(), addr, data );
}

int Simple_Apu::read_status()
{
	return apu.read_status( clock() );
}

void Simple_Apu::end_frame()
{
	time = 0;
    // on PAL, every frame is the same length
	//frame_length ^= 1;
	apu.end_frame( frame_length );
	buf.end_frame( frame_length );
}

long Simple_Apu::samples_avail() const
{
	return buf.samples_avail();
}

long Simple_Apu::read_samples( sample_t* p, long s )
{
	return buf.read_samples( p, s );
}

void Simple_Apu::save_snapshot( apu_snapshot_t* out ) const
{
	apu.save_snapshot( out );
}

void Simple_Apu::load_snapshot( apu_snapshot_t const& in )
{
	apu.load_snapshot( in );
}

