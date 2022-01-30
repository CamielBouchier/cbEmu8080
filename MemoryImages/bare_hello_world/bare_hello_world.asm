;
; $BeginLicense$
;
; $EndLicense$

;
; This bootrom does :
;
; - Provide CALL5 , FN 2 and 9 support for prompting to console.
; - Jumps to 100H for (COM area) to run a hello world.
; 

BOOT	EQU	0000H		; Really boot location.
CALL5	EQU	0005H
COM     EQU     0100H
STACK   EQU     1FFFH		; There we have RAM that can grow down.
; The IO registers.
TTY	EQU	040H
;
; Start of the stuff
;
	ORG	BOOT
        JMP	0100H

        ORG	CALL5
        PUSH    B
	PUSH	D
	PUSH	H
	PUSH	PSW
	MOV	A,C
	CPI	9
	JZ	FN9
	CPI	2
	JZ	FN2
	JMP	RT1
FN9:	LDAX	D
	CPI	'$'
	JZ	RT1
	OUT	TTY
	INX	D
	JMP	FN9
FN2:	MOV	A,E
	OUT	TTY
RT1:	
	POP	PSW
	POP	H
	POP	D
	POP	B
	RET
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 	COM
	MVI	C,09H	; print string in BDOS
	LXI	D,HW
	CALL	CALL5
	HLT	

HW:	DB	'Hello world !', 0DH, 0AH , '$'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=80 lines=45 fileencoding=utf-8
