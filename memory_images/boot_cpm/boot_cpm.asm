;
; $BeginLicense$
;
; $EndLicense$
;

;
; This boot image does :
;
; - Display a welcome.
; - Read sector 0 and 1 of the drive 0 to address 0.
;   (The first 512 bytes are typically the boot code)
; - Jump to 0 for the boot.
; - Short user interaction in case of failure.
; 

BOOT	EQU	0000H		; Really boot location.
TPA	EQU	0100H		
STACK   EQU     1FFFH		; There we have RAM that can grow down.
; The IO registers.
TTY	EQU	040H
KBDSTAT EQU	041H
KBDDATA EQU	042H
DRIVE	EQU	000H
SECTOR	EQU	DRIVE+1
TRACKL	EQU	DRIVE+2
TRACKH	EQU	DRIVE+3
FDCCMD	EQU	DRIVE+4
FDCSTA	EQU	DRIVE+5
DMAL   	EQU	DRIVE+6
DMAH   	EQU	DRIVE+7
;
; Start of the stuff
;
	ORG	BOOT
	JMP	TPA
;
; We work from TPA because otherwise we overwrite ourselves !
;
	ORG 	TPA
;
;	Init stack.
;
	LXI     SP,STACK
;
;	Print welcome (or error) message and wait for key
;
	LXI	H,MSGHI		; Print welcome (or error) message.
LOOP1:	CALL	PRTSCR		
	CALL	WTKEY		
	LXI	H,MSGBUSY	; Print that we will start on it !
	CALL 	PRTSCR
;
;	Load the boot record : First sector.
;
        LXI  	H,0     	; HL => Destination.
        MVI  	B,0         	; B => Track, starting 0.
        MVI  	C,1    		; C => Sector, starting 1.
        CALL	SECRD	     	; Read sector.
	JZ 	SCCS		; Zero => success.
	LXI	H,MSGER
	JMP	LOOP1		; Retry in case of failure.
;
;	Show success message.
;
SCCS:	LXI	H,MSGSC		
	CALL	PRTSCR
	JMP	BOOT
;
; 	Sub : Wait for key.
;
WTKEY:	IN	KBDSTAT
	CPI	00
	JZ	WTKEY
EAT:	IN	KBDDATA
	IN	KBDSTAT
	CPI	00
	JNZ	EAT
	RET
;
; 	Sub : Print null terminated to screen.
;	HL  : Pointer to message.
;
PRTSCR:	MOV     A,M		; Remember MOV doesn't set flags.
        CPI	0
	RZ			; NULL terminated.
	OUT	TTY
	INX	H
	JMP	PRTSCR
;
; 	Sub : Read sector from drive 0.
;	B   : Track
;       C   : Sector
;	HL  : Destination
;
SECRD:	PUSH 	B           	; BC/HL needs preservation.
        PUSH 	H  
	XRA 	A
        OUT 	DRIVE
	OUT 	TRACKH
        MOV 	A,B
        OUT 	TRACKL
 	MOV 	A,C
	OUT 	SECTOR
        MOV 	A,L
        OUT 	DMAL
	MOV 	A,H
	OUT 	DMAH
	XRA 	A
        OUT 	FDCCMD
	IN  	FDCSTA
	CPI 	0		; NZ (returned) is error.
	POP  	H           	; Restore BC/HL
        POP  	B
        RET  
;
; 	Message strings
;
MSGHI	DB	'cb_emu_8080',0DH,0AH
MSGC0	DB	'(C) 2017-2022 Camiel Bouchier <camiel@bouchier.be>',0DH,0AH
MSGC1	DB	'Please make sure the bootdisk is in drive 0',0DH,0AH
MSGC2	DB	'Press any key to continue',0DH,0AH,0
;
MSGER	DB	'Could not boot from drive 0. Disk inserted ?',0DH,0AH
MSGC3	DB	'Print any key to retry',0DH,0AH,0
;
MSGSC	DB	'Boot sector from drive 0 loaded. Switching to it.',0DH,0AH,0
;
MSGBUSY	DB	'Loading boot sector from drive 0.',0DH,0AH,0
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=80 lines=45 fileencoding=utf-8
