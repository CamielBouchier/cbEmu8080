;
; $BeginLicense$
;
; $EndLicense$
;

;
; This bootrecord does load the system from disk and jumps on success to it.
;
BOOT	EQU	0000H		; Really boot location.
STACK   EQU     1FFFH		; There we have RAM that can grow down.
;
; 	The IO registers.
;
TTY     EQU	040H
KBDSTAT EQU	041H
KBDDATA EQU	042H
BNKSWT0	EQU	0FDH	
BNKSWT1	EQU	0FEH	
BNKSWT2	EQU	0FFH	
DRIVE	EQU	000H
SECTOR	EQU	DRIVE+1
TRACKL	EQU	DRIVE+2
TRACKH	EQU	DRIVE+3
FDCCMD	EQU	DRIVE+4
FDCSTA	EQU	DRIVE+5
DMAL   	EQU	DRIVE+6
DMAH   	EQU	DRIVE+7
;
;	CP/M system locations
;
MSIZE	EQU	64		; Memory in Kbytes.
BIAS	EQU	(MSIZE-20)*1024	; Offset from reference 20K system.
CCP	EQU	3400H+BIAS	; CCCP base.
BIOS	EQU	CCP+1600H	; BIOS base.
SIZE	EQU	1900H		; Size of CMP system. (adding 300H bios).
SECTS	EQU	SIZE/128	; Number of sectors to load.
;
; Start of the stuff
;
	ORG	BOOT
;
                                ; Sector is starting with 1 ( no sector 0 ! )
	LXI	B,2	        ; B,C = track,sector = 0,2.
	MVI	D,SECTS		; D = Number of sectors to load.
	LXI	H,CCP		; HL = CCCP base.
;
;       Read track code.
;
        XRA 	A
        OUT 	DRIVE
	OUT 	TRACKH
RDSEC:  MOV 	A,B
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
        JNZ	ERROR
	DCR	D		; Decrement sectors to load.
	JZ	SCCS		; Jump into BIOS when all sectors read.
        ; Next sector.
	LXI	SP,128		; Use SP for HL+=128
	DAD	SP		; HL += 128
	INR	C		; Sector++
	MOV 	A,C
	CPI	27
	JC	RDSEC		; 26 sectors per track. Same track as long C.
	; Next track. 
	MVI	C,1		; Sector = 1
	INR	B		; Track++
	JP	RDSEC
;
; 	Error message.
;
ERROR:	LXI	H,MSGER
PRTSCR:	MOV	A,M
	CPI	0
	JZ	STOP		; Null terminated.
	OUT	TTY
	INX	H
	JMP	PRTSCR
;
;	Stop processor on fail.
;
STOP:	DI
	HLT
;
;	On success jump to the BIOS.
;
SCCS:	JMP	BIOS
;
;	Message strings.
;
MSGER	DB	'Boot failed.',0DH,0AH,0
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=80 lines=45 fileencoding=utf-8
