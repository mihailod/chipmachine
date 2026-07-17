;**************************************************************************
;*
;* FILE  macros.i
;* Copyright (c) 1995-1996, 2002 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: macros.i,v 1.2 2003/08/02 15:42:00 tlr Exp $
;*
;* DESCRIPTION
;*   Useful macros 
;*   Some need dummyzp to point to an unused zeropage address. 
;*   (for generating a 3 cycle delay)
;*
;******
	IFNCONST MACROS_I
MACROS_I	EQU	1

;**************************************************************************
;*
;* NAME  INFO, WARNING, ERROR
;*
;* SYNOPSIS
;*   INFO <str>,<str>,...
;*   WARNING <str>,<str>,...
;*   ERROR <str>,<str>,...
;*
;* DESCRIPTION
;*   info,warning & error macros.
;*
;* KNOWN BUGS
;*   none
;* 
;******
	MAC	INFO
	echo	"INFO:", {0}
	ENDM
	MAC	ERROR
	echo	"ERROR!:", {0}
	ERR
	ENDM
	MAC	WARNING
	echo	"WARNING!:", {0}
	ENDM


;**************************************************************************
;*
;* NAME  sbne, sbeq
;*
;* SYNOPSIS
;*   sbne <addr>
;*   sbeq <addr>
;*
;* DESCRIPTION
;*   stable branch macros
;*   6 cycles (7 cycles if branch occurs)
;*   NO extra cycles for pagecrossing. needs 'dummyzp' zeropage address
;*
;* KNOWN BUGS
;*   none
;* 
;******
;* sbne *
	MAC	sbne
	IF [>[.+4]]-[>{1}]
	sta	dummyzp
	ELSE
	nop
	nop
	ENDIF
	bne	{1}
	ENDM

;* sbeq *
	MAC	sbeq
	IF [>[.+4]]-[>{1}]
	sta	dummyzp
	ELSE
	nop
	nop
	ENDIF
	beq	{1}
	ENDM

;**************************************************************************
;*
;* NAME  jmpind
;*
;* SYNOPSIS
;*   jmpind <addr>
;*
;* DESCRIPTION
;*   performs jmp (<addr>) but check compile time that the two bytes
;*   at <addr> do not cross a page.
;*   This avoids a bug in 6502/6510.
;*
;* KNOWN BUGS
;*   none
;* 
;******
	MAC	jmpind
	IF [[<{1}]=$ff]
	ERROR 	"[jmpind] page crossing detected!"
	ELSE
	jmp	([{1}])
	ENDIF
	ENDM

;**************************************************************************
;*
;* NAME  DELAY, DELAYCODE
;*
;* SYNOPSIS
;*   DELAY <n>
;*   DELAYCODE
;*
;* DESCRIPTION
;*   Generate a minimum amount of code to delay for <n> cycles.
;*   DELAYCODE must be called after all DELAYs to generate delay
;*   subroutines.
;*
;* KNOWN BUGS
;*   none
;*
;******
;* DELAY *
	MAC	DELAY
	IF	[{1}]>32
	ERROR 	"[DELAY] Delay too big!"
	ENDIF
	IF	[{1}]==1
	ERROR 	"[DELAY] One cycle delay not possible!"
	ENDIF
	IF	[{1}]<0
	ERROR 	"[DELAY] Negative delay not possible!"
	ENDIF

	IF	[{1}]&1
	IFCONST	_MAXDELAYODD
	IF	[{1}] > _MAXDELAYODD
_MAXDELAYODD	SET	[{1}]
	ENDIF
	ELSE
_MAXDELAYODD	SET	[{1}]
	ENDIF
	ELSE
	IFCONST	_MAXDELAYEVEN
	IF	[{1}] > _MAXDELAYEVEN
_MAXDELAYEVEN	SET	[{1}]
	ENDIF
	ELSE
_MAXDELAYEVEN	SET	[{1}]
	ENDIF
	ENDIF

	IF	[{1}]==32
	jsr	_thirtytwo	;32
	ENDIF
	IF	[{1}]==31
	jsr	_thirtyone	;31
	ENDIF
	IF	[{1}]==30
	jsr	_thirty		;30
	ENDIF
	IF	[{1}]==29
	jsr	_twentynine	;29
	ENDIF
	IF	[{1}]==28
	jsr	_twentyeight	;28
	ENDIF
	IF	[{1}]==27
	jsr	_twentyseven	;27
	ENDIF
	IF	[{1}]==26
	jsr	_twentysix	;26
	ENDIF
	IF	[{1}]==25
	jsr	_twentyfive	;25
	ENDIF
	IF	[{1}]==24
	jsr	_twentyfour	;24
	ENDIF
	IF	[{1}]==23
	jsr	_twentythree	;23
	ENDIF
	IF	[{1}]==22
	jsr	_twentytwo	;22
	ENDIF
	IF	[{1}]==21
	jsr	_twentyone	;21
	ENDIF
	IF	[{1}]==20
	jsr	_twenty		;20
	ENDIF
	IF	[{1}]==19
	jsr	_nineteen	;19
	ENDIF
	IF	[{1}]==18
	jsr	_eighteen	;18
	ENDIF
	IF	[{1}]==17
	jsr	_seventeen	;17
	ENDIF
	IF	[{1}]==16
	jsr	_sixteen	;16
	ENDIF
	IF	[{1}]==15
	jsr	_fifteen	;15
	ENDIF
	IF	[{1}]==14
	jsr	_fourteen	;14
	ENDIF
	IF	[{1}]==13
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==12
	jsr	_twelve		;12
	ENDIF
	IF	[{1}]==11
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==10
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	ENDIF
	IF	[{1}]==9
	nop			;2
	nop			;2
	nop			;2
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==8
	nop			;2
	nop			;2
	nop			;2
	nop			;2
	ENDIF
	IF	[{1}]==7
	nop			;2
	nop			;2
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==6
	nop			;2
	nop			;2
	nop			;2
	ENDIF
	IF	[{1}]==5
	nop			;2
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==4
	nop			;2
	nop			;2
	ENDIF
	IF	[{1}]==3
	sta	dummyzp		;3
	ENDIF
	IF	[{1}]==2
	nop			;2
	ENDIF
	ENDM

;* DELAYCODE *
	MAC	DELAYCODE
;we cannot use locals if we want to pass as argument to another macro
_delaycode_start	SET	.
	IFCONST	_MAXDELAYEVEN
	INFO 	"[DELAYCODE] Max even delay used:",_MAXDELAYEVEN
	IF	_MAXDELAYEVEN >= 32
_thirtytwo:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 30
_thirty:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 28
_twentyeight:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 26
_twentysix:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 24
_twentyfour:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 22
_twentytwo:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 20
_twenty:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 18
_eighteen:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 16
_sixteen:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 14
_fourteen:
	nop			;2
	ENDIF
	IF	_MAXDELAYEVEN >= 12
_twelve:
	rts
	ENDIF
	ELSE
	INFO 	"[DELAYCODE] Max even delay used: -"
	ENDIF

	IFCONST	_MAXDELAYODD
	INFO 	"[DELAYCODE] Max odd delay used:",_MAXDELAYODD
	IF	_MAXDELAYODD >= 31
_thirtyone:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 29
_twentynine:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 27
_twentyseven:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 25
_twentyfive:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 23
_twentythree:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 21
_twentyone:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 19
_nineteen:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 17
_seventeen:
	nop			;2
	ENDIF
	IF	_MAXDELAYODD >= 15
_fifteen:
	sta	dummyzp		;3
	rts
	ENDIF
	ELSE
	INFO 	"[DELAYCODE] Max odd delay used: -"
	ENDIF
;we cannot use locals if we want to pass as argument to another macro
_delaycode_end	SET	.
	IF	_delaycode_end-_delaycode_start
	INFO	"[DELAYCODE] code @",_delaycode_start,"-",_delaycode_end-1
	ENDM

;**************************************************************************
;*
;* NAME  START_SAMEPAGE, END_SAMEPAGE
;*
;* SYNOPSIS
;*   START_SAMEPAGE [<label>]
;*   END_SAMEPAGE
;* 
;* DESCRIPTION
;*   These macros marks a section that must stay within the same page.
;*   If the condition is violated, assembly will be aborted.
;*   An optional label argument may be given to START_SAMEPAGE.  This
;*   label is output when showing the address range of the section.
;*
;* KNOWN BUGS
;*   none
;* 
;******
	MAC	START_SAMEPAGE
_SAMEPAGE_MARKER	SET	.
_SAMEPAGE_NAME		SET	{0}
	ENDM

	MAC	END_SAMEPAGE
	IF	>_SAMEPAGE_MARKER != >.
	IF	_SAMEPAGE_NAME!=""
	ERROR	"[SAMEPAGE] Page crossing not allowed! (",_SAMEPAGE_NAME,"@",_SAMEPAGE_MARKER,"-",.,")"
	ELSE
	ERROR	"[SAMEPAGE] Page crossing not allowed! (",_SAMEPAGE_MARKER,"-",.,")"
	ENDIF	
	ELSE
	IF	_SAMEPAGE_NAME!=""
	INFO	"[SAMEPAGE] successful SAMEPAGE. (",_SAMEPAGE_NAME,"@",_SAMEPAGE_MARKER,"-",.,")"
	ELSE
	INFO	"[SAMEPAGE] successful SAMEPAGE. (",_SAMEPAGE_MARKER,"-",.,")"
	ENDIF
	ENDM

;**************************************************************************
;*
;* NAME  SHOWRANGE
;*
;* SYNOPSIS
;*   SHOWRANGE <str>,<startaddr>,<endaddr>
;*
;* DESCRIPTION
;*   print out a string followed by start, end, len, and len in blks
;*
;* KNOWN BUGS
;*   none
;* 
;******
	MAC	SHOWRANGE
	INFO	"[SHOWRANGE]",{1},[{2}],"-",[{3}],"(=",[{3}]-[{2}],"bytes,",([{3}]-[{2}]+253)/254,"blocks)"
	ENDM

	ENDIF ;MACROS.I
; eof

