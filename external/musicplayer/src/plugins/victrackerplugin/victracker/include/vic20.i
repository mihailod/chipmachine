;**************************************************************************
;*
;* FILE  vic20.i
;* Copyright (c) 2003 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: vic20.i,v 1.1 2003/07/12 10:13:48 tlr Exp $
;*
;* DESCRIPTION
;*   Misc definitions for the VIC-20 computer.
;*
;******
	IFNCONST VIC20_I
VIC20_I	EQU	1

; MOS6561 (PAL-B)  Clock 4433618/4 Hz   = 1108404 Hz
; 1108404/22150=50.041Hz
; 1108404/18383=60.295Hz  (NTSC speed on a PAL machine)
; 50.041/5*6=60.050Hz (NTSC speed emulation by inserting an extra frame every
; Error ~0.4%          5 frames)
;the length of a screen in cycles (PAL)
SCREENTIME_PAL		EQU	22150
SCREENTIME_PAL_NTSC	EQU	18383 ;NTSC speed on a PAL machine
;the length of a raster line (PAL)
LINETIME_PAL	EQU	71

; MOS6560 (NTSC-M) Clock 14318181/14 Hz = 1022727 Hz
; 1022727/16963=60.292Hz
; 1022727/20439=50.038Hz  (PAL speed on an NTSC machine)
; 60.292/6*5=50.243Hz (PAL speed emulation by skipping every 6:th frame)
; Error ~0.4%         
;the length of a screen in cycles (NTSC)
SCREENTIME_NTSC		EQU	16963
SCREENTIME_NTSC_PAL	EQU	20439  ;PAL speed on an NTSC machine 
;the length of a raster line (NTSC)
LINETIME_NTSC	EQU	65

ScreenRAM_Exp	EQU	$1000	;$1e00
ColorRAM_Exp	EQU	$9400	;$9600


; Various BASIC tokens.
TOKEN_SYS	EQU	$9e
TOKEN_PEEK	EQU	$c2
TOKEN_PLUS	EQU	$aa
TOKEN_TIMES	EQU	$ac
TOKEN_REM	EQU	$8f


	ENDIF ;VIC20_I
; eof
