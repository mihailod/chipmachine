;**************************************************************************
;* harness.asm -- standalone entry harness for the VIC-TRACKER player.
;*
;* Assembles Daniel Kahlin's VIC-TRACKER 2.0 player (musiced/player.asm,
;* BSD-licensed, see LICENSE.txt) into a position-fixed blob at $2000 that
;* interprets a raw T1/T0 tune image loaded at $3300 (a .vt file with its
;* 2-byte $3300 load address stripped). The tracker editor, screen, disk and
;* IRQ scheduling are all omitted -- the host (vt_machine.cpp) calls the three
;* JMP-vectors below directly and clocks pl_Play itself.
;*
;* Rebuild vtplayer_bin.h with build_player.sh (needs dasm).
;**************************************************************************
	processor 6502
dummyzp	EQU	$fe
	include	"include/macros.i"
	include	"include/vic20.i"
	include	"musiced/vt.i"

	seg	code
	org	$2000
Entry_Init:	jmp	pl_Init		; A/pl_ThisSong = subsong
Entry_UnInit:	jmp	pl_UnInit
Entry_Play:	jmp	pl_Play		; call once per interrupt tick

	include	"musiced/player.asm"
	include	"musiced/playerdata.asm"
