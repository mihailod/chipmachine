#pragma once
// Music Studio beeper engine Z80 player, extracted verbatim from 1tracker's
// musicstudio.1te. Original Z80 code by Sasa Pusica (The Music Studio, 1988);
// 1tracker version by Shiru 2018, based on the Beepola version by ccowley.
namespace musix::bbsong {
static const char* const MUSICSTUDIO_ASM = R"ASM(

	org #8000

	;test code

begin

	ld hl,music_data
	call play
	ret
	
	
	
; *****************************************************************************
; * The Music Studio Player Engine
; *
; * Based on code written by Sasa Pusica for the utility, The Music Studio.
; * and modified by Chris Cowley for Beepola v1.08.01
; * Modified again by Shiru for 1tracker, different song format, but
; * sound generation code remained intact
; ******************************************************************************
 
border_col=0

play

	di
	
	ld   a,(hl)
	inc hl
	ld (tempo),a

	ld   (row_ptr),hl

	exx
	push  hl

nextnote

	call  playnote
	xor   a
	in    a,(#fe)
	and   #1f
	cp    #1f
	jr    z,nextnote			; play next note if no key pressed

	pop   hl
	exx							; restore hl' for return to basic
	ei
	ret							; return from playing tune



playnote

row_ptr=$+1
	ld hl,0

row_loop

	ld a,(hl)
	inc hl
	or a
	
	jr   nz,no_row_loop
	
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	jr row_loop

no_row_loop

	ld d,a
	ld e,1
	
	ld a,(hl)
	inc hl
	ld (row_ptr),hl
	
	exx
	ld d,a
	ld e,1
	exx
	 
tempo=$+1
	ld   bc,0
	ld   a,border_col
	ex   af,af'
	ld   a,border_col			; so now bc = tempo, a and a' = border_col
	exx

output_note

	ld   ixh,d					; put note frequency for chan 1 into ixh
	ld   h,d
	ld   l,h
	dec  l
	ld   e,l
	jr   z,continue1
	ld   e,#10
	
continue1

	exx
	ld   ixl,d					; put note frequency for chan 2 into ixl
	ld   h,d
	ld   l,h
	dec  l
	ld   e,l
	jr   z,continue2
	ld   e,#10
	
continue2

	exx
	ex   af,af'
	out  (#fe),a
	dec  h						; dec h, which also holds the frequency value
	jr   nz,l8055
	xor  e
	ld   h,d
	push af
	ld   a,ixh
	cp   #20
	jr   nc,l8054				; if a > #20 then this is not a drum effect, skip the inc d
	inc  d						; create the 'fast falling pitch' percussion effect
	
l8054

	pop	af
	
l8055

	dec  l
	jr   nz,l805b
	xor  e
	ld   l,d
	dec  l
	
l805b

	exx
	ex   af,af'
	out  (#fe),a
	dec  h
	jr   nz,l806d
	xor  e
	ld   h,d
	push af
	ld   a,ixl
	cp   #20
	jr   nc,l806c				; if a > #20 then this is not a drum effect, skip the inc d
	inc  d						; create the 'fast falling pitch' percussion effect
	
l806c

	pop  af
	
l806d

	dec  l
	jr   nz,l8073
	xor  e
	ld   l,d
	dec  l

l8073

	djnz continue2
	dec  c
	jr   nz,continue2
	ret

)ASM";
} // namespace musix::bbsong
