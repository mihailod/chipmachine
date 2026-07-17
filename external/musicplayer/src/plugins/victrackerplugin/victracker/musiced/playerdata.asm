;**************************************************************************
;*
;* FILE  playerdata.asm
;* Copyright (c) 1994, 2003 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: playerdata.asm,v 1.14 2003/08/09 19:12:23 tlr Exp $
;*
;* DESCRIPTION
;*   The actual music data in T0 or T1 format.
;*   
;*
;******

	seg.u	bss
	org	$3300

PLAYER_T1	EQU	1
TuneStart:
;**************************************************************************
;*
;* TuneSpecific Data
;*
;******
pl_ID:
	ds.b	2
pl_Version:
	ds.b	1
pl_Revision:
	ds.b	1

;0 if interrupt speed shall be LEGACY (victracker 1.0)
;1 if interrupt speed shall be PAL...
;2 if interrupt speed shall be PAL.2X
;3 if interrupt speed shall be PAL.3X
;4 if interrupt speed shall be PAL.4X
;5 if interrupt speed shall be NTSC..
;6 if interrupt speed shall be NTSC2X
;7 if interrupt speed shall be NTSC3X
;8 if interrupt speed shall be NTSC4X
;9 if interrupt speed shall be SYNC24 (trigged by external device)
;A if interrupt speed shall be SYNC48 (trigged by external device)
pl_PlayMode:
	ds.b	1
;0 if scale shall be LEGACY (victracker 1.0)
pl_Scale:
	ds.b	1

pl_Reserved0:
	ds.b	1	;pad

pl_SongNum:
	ds.b	1

;Start and end positions and Speeds for 16 songs 
pl_StartStep:
	ds.b	1
pl_EndStep:
	ds.b	1
pl_RepeatStep:
	ds.b	1
pl_StartSpeed:
	ds.b	1
;the remaining 13
	ds.b	4*13

pl_Arpeggios:
	ds.b	256
	IFCONST	PLAYER_T1
pl_ExtraT1Start:
pl_ArpeggioConf:
	ds.b	32
pl_Reserved2:
	ds.b	60	;pad
pl_Title:
	ds.b	16	;null terminated _or_ exactly 16 chars
pl_Author:
	ds.b	16	;null terminated _or_ exactly 16 chars
pl_Year:
	ds.b	4	;year written as 4 ascii digits
pl_Sounds:
	ds.b	256
pl_LengthTab:
	ds.b	256
pl_ExtraT1End:
	ENDIF ;PLAYER_T1
pl_PattLists:
pl_Tab1:
	ds.b	256
pl_Tab2:
	ds.b	256
pl_Tab3:
	ds.b	256
pl_Tab4:
	ds.b	256
pl_Tab5:
	ds.b	256

pl_PatternData:
	ds.b	$40*128
TuneEnd:

; eof
