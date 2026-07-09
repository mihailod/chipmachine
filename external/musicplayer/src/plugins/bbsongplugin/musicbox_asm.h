#pragma once
// Music Box beeper engine Z80 player, extracted verbatim from 1tracker's
// musicbox.1te. Original Z80 code by Mark Alexander (WHAM! The Music Box,
// 1985); 1tracker reimplementation by Shiru, 2018. Assembled in-repo.
namespace musix::bbsong {
static const char* const MUSICBOX_ASM = R"ASM(

	org #8000
	
begin

	ld hl,music_data
	call play
	ret
	
	
	
play

	ld a,(hl)
	ld (tempo),a
	
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	
	ld (ch1_ptr),de
	
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld (ch2_ptr),de

	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld (ch1_loop),de
	
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld (ch2_loop),de
	
	di
	
play_loop

	call play_row
	call #028E
	inc	e
	jr	z,play_loop
	ei
	
	ret

ch1_note 		db 0
ch2_note 		db 0
border_color 	db 0
ch1_ptr 		dw 0
ch1_loop 		dw 0
ch2_ptr 		dw 0
ch2_loop 		dw 0
tempo 			db 0	;230..255

read_note

	ld	e,(hl)
	inc	hl
	ld	d,(hl)	
	
reread_note

	ld	a,(de)
	inc	de
	
	cp	#40
	jr	z,read_note_loop
	
	ld	(hl),d
	dec	hl
	ld	(hl),e
	
	ret

note_to_period

	ld	a,(hl)
	
	add	a,#0c
	ld	e,a
	ld	d,#00
	
	ld	hl,noteTable
	add	hl,de
	
	ld	h,(hl)
	ld	l,#01

	ret

read_note_loop

	inc	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	
	dec	hl
	dec	hl
	
	jr	reread_note

play_row

	ld	hl,ch1_ptr
	call	read_note
	ld	(ch1_note),a
	
	ld	hl,ch2_ptr
	call	read_note
	ld	(ch2_note),a
	
	ld	hl,ch1_note
	call	note_to_period
	
	rl e
	jp c,drum
	
	push	hl
	
	ld	hl,ch2_note
	call note_to_period
	
	pop	de
	
	ld	a,h
	dec	a
	jr	nz,L8074
	
	ld	a,d
	dec	a
	jr	z,delay
	
L8074

	ld	a,(tempo)
	ld	c,a
	ld	b,0
	ld	a,(border_color)
	ex	af,af'
	ld	a,(border_color)

	ld	ixh,d
	ld	d,#10
	
sound_loop

	nop
	nop
	
L8087

	ex	af,af'
	dec	e
	out	(#fe),a
	
	jr	nz,L80A4

	ld	e,ixh
	xor	d
	ex	af,af'
	dec	l
	jp	nz,L80AB
	
L8095

	out	(#fe),a
	ld	l,h
	xor	d
	djnz sound_loop
	
	inc	c
	jp	nz,L8087
	
	ret

L80A4

	jr	z,L80A4
	ex	af,af'
	dec	l
	jp	z,L8095
	
L80AB

	out	(#fe),a
	nop
	nop
	djnz sound_loop
	
	inc	c
	jp	nz,L8087
	
	ret

delay

	ld	a,(tempo)
	cpl
	ld	c,a
	
L80BB

	push	bc
	push	af
	
	ld	b,0
	
delay1

	push	hl
	
	ld	hl,0
	sra	(hl)
	sra	(hl)
	sra	(hl)
	nop
	pop	hl
	djnz delay1
	
	dec	c
	jp nz,delay1
	
	pop	af
	pop	bc
	
	ret
	
	
	
drum

	push af
	ld   a,(ch2_note)
	ld   d,a
	pop  af
	
	call L812D
	
	cp   #ff
	jr   z,L8142
	cp   #c0
	jp   z,L817F
	ld   b,4
	ld   c,e
	rla
	rla
	rla
	rla
	
L8123

	rla
	
	call c,L8142
	call nc,L80BB
	
	djnz L8123
	
	ret
	
L812D

	push af
	
	ld   a,(tempo)
	cpl
	ld   b,a
	ld   c,a
	add  a,#01
	sra  a
	sra  a
	ld   e,a
	cp   #00
	jr   nz,$+3
	inc  e

	pop  af
	
	ret

L8142

	push af
	push hl
	push bc
	ld   a,(border_color)
	ld   b,0
	ld   hl,#03e8
	
L814D

	rrc  d
	jp   nc,L8171
	inc  hl
	bit  0,(hl)
	jp   z,L816D
	set  4,a
	xor  #83
	xor  #83
	
L815E

	out  (#fe),a
	
L8160

	nop
	dec  b
	jp   nz,L814D
	dec  c
	jp   nz,L814D
	
	pop  bc
	pop  hl
	pop  af
	
	ret

L816D

	res 4,a
	jr L815E
	
L8171

	scf
	jp   nc,0
	jp   nc,0
	jp   nc,0
	nop
	nop
	jr   L8160

L817F:

	ld   e,b
	ld   d,00
	ld   hl,player_code_end
	adc  hl,de
	
	ld   a,(hl)
	ld   b,a
	ld   hl,#0003
	
L818C:

	push bc
	ld   de,#0001
	push hl
	call L819E
	pop  hl
	ld   de,#00FF
	adc  hl,de
	pop  bc
	djnz L818C
	
	ret
	
L819E:

	ld   a,l
	srl  l
	srl  l
	cpl
	and  #3
	ld   c,a
	ld   b,0
	ld   ix,#03D1
	add  ix,bc
	
	ld   a,(border_color)
	
	call #03D4
	
	di
	
	ret

noteTable

	db #FF,#F0,#E3,#D7,#CB,#C0,#B4,#AB,#A1,#97,#90,#88
	db #80,#79,#72,#6C,#66,#60,#5B,#56,#51,#4C,#48,#44
	db #40,#3D,#39,#36,#33,#30,#2D,#2B,#28,#26,#24,#22
	db #20,#1E,#1C,#1B,#19,#18,#17,#15,#14,#13,#12,#11
	db #10,#0F,#0E,#0D,#0C,#01
	
player_code_end

)ASM";
} // namespace musix::bbsong
