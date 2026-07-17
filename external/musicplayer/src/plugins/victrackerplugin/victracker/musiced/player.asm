	IFNCONST PL_PACKED
;**************************************************************************
;*
;* FILE  player.asm
;* Copyright (c) 1994, 2001, 2003 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: player.asm,v 1.74 2003/08/27 10:42:14 tlr Exp $
;*
;* DESCRIPTION
;*   The player code.
;*
;******

; Which player support should be included
PL_SUPPORT_USERFLAG	EQU	1
PL_SUPPORT_SOUNDS	EQU	1
PL_SUPPORT_SONGS	EQU	1
PL_SUPPORT_ARPEGGIOS	EQU	1
PL_SUPPORT_ARPSOUND	EQU	1
PL_SUPPORT_ARPEFFECT	EQU	1
PL_SUPPORT_DELAY	EQU	1
PL_SUPPORT_ARPMODE00	EQU	1
PL_SUPPORT_ARPMODE10	EQU	1
PL_SUPPORT_ARPMODEF0	EQU	1
PL_SUPPORT_EXACTPRELOAD	EQU	1
PL_SUPPORT_PORTAMENTO	EQU	1
PL_SUPPORT_PORTSOUND	EQU	1
PL_SUPPORT_PORTEFFECT	EQU	1
PL_SUPPORT_PORTFAST	EQU	1	;not yet implemented
PL_SUPPORT_PORTSLOW	EQU	1	;not yet implemented
PL_NO_OPTIMIZE		EQU	1
;PL_OPTIMIZE_FIVE	EQU	1	;Optimizations requiring a
					;minimum speed of 5.
;PL_OPTIMIZE_THREE	EQU	1	;Optimizations requiring a
					;minimum speed of 3.
;PL_OPTIMIZE_TWO	EQU	1	;Optimizations requiring a
					;minimum speed of 2.
;PL_OPTIMIZE_ONE	EQU	1	;Optimizations requiring a
					;minimum speed of 1.
PL_SUPPORT_VOLVOICE	EQU	1
	ENDIF ;!PL_PACKED
	IFCONST PL_PACKED
;**************************************************************************
;*
;* FILE  player.asm
;* Copyright (c) 1994, 2001, 2003 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: player.asm,v 1.74 2003/08/27 10:42:14 tlr Exp $
;*
;* DESCRIPTION
;*   Player for vic-tracker tunes packed by vtcomp.
;*   Nice huh?
;*
;*   Initialize tune:
	IFCONST PL_SUPPORT_SONGS
;*     lda #Song
	ENDIF ;PL_SUPPORT_SONGS
;*     jsr pl_Init 
;*
;*   Uninitialize tune:
;*     jsr pl_UnInit 
;*
;*   Every frame:
;*     jsr pl_Play
	IFCONST	PL_SUPPORT_USERFLAG
;*
;*   Read UserFlag into Acc:
;*     jsr pl_ReadFlag
	ENDIF ;PL_SUPPORT_USERFLAG
;*
;******
PL_SUPPORT_PORTFAST	EQU	1	;not yet implemented
PL_SUPPORT_PORTSLOW	EQU	1	;not yet implemented
	ENDIF ;PL_PACKED

; Player commands
	IFCONST PL_SUPPORT_PORTEFFECT
	IFCONST	PL_SUPPORT_PORTFAST
PL_PORTUP	EQU	$10
PL_PORTDOWN	EQU	$20
	ENDIF ;PL_SUPPORT_PORTFAST
	ENDIF ;PL_SUPPORT_PORTEFFECT
	IFCONST PL_SUPPORT_ARPEFFECT
PL_ARPEGGIO	EQU	$30
	ENDIF ;PL_SUPPORT_ARPEFFECT
	IFCONST PL_SUPPORT_PORTEFFECT
	IFCONST	PL_SUPPORT_PORTSLOW
PL_PORTUPSLOW	EQU	$50
PL_PORTDOWNSLOW	EQU	$60
	ENDIF ;PL_SUPPORT_PORTSLOW
	ENDIF ;PL_SUPPORT_PORTEFFECT
	IFCONST	PL_SUPPORT_USERFLAG
PL_SETUSERFLAG	EQU	$70
	ENDIF ;PL_SUPPORT_USERFLAG
PL_SETSOUND	EQU	$80
PL_CUTNOTE	EQU	$c0
	IFCONST PL_SUPPORT_DELAY
PL_DELAYNOTE	EQU	$d0
	ENDIF ;PL_SUPPORT_DELAY

	IFCONST PL_SUPPORT_VOLVOICE
PL_NUMVOICES	EQU	5
	ELSE ;PL_SUPPORT_VOLVOICE
PL_NUMVOICES	EQU	4
	ENDIF ;PL_SUPPORT_VOLVOICE

;The sound format
PL_SND_DURATION		EQU	0
PL_SND_FOFFS		EQU	1
PL_SND_GLIDE		EQU	2
PL_SND_ARPEGGIO		EQU	3

;**************************************************************************
;*
;* zero page allocation
;*
;******
	seg.u	zp
	org	$b0
PatternZP:	ds.w	1
PatternTabZP:	ds.w	1
pl_Temp1ZP:	ds.b	1
pl_Temp2ZP:	ds.b	1
pl_Temp3ZP:	ds.b	1

	seg	code
;**************************************************************************
;*
;* Start of generated code
;*
;******
pl_Init:
	jmp	pl_IInit
pl_UnInit:
	jmp	pl_UUnInit
pl_Play:
	jmp	pl_PPlay
	IFCONST	PL_SUPPORT_USERFLAG
pl_ReadFlag:
	jmp	pl_RReadFlag
	ENDIF ;PL_SUPPORT_USERFLAG

;**************************************************************************
;*
;* Data
;*
;******
pl_PlayFlag:
	dc.b	0
pl_Speed:
	dc.b	0
pl_Count:
	dc.b	0
	IFCONST	PL_SUPPORT_USERFLAG
pl_UserFlag:
	dc.b	0
	ENDIF ;PL_SUPPORT_USERFLAG
	IFNCONST PL_PACKED
pl_Step:
	dc.b	0
pl_PatternPos:
	dc.b	0
pl_Mute:
	dc.b	0,0,0,0
pl_ThisSong:
	dc.b	0
pl_ThisStartSpeed:
	dc.b	0
pl_ThisStartStep:
	dc.b	0
pl_ThisEndStep:
	dc.b	0
pl_ThisRepeatStep:
	dc.b	0
;Conversion table for the editor, so that notes may be entered
;as two digits, the first meaning octave, and the second meaning note
;(0-b), $3c and $3d is the same as $40 and $41 for making the note value
;fit into the range $00 - $3f when needed.
pl_ConvTab:
	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	              ;$00-$0f
	dc.b	1,2,3,4,5,6,7,8,9,10,11,12,0,0,0,0            ;$10-$1f
	dc.b	13,14,15,16,17,18,19,20,21,22,23,24,0,0,0,0   ;$20-$2f
	dc.b	25,26,27,28,29,30,31,32,33,34,35,36,37,38,0,0 ;$30-$3f
	dc.b	37,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0             ;$40-$4f
; The arpeggio index, pretty straight forward here, but in the packer
; this gets optimized.
pl_ArpeggioIndex:
	dc.b	$00,$10,$20,$30,$40,$50,$60,$70
	dc.b	$80,$90,$a0,$b0,$c0,$d0,$e0,$f0
	ENDIF ;!PL_PACKED 
pl_FreqTab:
	dc.b	0
	dc.b	135,143,147,151,159,163,167,175,179,183,187,191
	dc.b	195,199,201,203,207,209,212,215,217,219,221,223
	dc.b	225,227,228,229,231,232,233,235,236,237,238,239
	dc.b	240,241
pl_VoiceData:
; The voice data structure
	IFNCONST PL_PACKED
pl_vd_PatternTabLow:	dc.b	<pl_Tab1,<pl_Tab2,<pl_Tab3,<pl_Tab4,<pl_Tab5 ;pointer to Pattlist
pl_vd_PatternTabHigh	dc.b	>pl_Tab1,>pl_Tab2,>pl_Tab3,>pl_Tab4,>pl_Tab5 ;
	ELSE ;!PL_PACKED
	IFNCONST PL_SUPPORT_SONGS
	IFCONST PL_SUPPORT_VOLVOICE
pl_vd_PatternTabLow:	dc.b	<pl_Tab1_0,<pl_Tab2_0,<pl_Tab3_0,<pl_Tab4_0,<pl_Tab5_0 ;pointer to Pattlist
pl_vd_PatternTabHigh	dc.b	>pl_Tab1_0,>pl_Tab2_0,>pl_Tab3_0,>pl_Tab4_0,>pl_Tab5_0 ;
	ELSE ;PL_SUPPORT_VOLVOICE
pl_vd_PatternTabLow:	dc.b	<pl_Tab1_0,<pl_Tab2_0,<pl_Tab3_0,<pl_Tab4_0 ;pointer to Pattlist
pl_vd_PatternTabHigh	dc.b	>pl_Tab1_0,>pl_Tab2_0,>pl_Tab3_0,>pl_Tab4_0 ;
	ENDIF ;PL_SUPPORT_VOLVOICE
	ELSE ;!PL_SUPPORT_SONGS
pl_vd_PatternTabLow:	ds.b	PL_NUMVOICES	;pointer to Pattlist
pl_vd_PatternTabHigh:	ds.b	PL_NUMVOICES	;
	ENDIF ;PL_SUPPORT_SONGS
	ENDIF ;PL_PACKED
pl_vd_ZeroBegin:	;!!Everything after this gets cleared!!
pl_vd_Note:		ds.b	PL_NUMVOICES	;Current Pitch
pl_vd_Param:		ds.b	PL_NUMVOICES	;Current Param
pl_vd_NextNote:		ds.b	PL_NUMVOICES	;Next Pitch
pl_vd_NextParam:	ds.b	PL_NUMVOICES	;Next Param
	IFCONST PL_SUPPORT_DELAY
pl_vd_DelayCount:	ds.b	PL_NUMVOICES	;Delay Count
	ENDIF ;PL_SUPPORT_DELAY
pl_vd_DurationCount:	ds.b	PL_NUMVOICES	;Duration Count
pl_vd_ThisNote:		ds.b	PL_NUMVOICES	;This Note   (These get updated when
pl_vd_ThisParam:	ds.b	PL_NUMVOICES	;This Param   pl_Retrig is called)
pl_vd_EffectiveNote:	ds.b	PL_NUMVOICES	;Effective Note
	IFCONST	PL_SUPPORT_PORTAMENTO
pl_vd_FreqOffsetLow:	ds.b	PL_NUMVOICES	;Current freqoffset
pl_vd_FreqOffsetHigh:	ds.b	PL_NUMVOICES	;Current freqoffset
	ENDIF ;PL_SUPPORT_PORTAMENTO
	IFCONST	PL_SUPPORT_ARPEGGIOS
pl_vd_ArpMode:		ds.b	PL_NUMVOICES	;Arpeggio Mode
pl_vd_ArpStep:		ds.b	PL_NUMVOICES	;Arpeggio step
pl_vd_ArpOffset:	ds.b	PL_NUMVOICES	;Arpeggio note offset
pl_vd_ArpCount:		ds.b	PL_NUMVOICES	;Arpeggio count
	ENDIF ;PL_SUPPORT_ARPEGGIOS
pl_vd_Sound:		ds.b	PL_NUMVOICES	;Current Sound
	IFCONST PL_PACKED
pl_vd_PattlistStep:	ds.b	PL_NUMVOICES	;Step in pattlist
pl_vd_PattlistWait:	ds.b	PL_NUMVOICES	;Time before reading the next pattlist
                                        ;entry
pl_vd_CurrentPattern:	ds.b	PL_NUMVOICES
pl_vd_PatternStep:	ds.b	PL_NUMVOICES	;Step in pattern
pl_vd_PatternWait:	ds.b	PL_NUMVOICES	;Time before reading the next note.
	ENDIF ;PL_PACKED
pl_vd_ZeroEnd:		;!!Everything before this gets cleared!!
	IFCONST PL_PACKED
pl_vd_FetchMode:	ds.b	PL_NUMVOICES	;Fetch mode.
	ENDIF ;PL_PACKED

;**************************************************************************
;*
;* InitRoutine
;*
;******
pl_IInit:
	jsr	pl_UUnInit

	IFNCONST PL_PACKED
	lda	pl_ThisSong	;X=pl_ThisSong*4
	asl
	asl
	tax
	lda	pl_StartSpeed,x
	sta	pl_ThisStartSpeed
	and	#$3f
	sta	pl_Speed
	lda	pl_StartStep,x
	sta	pl_ThisStartStep
	sta	pl_Step
	lda	pl_EndStep,x
	sta	pl_ThisEndStep
	lda	pl_RepeatStep,x
	sta	pl_ThisRepeatStep
	lda	#0
	sta	pl_PatternPos
	sta	pl_Count
	ELSE ;!PL_PACKED 
	IFCONST PL_SUPPORT_SONGS
	tax			;X=Song
	IFCONST PL_SUPPORT_VOLVOICE
	sta	pl_Temp1ZP
	asl
	asl
	clc
	adc	pl_Temp1ZP
	tay			;Y=Song*5
	ELSE ;PL_SUPPORT_VOLVOICE
	asl
	asl
	tay			;Y=Song*4
	ENDIF ;PL_SUPPORT_VOLVOICE
	lda	pl_SongSpeed,x
	sta	pl_Speed
	ldx	#0
	stx	pl_Count
pli_lp1:
	lda	pl_SongLow,y
	sta	pl_vd_PatternTabLow,x
	lda	pl_SongHigh,y
	sta	pl_vd_PatternTabHigh,x
	lda	#$80
	sta	pl_vd_FetchMode,x
	iny
	inx
	cpx	#PL_NUMVOICES
	bne	pli_lp1
	ELSE ;PL_SUPPORT_SONGS
	lda	pl_SongSpeed
	sta	pl_Speed
	lda	#0
	sta	pl_Count

	ldx	#PL_NUMVOICES-1
	lda	#$80
pli_lp1:
	sta	pl_vd_FetchMode,x
	dex
	bpl	pli_lp1
	ENDIF ;PL_SUPPORT_SONGS
			
	ENDIF ;PL_PACKED 

	lda	#0
	IFCONST	PL_SUPPORT_USERFLAG
	sta	pl_UserFlag
	ENDIF ;PL_SUPPORT_USERFLAG
	ldx	#pl_vd_ZeroEnd-pl_vd_ZeroBegin-1
pli_lp2:
	sta	pl_vd_ZeroBegin,x
	dex
	bpl	pli_lp2
	IFCONST	PL_SUPPORT_EXACTPRELOAD
	ldx	#PL_NUMVOICES-1
pli_lp3:
	jsr	pl_GetVoice
	IFNCONST PL_NO_OPTIMIZE
	jsr	pl_GetVoice	;Necessary for optimized players.
	ENDIF ;PL_NO_OPTIMIZE
	dex
	bpl	pli_lp3
	ENDIF ;PL_SUPPORT_EXACTPRELOAD
	IFNCONST PL_PACKED
	jsr	pl_UpdatePosition
	ENDIF ;PL_PACKED

; Ok, now x=$ff
	stx	pl_PlayFlag	;Start Song! (pl_PlayFlag=$ff)

	IFNCONST PL_SUPPORT_VOLVOICE
	lda	#$0f		;Set initial volume
	jsr	plpv_SetVolume
	ENDIF ;PL_SUPPORT_VOLVOICE
	rts

;**************************************************************************
;*
;* UnInitRoutine
;*
;******
pl_UUnInit:
	pha
	lda	#0
	sta	pl_PlayFlag	;Ensure that we don't try to play
; initialize sound registers
	sta	$900a
	sta	$900b
	sta	$900c
	sta	$900d
	jsr	plpv_SetVolume
	pla
	rts


;**************************************************************************
;*
;* PlayRoutine
;*
;******
pl_PPlay:
	lda	pl_PlayFlag
	beq	plpp_ex1

	dec	pl_Count
	bmi	pl_skp1

	IFCONST	PL_OPTIMIZE_FIVE
	ldx	pl_Count
	cpx	#PL_NUMVOICES	;is X less that PL_NUMVOICES? 
	bcs	pl_skp3		;no, skip out.
	jsr	pl_GetVoice	;fetch data for voice X.
	ENDIF ;PL_OPTIMIZE_FIVE
	IFCONST	PL_OPTIMIZE_THREE
	lda	pl_Count
	beq	pl_o3step0
	cmp	#1
	beq	pl_o3step1
	cmp	#2
	beq	pl_o3step2
	ENDIF ;PL_OPTIMIZE_THREE
	IFCONST	PL_OPTIMIZE_TWO
	lda	pl_Count
	beq	pl_o2step0
	cmp	#1
	beq	pl_o2step1
	ENDIF ;PL_OPTIMIZE_TWO
	IFCONST	PL_OPTIMIZE_ONE
	lda	pl_Count
	beq	pl_o1step0
	ENDIF ;PL_OPTIMIZE_ONE
	jmp	pl_skp3
plpp_ex1:
	rts
	IFCONST	PL_OPTIMIZE_THREE
pl_o3step0:
	ldx	#0
	jsr	pl_GetVoice
	jmp	pl_skp3
pl_o3step1:
	ldx	#2
	jsr	pl_GetVoice
	dex
	jsr	pl_GetVoice
	jmp	pl_skp3
pl_o3step2:
	IFCONST PL_SUPPORT_VOLVOICE
	ldx	#4
	jsr	pl_GetVoice
	dex
	ELSE ;PL_SUPPORT_VOLVOICE
	ldx	#3
	ENDIF ;PL_SUPPORT_VOLVOICE
	jsr	pl_GetVoice
	jmp	pl_skp3
	ENDIF ;PL_OPTIMIZE_THREE
	IFCONST	PL_OPTIMIZE_TWO
pl_o2step0:
	ldx	#1
	jsr	pl_GetVoice
	dex
	jsr	pl_GetVoice
	jmp	pl_skp3
pl_o2step1:
	IFCONST	PL_SUPPORT_VOLVOICE
	ldx	#4
	jsr	pl_GetVoice
	dex
	ELSE ;PL_SUPPORT_VOLVOICE
	ldx	#3
	ENDIF ;PL_SUPPORT_VOLVOICE
	jsr	pl_GetVoice
	dex
	jsr	pl_GetVoice
	jmp	pl_skp3
	ENDIF ;PL_OPTIMIZE_TWO
	IFCONST	PL_OPTIMIZE_ONE
pl_o1step0:
	ldx	#PL_NUMVOICES-1
plo1_lp1:
	jsr	pl_GetVoice
	dex
	bpl	plo1_lp1
	jmp	pl_skp3
	ENDIF ;PL_OPTIMIZE_ONE
	
pl_skp1:

;*** Get Data from tune ***
	ldx	#PL_NUMVOICES-1
pl_lp1:
	IFCONST PL_NO_OPTIMIZE
	jsr	pl_GetVoice
	ENDIF ;PL_NO_OPTIMIZE
	jsr	pl_PreCheckEffects
	jsr	pl_Retrig
	jsr	pl_PostCheckEffects
	dex
	bpl	pl_lp1

	lda	pl_Speed
	sta	pl_Count

	IFNCONST PL_PACKED
	jsr	pl_UpdatePosition
	ENDIF ;!PL_PACKED 
pl_skp3:
;*** play the voices ***
	ldx	#PL_NUMVOICES-1
pl_lp3:
	jsr	pl_PreUpdateVoice
	jsr	pl_PlayVoice
	jsr	pl_PostUpdateVoice
	dex
	bpl	pl_lp3

	rts

	IFCONST	PL_SUPPORT_USERFLAG
;**************************************************************************
;*
;* Read flag
;*
;******
pl_RReadFlag:
	lda	pl_UserFlag
	rts
	ENDIF ;PL_SUPPORT_USERFLAG

	IFNCONST PL_PACKED
;**************************************************************************
;*
;* GetVoice
;* X=Voice
;*
;******
pl_GetVoice:

;*** Get Pointer To PatternTab ***
	lda	pl_vd_PatternTabLow,x
	sta	PatternTabZP
	lda	pl_vd_PatternTabHigh,x
	sta	PatternTabZP+1

;*** Get Address to Current Pattern ***
	ldy	pl_Step
	lda	#0
	sta	PatternZP
	lda	(PatternTabZP),y ;PatternZP=pl_PatternData+patt*$100/$4
	and	#$7f
	lsr
	ror	PatternZP
	lsr
	ror	PatternZP
	pha
	lda	PatternZP
	clc
	adc	#<pl_PatternData
	sta	PatternZP
	pla
	adc	#>pl_PatternData
	sta	PatternZP+1


;*** Read last note and param values ***
	lda	pl_vd_NextNote,x
	sta	pl_vd_Note,x
	lda	pl_vd_NextParam,x
	sta	pl_vd_Param,x

;*** Get Current NOTE and PARAM from Pattern ***
	lda	pl_PatternPos
	asl
	tay
	lda	(PatternZP),y	;Acc=Note
	pha
	and	#$80
	sta	pl_Temp1ZP	;pl_Temp1ZP=Note&0x80
	iny
	lda	(PatternZP),y	;Acc=Param
	sta	pl_vd_NextParam,x
	pla			;Acc=Note

	cpx	#$03		;Is this voice >=3
	bcs	plgv_skp1	;yes, then do not convert pitch

	cmp	#$80		;Is this repeat last note?
	beq	plgv_skp1	;yes, then do not convert pitch

	and	#$7f
	tay
	lda	pl_ConvTab,y
	ora	pl_Temp1ZP
plgv_skp1:
	sta	pl_vd_NextNote,x
plgv_ex1:
	rts

;**************************************************************************
;*
;* pl_UpdatePosition
;*
;******
pl_UpdatePosition:
;*** Handle position ***
	ldy	pl_Step
	lda	pl_PatternPos
	inc	pl_PatternPos
	cmp	pl_LengthTab,y
	bne	plup_skp1

	lda	#0
	sta	pl_PatternPos

	inc	pl_Step
	cpy	pl_ThisEndStep
	bne	plup_skp1

; Check repeat flags in speed to see if StartStep or Repeat step is the
; target.  This should handle halt aswell.
	ldy	pl_ThisStartStep
	lda	pl_ThisStartSpeed
	and	#$c0
	beq	plup_skp2
	ldy	pl_ThisRepeatStep
plup_skp2:
	sty	pl_Step

plup_skp1:
	rts
			
	ELSE !PL_PACKED
;**************************************************************************
;*
;* GetVoice
;* X=Voice
;*
;******
pl_GetVoice:

;*** Get Pointer To PatternTab ***
	lda	pl_vd_FetchMode,x
	cmp	#$80		;Fetch new pattern?
	bne	plgv_skp1	;No... skip

plgv_lp1:
;*** Get new entry from patternlist ***
	jsr	pl_GetPattlist

plgv_skp1:
;*** Read last note and param values ***
	lda	pl_vd_NextParam,x
	sta	pl_vd_Param,x
	lda	pl_vd_NextNote,x
	sta	pl_vd_Note,x

;*** Calculate pattern address ***

	ldy	pl_vd_CurrentPattern,x
	lda	pl_PatternsLow,y
	sta	PatternZP
	lda	pl_PatternsHigh,y
	sta	PatternZP+1

;*** Get Current NOTE and PARAM from Pattern ***
	lda	pl_vd_PatternWait,x
	beq	plgv_skp2
	sec
	sbc	#1
	sta	pl_vd_PatternWait,x
	jmp	plgv_skp4

plgv_skp2:

;*** Interpret pattern data ***
	ldy	pl_vd_PatternStep,x
	lda	(PatternZP),y	;Get Codebyte
	iny
	sta	pl_Temp1ZP
	and	#%11100000
	sta	pl_Temp2ZP
	cmp	#$80		;End of pattern?
	bne	plgv_skp3	;no... skip!

	sta	pl_vd_FetchMode,x
	lda	#0
	sta	pl_vd_PatternStep,x
	sta	pl_vd_PatternWait,x
	jmp	plgv_lp1	; Fetch new pattern

plgv_skp3:
	
	tya
	sta	pl_vd_PatternStep,x
	lda	pl_Temp1ZP
	and	#$1f
	sta	pl_vd_PatternWait,x
	lda	pl_Temp2ZP
	sta	pl_vd_FetchMode,x
	jsr	pl_Fetch
	jmp	plgv_ex1

plgv_skp4:
;*** fetch data ***
	lda	pl_vd_FetchMode,x
	bpl	plgv_ex1
	jsr	pl_Fetch
	
plgv_ex1:
	rts

;**************************************************************************
;*
;* GetPattlist
;* X=Voice
;*
;******
pl_GetPattlist:
	lda	pl_vd_PattlistWait,x
	beq	plgl_skp1
	sec
	sbc	#1
	sta	pl_vd_PattlistWait,x
	rts

plgl_skp1:
	lda	pl_vd_PatternTabLow,x
	sta	PatternTabZP
	lda	pl_vd_PatternTabHigh,x
	sta	PatternTabZP+1

;*** Get Address to Current Pattern ***
	ldy	pl_vd_PattlistStep,x
plgl_lp1:
	lda	#0
	sta	pl_Temp1ZP
	lda	(PatternTabZP),y
	bpl	plgl_skp3
	pha
	and	#$3f
	sta	pl_Temp1ZP
	pla
	and	#$c0
	cmp	#$c0
	bne	plgl_skp2	
	ldy	pl_Temp1ZP	;Starta om
	jmp	plgl_lp1	;stämman
plgl_skp2:
	iny
	lda	(PatternTabZP),y
plgl_skp3:
	sta	pl_Temp2ZP

	iny
	tya
	sta	pl_vd_PattlistStep,x
	lda	pl_Temp1ZP
	sta	pl_vd_PattlistWait,x
	lda	pl_Temp2ZP
	sta	pl_vd_CurrentPattern,x
plgl_ex1:
	rts
	
;**************************************************************************
;*
;* Fetch
;* Acc=CodeByte X=Voice
;*
;******
pl_Fetch:
	ldy	pl_vd_PatternStep,x
	and	#$60
	beq	plf_Null
	cmp	#$20
	beq	plf_Pitch
	cmp	#$40
	beq	plf_Param

;*** fetch pitch+param ***
	lda	(PatternZP),y
	iny
	sta	pl_Temp1ZP
	lda	(PatternZP),y
	iny
	sta	pl_Temp2ZP
	jmp	plf_ex1

;*** set to zero ***
plf_Null:
	lda	#0
	sta	pl_Temp1ZP
	sta	pl_Temp2ZP
	jmp	plf_ex1
;*** fetch pitch ***
plf_Pitch:
	lda	(PatternZP),y
	iny
	sta	pl_Temp1ZP
	lda	#0
	sta	pl_Temp2ZP
	jmp	plf_ex1
;*** fetch param , pitch to $80 ***
plf_Param:
	lda	(PatternZP),y
	iny
	sta	pl_Temp2ZP
	lda	#$80
	sta	pl_Temp1ZP
	jmp	plf_ex1

plf_ex1:
	tya
	sta	pl_vd_PatternStep,x

	lda	pl_Temp2ZP
	sta	pl_vd_NextParam,x
	lda	pl_Temp1ZP
	sta	pl_vd_NextNote,x

	rts


	ENDIF ;PL_PACKED

;**************************************************************************
;*
;* PreCheckEffects
;* X=Voice
;*
;******
pl_PreCheckEffects:
	IFCONST PL_SUPPORT_VOLVOICE
	cpx	#$04
	beq	plpce_VolumeTempo

	ENDIF ;PL_SUPPORT_VOLVOICE
	lda	pl_vd_Param,x
	and	#$f0
	IFCONST	PL_SUPPORT_SOUNDS
	cmp	#PL_SETSOUND
	beq	plpce_SetSound
	ENDIF ;PL_SUPPORT_SOUNDS
	rts

	IFCONST	PL_SUPPORT_SOUNDS
plpce_SetSound:
	lda	pl_vd_Param,x
	and	#$0f
	asl
	asl
	IFNCONST PL_PACKED
	asl	; the packed player uses 4 bytes per sound, and the 
		; editor player uses 8.
	ENDIF ;PL_PACKED
	sta	pl_vd_Sound,x
	rts
	ENDIF ;PL_SUPPORT_SOUNDS
		
	IFCONST PL_SUPPORT_VOLVOICE
plpce_VolumeTempo:
	lda	pl_vd_Note,x
	and	#$f0
	beq	plcevt_ex1
	lsr
	lsr
	lsr
	lsr
	sta	pl_Speed
plcevt_ex1:
	rts
	ENDIF ;PL_SUPPORT_VOLVOICE

;**************************************************************************
;*
;* Retrig
;* X=Voice
;*
;******
pl_Retrig:
; Transfer the preloaded param
	lda	pl_vd_Param,x
	sta	pl_vd_ThisParam,x

; Transfer the preloaded Note (if not a tie.)
	lda	pl_vd_Note,x
	beq	plrt_ex2	; No note... skip out!
	cmp	#$80
	beq	plrt_skp3	; Last note... just update the duration and
				; skip out!
	lda	pl_vd_Note,x
	sta	pl_vd_ThisNote,x ; Update the note value.
	bmi	plrt_skp3	; No retrig... just update the duration and
				; skip out!

	lda	#0
	IFCONST PL_SUPPORT_DELAY
	sta	pl_vd_DelayCount,x
	ENDIF ;PL_SUPPORT_DELAY
	IFCONST	PL_SUPPORT_ARPEGGIOS
	sta	pl_vd_ArpMode,x
	sta	pl_vd_ArpStep,x
	sta	pl_vd_ArpOffset,x
	sta	pl_vd_ArpCount,x
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	IFCONST	PL_SUPPORT_PORTAMENTO
	sta	pl_vd_FreqOffsetHigh,x
	lda	#$80
	sta	pl_vd_FreqOffsetLow,x     ;The initial value is $0080

	ldy	pl_vd_Sound,x	;Frequency offset from the sound
	lda	pl_Sounds+PL_SND_FOFFS,y
	beq	plrt_skp4	;Do not adjust from $0080 if we are
			        ;already there as this is kind of slow.
	jsr	pl_DoPortamentoSigned	;do portamento _once_.
plrt_skp4:

	ENDIF ;PL_SUPPORT_PORTAMENTO
	IFCONST	PL_SUPPORT_ARPEGGIOS
	IFCONST PL_SUPPORT_ARPEFFECT
;Does this Command indicate arpeggio?
	lda	pl_vd_Param,x
	pha
	and	#$f0
	tay
	pla
	cpy	#PL_ARPEGGIO
	beq	plrt_Arpeggio	;Yes, run it!

	ENDIF ;PL_SUPPORT_ARPEFFECT
;Does this sound indicate arpeggio?
	ldy	pl_vd_Sound,x
	lda	pl_Sounds+PL_SND_ARPEGGIO,y
	bpl	plrt_skp3	;no, skip it!

; If we have an arpeggio, we must set up the initial offset
; when we are at it we determine the mode aswell.
plrt_Arpeggio:
	and	#$0f
	tay
	asl
	sta	pl_Temp1ZP	;pl_Temp1ZP=ArpNum*2
	lda	pl_ArpeggioIndex,y
	tay			;y=ArpNum*16
	lda	pl_Arpeggios,y
	sta	pl_vd_ArpOffset,x
	ldy	pl_Temp1ZP
	lda	pl_ArpeggioConf,y
	and	#$f0
	sta	pl_vd_ArpMode,x
	ENDIF ;PL_SUPPORT_ARPEGGIOS

plrt_skp3:

	IFCONST PL_SUPPORT_VOLVOICE
	cpx	#$04		;Volume/tempo channel always play legato
	beq	plrt_skp1

	ENDIF ;PL_SUPPORT_VOLVOICE
	lda	pl_vd_NextNote,x ;If next note is Tie or noretrig
	bmi	plrt_skp1		     ; Play legato aswell!

;Fetch the duration from the current sound
	ldy	pl_vd_Sound,x
	lda	pl_Sounds+PL_SND_DURATION,y
	bne	plrt_skp2
plrt_skp1:
	lda	pl_Speed	;Duration 00 means set duration to speed
	clc
	adc	#1
plrt_skp2:
	sta	pl_vd_DurationCount,x
	
plrt_ex2:
	rts

;**************************************************************************
;*
;* PostCheckEffects
;* X=Voice
;*
;******
pl_PostCheckEffects:
	lda	pl_vd_Param,x
	and	#$f0
	IFCONST	PL_SUPPORT_USERFLAG
	cmp	#PL_SETUSERFLAG
	beq	plce_UserFlag
	ENDIF ;PL_SUPPORT_USERFLAG
	IFCONST PL_SUPPORT_DELAY
	cmp	#PL_DELAYNOTE
	beq	plce_DelayNote
	ENDIF ;PL_SUPPORT_DELAY

	IFNCONST PL_PACKED
	IFCONST PL_SUPPORT_VOLVOICE
	cpx	#$04		; Effects below not available for the
	beq	plce_ex1	; volume track. (the packer should make sure)
	
	ENDIF ;PL_SUPPORT_VOLVOICE
	ENDIF ;PL_PACKED
	cmp	#PL_CUTNOTE
	beq	plce_CutNote
plce_ex1:
	rts

	IFCONST	PL_SUPPORT_USERFLAG
plce_UserFlag:
	lda	pl_vd_Param,x
	and	#$0f
	sta	pl_UserFlag
	rts
	ENDIF ;PL_SUPPORT_USERFLAG
plce_CutNote:
	lda	pl_vd_Param,x
	and	#$0f
	sta	pl_vd_DurationCount,x
	rts
	IFCONST PL_SUPPORT_DELAY
plce_DelayNote:
	lda	pl_vd_Param,x
	and	#$0f
	sta	pl_vd_DelayCount,x
	rts
	ENDIF ;PL_SUPPORT_DELAY

;**************************************************************************
;*
;* PreUpdateVoice
;* X=Voice
;*
;******
pl_PreUpdateVoice:
	IFCONST PL_SUPPORT_DELAY
; Handle the Delay Count
	lda	pl_vd_DelayCount,x
	bne	pl_puv_ex2	;Not zero... decrease and keep it silent.

	ENDIF ;PL_SUPPORT_DELAY
; Handle the Duration Count
	lda	pl_vd_DurationCount,x
	beq	pl_puv_ex1	;Zero... silence it.
	sec
	sbc	#1
	sta	pl_vd_DurationCount,x ;decrease count
	lda	pl_vd_ThisNote,x
	and	#$7f
	sta	pl_vd_EffectiveNote,x ;set effective note
	rts
	IFCONST PL_SUPPORT_DELAY
pl_puv_ex2:	
	sec
	sbc	#1
	sta	pl_vd_DelayCount,x
	ENDIF ;PL_SUPPORT_DELAY
pl_puv_ex1:
; silence it.
	lda	#0
	sta	pl_vd_EffectiveNote,x

	IFCONST PL_SUPPORT_PORTAMENTO
	sta	pl_vd_FreqOffsetLow,x
	sta	pl_vd_FreqOffsetHigh,x
	ENDIF ;PL_SUPPORT_PORTAMENTO
	IFCONST PL_SUPPORT_ARPEGGIOS
	sta	pl_vd_ArpStep,x
	sta	pl_vd_ArpOffset,x
	sta	pl_vd_ArpCount,x
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	rts

;**************************************************************************
;*
;* PlayVoice
;* X=Voice
;*
;******
pl_PlayVoice:
	IFCONST PL_SUPPORT_VOLVOICE
	cpx	#$04			;Is VOL/Speed channel?
	beq	plpv_VolumeTempo

	ENDIF ;PL_SUPPORT_VOLVOICE
	IFNCONST PL_SUPPORT_ARPEGGIOS
	lda	pl_vd_EffectiveNote,x
	and	#$7f
	beq	plpv_SetValue
	cpx	#$03			;Is this the noise channel?
	beq	plpv_Noise
	tay
	lda	pl_FreqTab,y	;Freq
	beq	plpv_SetValue
plpv_Noise:
	IFCONST PL_SUPPORT_PORTAMENTO
	clc
	adc	pl_vd_FreqOffsetHigh,x  ;Add frequency offset
	ENDIF ;PL_SUPPORT_PORTAMENTO
	ora	#$80		;SetGate
	jmp	plpv_SetValue
	ELSE ;PL_SUPPORT_ARPEGGIOS
	IFCONST	PL_SUPPORT_ARPMODEF0
	lda	pl_vd_ArpMode,x
	cmp	#$f0
	bne	plpv_skp8
	jmp	plpv_ArpModef0
plpv_skp8:
	ENDIF ;PL_SUPPORT_ARPMODEF0

; Arpeggio modes 0 and 1
;Get note in, and put it in pl_Temp2ZP
	lda	pl_vd_EffectiveNote,x
	and	#$7f
	beq	plpv_SetValue
	sta	pl_Temp2ZP

	IFCONST PL_SUPPORT_PORTAMENTO
;put FreqOffset in pl_Temp1ZP.
	lda	pl_vd_FreqOffsetHigh,x
	sta	pl_Temp1ZP
	ENDIF ;PL_SUPPORT_PORTAMENTO
;Preserve ArpOffset into pl_Temp3ZP
	lda	pl_vd_ArpOffset,x
	sta	pl_Temp3ZP
	beq	plpv_skp4	; If no offset, play as fast as possible
	
	IFCONST PL_SUPPORT_PORTAMENTO
;Clear out FreqOffset in pl_Temp1ZP if ArpOffset bit 7 is set.
	bit	pl_Temp3ZP
	bpl	plpv_skp1
	lda	#0
	sta	pl_Temp1ZP
plpv_skp1:
	ENDIF ;PL_SUPPORT_PORTAMENTO

	IFCONST PL_SUPPORT_ARPMODE10
	lda	pl_vd_ArpMode,x
	cmp	#$10
	beq	plpv_ArpMode10	

	ENDIF ;PL_SUPPORT_ARPMODE10
	IFCONST PL_SUPPORT_ARPMODE00
plpv_ArpMode00:
;put new absolute note into pl_Temp2ZP if ArpOffset bit 6 is set.
	bit	pl_Temp3ZP
	bvc	plpv_skp3
	lda	pl_Temp3ZP
	and	#$3f
	IFNCONST PL_PACKED
	tay
	lda	pl_ConvTab,y
	ENDIF ;PL_PACKED
	sta	pl_Temp2ZP
	jmp	plpv_skp4

plpv_skp3:
	lda	pl_Temp3ZP	;Acc=pl_Temp3ZP signextended with 2 bits.
	asl
	asl
	pha
	asl
	pla
	ror
	pha
	asl
	pla
	ror

	clc
	adc	pl_Temp2ZP
	sta	pl_Temp2ZP

plpv_skp4:		
	lda	pl_Temp2ZP	;Acc = EffectiveNote
	cpx	#$03			;Is this the noise channel?
	beq	plpv00_Noise
	tay
	lda	pl_FreqTab,y	;Freq
	beq	plpv_SetValue
plpv00_Noise:
	IFCONST PL_SUPPORT_PORTAMENTO
	clc
	adc	pl_Temp1ZP	;Add frequency offset
	ENDIF ;PL_SUPPORT_PORTAMENTO
	ora	#$80		;SetGate
	jmp	plpv_SetValue

	ENDIF ;PL_SUPPORT_ARPMODE00
	IFCONST PL_SUPPORT_ARPMODE10
plpv_ArpMode10:	
	lda	pl_Temp2ZP	;Acc = EffectiveNote
	cpx	#$03			;Is this the noise channel?
	beq	plpv10_Noise
	tay
	lda	pl_FreqTab,y	;Freq
	beq	plpv_SetValue
plpv10_Noise:
	;Acc = Effective note
	clc 	; note:	output value is only 7-bit, so no need to
		; sign extend here.

	adc	pl_Temp3ZP	;Add arp offset
	IFCONST PL_SUPPORT_PORTAMENTO
	clc
	adc	pl_Temp1ZP	;Add frequency offset
	ENDIF ;PL_SUPPORT_PORTAMENTO
	ora	#$80		;SetGate
	jmp	plpv_SetValue

	ENDIF ;PL_SUPPORT_ARPMODE10
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	IFCONST PL_SUPPORT_VOLVOICE
plpv_VolumeTempo:
;*** Handle Volume ***
	lda	pl_vd_EffectiveNote,x
	ENDIF ;PL_SUPPORT_VOLVOICE
plpv_SetVolume:
	and	#$0f
	sta	pl_Temp1ZP
	lda	$900e
	and	#$f0
	ora	pl_Temp1ZP
	sta	$900e
	rts

plpv_SetValue:
	IFNCONST PL_PACKED
	ldy	pl_Mute,x
	beq	plpv_skp2
	lda	#0
plpv_skp2:
	ENDIF ; !PL_PACKED
	sta	$900a,x
	rts
	
	IFCONST PL_SUPPORT_ARPEGGIOS
	IFCONST	PL_SUPPORT_ARPMODEF0
plpv_ArpModef0:
	IFCONST PL_SUPPORT_PORTAMENTO
;put FreqOffset in pl_Temp1ZP
	lda	pl_vd_FreqOffsetHigh,x
	sta	pl_Temp1ZP
	
	ENDIF ;PL_SUPPORT_PORTAMENTO
;Get note in ACC and jump
	lda	pl_vd_EffectiveNote,x
	and	#$7f
	cpx	#$03			;Is this the noise channel?
	beq	plpvf0_Noise

;*** Play Note ***
	IFCONST PL_SUPPORT_ARPEGGIOS
	clc
	adc	pl_vd_ArpOffset,x	;Add Arpeggio
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	tay			;y = EffectiveNote
	lda	pl_FreqTab,y	;Freq
	beq	plpv_SetValue
	IFCONST PL_SUPPORT_PORTAMENTO
	clc
	adc	pl_Temp1ZP
	ENDIF ;PL_SUPPORT_PORTAMENTO
	ora	#$80		;SetGate
	jmp	plpv_SetValue
plpvf0_Noise:
;*** Play Noise ***
	cmp	#$00
	beq	pplvf0n_skp1
	IFCONST PL_SUPPORT_ARPEGGIOS
	clc
	adc	pl_vd_ArpOffset,x	;Add arpeggio
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	IFCONST PL_SUPPORT_PORTAMENTO
	clc
	adc	pl_Temp1ZP
	ENDIF ;PL_SUPPORT_PORTAMENTO
	ora	#$80		;SetGate
pplvf0n_skp1:
	jmp	plpv_SetValue
	ENDIF ;PL_SUPPORT_ARPMODEF0
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	

;**************************************************************************
;*
;* PostUpdateVoice
;* X=Voice
;*
;******
pl_PostUpdateVoice:
	IFCONST PL_SUPPORT_PORTAMENTO
	jsr	pl_puv_UpdatePort
	ENDIF ;PL_SUPPORT_PORTAMENTO
	IFCONST PL_SUPPORT_ARPEGGIOS
	jsr	pl_puv_UpdateArp
	ENDIF ;PL_SUPPORT_ARPEGGIOS
	rts

	IFCONST PL_SUPPORT_PORTAMENTO
pl_puv_UpdatePort:
;*** Handle Portamento Effects ***
	IFCONST PL_SUPPORT_PORTEFFECT
	lda	pl_vd_ThisParam,x
	tay
	and	#$0f
	sta	pl_Temp1ZP
	tya
	and	#$f0

	cmp	#PL_PORTUP
	beq	pl_PortamentoUp
	cmp	#PL_PORTDOWN
	beq	pl_PortamentoDown
	cmp	#PL_PORTUPSLOW
	beq	pl_PortamentoUpSlow
	cmp	#PL_PORTDOWNSLOW
	beq	pl_PortamentoDownSlow

	ENDIF ;PL_SUPPORT_PORTEFFECT
	IFCONST PL_SUPPORT_PORTSOUND
	ldy	pl_vd_Sound,x	; Portamento enabled from the sound?
	lda	pl_Sounds+PL_SND_GLIDE,y
	beq	plpuv_skp2	;No, skip it!
	jmp	pl_DoPortamentoSigned	;Yes, do portamento.
plpuv_skp2:
	ENDIF ;PL_SUPPORT_PORTSOUND
	rts

;* run PORTUP *
pl_PortamentoUp:
	lda	#0
	jsr	pl_PreparePortamento
	jmp	pl_DoPortamentoUp

;* run PORTDOWN *
pl_PortamentoDown:
	lda	#0
	jsr	pl_PreparePortamento
	jmp	pl_DoPortamentoDown

;* run PORTUPSLOW *
pl_PortamentoUpSlow:
	jsr	pl_PreparePortamentoSlow
pl_DoPortamentoUp:
	lda	pl_vd_FreqOffsetLow,x
	clc
	adc	pl_Temp2ZP
	sta	pl_vd_FreqOffsetLow,x
	lda	pl_vd_FreqOffsetHigh,x
	adc	pl_Temp1ZP
	sta	pl_vd_FreqOffsetHigh,x
	rts

;* run PORTDOWNSLOW *
pl_PortamentoDownSlow:
	jsr	pl_PreparePortamentoSlow
pl_DoPortamentoDown:
	lda	pl_vd_FreqOffsetLow,x
	sec
	sbc	pl_Temp2ZP
	sta	pl_vd_FreqOffsetLow,x
	lda	pl_vd_FreqOffsetHigh,x
	sbc	pl_Temp1ZP
	sta	pl_vd_FreqOffsetHigh,x
	rts

pl_DoPortamentoSigned:	
	sta	pl_Temp1ZP
	bpl	pl_PortamentoUpSlow
	eor	#$ff
	clc
	adc	#1
	sta	pl_Temp1ZP
	jmp	pl_PortamentoDownSlow
	
pl_PreparePortamentoSlow:
	lda	#0
	lsr	pl_Temp1ZP
	ror
	lsr	pl_Temp1ZP
	ror
pl_PreparePortamento:
	lsr	pl_Temp1ZP
	ror
	sta	pl_Temp2ZP
	rts

	ENDIF ;PL_SUPPORT_PORTAMENTO
	IFCONST PL_SUPPORT_ARPEGGIOS
pl_puv_UpdateArp:
;*** Handle Arpeggio Effects ***
	IFCONST	PL_SUPPORT_ARPEFFECT
	lda	pl_vd_ThisParam,x
	tay
	and	#$0f
	sta	pl_Temp1ZP
	tya
	and	#$f0

	cmp	#PL_ARPEGGIO
	beq	pl_Arpeggio

	ENDIF ;PL_SUPPORT_ARPEFFECT
	IFCONST PL_SUPPORT_ARPSOUND
	ldy	pl_vd_Sound,x	;Arpeggio enabled from the sound?
	lda	pl_Sounds+PL_SND_ARPEGGIO,y
	bpl	plpuv_skp1	;No, skip it!
	and	#$0f
	sta	pl_Temp1ZP
	jmp	pl_Arpeggio	;Yes, do arpeggio.
plpuv_skp1:
	ENDIF ;PL_SUPPORT_ARPSOUND
	rts

;* run ARPEGGIO *
pl_Arpeggio:
	lda	pl_Temp1ZP
	pha
	tay
	lda	pl_ArpeggioIndex,y
	sta	pl_Temp1ZP	;pl_Temp1ZP=Arpnum*16
	pla
	asl
	tay			;y=Arpnum * 2
		
	lda	pl_ArpeggioConf,y
	and	#$0f		;Acc=Speed
	cmp	pl_vd_ArpCount,x
	bne	pl_arp_ex1
	lda	#0
	sta	pl_vd_ArpCount,x

	lda	pl_ArpeggioConf+1,y
	and	#$0f		;Acc=endstep
	cmp	pl_vd_ArpStep,x
	bne	pl_arp_skp1
	lda	pl_ArpeggioConf+1,y
	lsr
	lsr
	lsr
	lsr			;Acc=startstep
	sta	pl_vd_ArpStep,x
	jmp	pl_arp_skp2
pl_arp_skp1:
	lda	pl_vd_ArpStep,x
	clc
	adc	#1
	sta	pl_vd_ArpStep,x
pl_arp_skp2:
	lda	pl_vd_ArpStep,x
	clc
	adc	pl_Temp1ZP
	tay
	lda	pl_Arpeggios,y
	sta	pl_vd_ArpOffset,x
	rts
pl_arp_ex1:
	lda	pl_vd_ArpCount,x
	clc
	adc	#1
	sta	pl_vd_ArpCount,x
	rts	
	ENDIF ;PL_SUPPORT_ARPEGGIOS

	IFNCONST PL_VTCOMP
; eof
	ENDIF ;PL_VTCOMP
