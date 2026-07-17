;**************************************************************************
;*
;* FILE  vt.i
;* Copyright (c) 2003 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: vt.i,v 1.7 2003/08/07 12:53:35 tlr Exp $
;*
;* DESCRIPTION
;*   Common definitions for vic-tracker.
;*
;******
	IFNCONST VT_I
VT_I		EQU	1

;**************************************************************************
;*
;* Configuration options
;* 
;******
;What to include in the code.
HAVESYNC24	EQU	1
;HAVEDOCS	EQU	1
;HAVESCALE	EQU	1
HAVEFADEIN	EQU	1
	
; Colors
BorderColor	 EQU	6
BorderColor_Play EQU	3
BorderColor_Play2 EQU	2
BackgroundColor	 EQU	14
DefaultTextColor EQU	6
DefaultTextColor2 EQU	0
StatusTextColor	 EQU	1
LineNumColor	EQU	0
Edit1Color	EQU	2
Edit2Color	EQU	6
Edit3Color	EQU	7
EditTextColor	EQU	0
EditTextColor2	EQU	7
DocColor	EQU	0	;The documentation screen.


;**************************************************************************
;*
;* Definitions for the editors.
;* 
;******
; The editor modes
EDIT_PATTLIST	EQU	0
EDIT_PATTERN	EQU	1
EDIT_ARP	EQU	2
EDIT_SOUND	EQU	3

; editor flags for the cursortab
CT_HIGHNYBBLE	EQU	$01
CT_DATA		EQU	$02
CT_LINENUM	EQU	$04
CT_NOTE		EQU	$08
CT_LINENUMHALF	EQU	$10
CT_NOSTEP	EQU	$20


;**************************************************************************
;*
;* Definitions regarding the player/data
;* 
;******
MAXNUMPATTERNS		EQU	$80
MAXNUMPATTERNS_MASK	EQU	$7f


;**************************************************************************
;*
;* Conversion to register values
;* 
;******
; Colors converted
NormalColor	EQU	[BackgroundColor<<4]+BorderColor+8
PlayColor	EQU	[BackgroundColor<<4]+BorderColor_Play+8
Play2Color	EQU	[BackgroundColor<<4]+BorderColor_Play2+8

	ENDIF ;VT_I
; eof
