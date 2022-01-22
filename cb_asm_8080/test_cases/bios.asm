;
; $BeginLicense$
;
; $EndLicense$
;
;
; 	CP/M BIOS for cb_emu_8080.
;	Based on Skeletal CBIOS from CP/M manual.
;

;
; 	The IO registers.
;
LST	EQU	080H
TTY	EQU	040H
KBDSTAT EQU	041H
KBDDATA EQU	042H
BNKSWT0	EQU	0FDH	
BNKSWT1	EQU	0FEH	
BNKSWT2	EQU	0FFH	
DRIVE	EQU	000H
FSECTOR	EQU	DRIVE+1
FTRACKL	EQU	DRIVE+2
FTRACKH	EQU	DRIVE+3
FDCCMD	EQU	DRIVE+4
FDCSTA	EQU	DRIVE+5
FDMAL   EQU	DRIVE+6
FDMAH   EQU	DRIVE+7
;
;	CP/M system locations
;
MSIZE	EQU	64		; Memory in Kbytes.
BIAS	EQU	(MSIZE-20)*1024	; Offset from reference 20K system.
CCP	EQU	3400H+BIAS	; CCCP base.
BDOS	EQU	CCP+806H	; BDOS base.
BIOS	EQU	CCP+1600H	; BIOS base.
SIZE	EQU	1600H		; Size of CMP system. Without BIOS.
SECTS	EQU	SIZE/128	; Number of sectors to load.
;
CDISK	EQU	0004H		; Current disk. (last logged on warm start).
IOBYTE	EQU	0003H		; Interl I/O byte. (see CP/M spec).

;
;	Start of the stuff.
;
	ORG	BIOS
;
;	CP/M Jump vector table.
;
	JMP	BOOT		; Cold start.
WBOOTE: JMP	WBOOT		; Warm start.
	JMP	CONST		; Console status.
	JMP	CONIN		; Console character in.
	JMP	CONOUT		; Console character out.
	JMP	LIST		; List character out.
	JMP	PUNCH		; Punch character out.
	JMP	READER		; Reader character in.
	JMP	HOME		; Move head to home position.
	JMP	SELDSK		; Select disk.
	JMP	SETTRK		; Select track.
	JMP	SETSEC		; Select sector.
	JMP	SETDMA		; Set DMA address.
	JMP	READ		; Read disk.
	JMP	WRITE		; Write disk.
	JMP	LISTST		; Return list status.
	JMP	SECTRAN		; Sector translate.

;
;	Fixed data tables for 4 IBM-compatible 8" SD disks.
;	See CP/M Manual.
;

;	Disk parameter header for disk 00 : ibm-3740
DPBASE:	DW	TRANS,0000H
	DW	0000H,0000H
	DW	DIRBF,DPBLK
	DW	CHK00,ALL00
;	Disk parameter header for disk 01 : ibm-3740
	DW	TRANS,0000H
	DW	0000H,0000H
	DW	DIRBF,DPBLK
	DW	CHK01,ALL01
;	Disk parameter header for disk 02 : ibm-3740
	DW	TRANS,0000H
	DW	0000H,0000H
	DW	DIRBF,DPBLK
	DW	CHK02,ALL02
;	Disk parameter header for disk 03 : ibm-3740
	DW	TRANS,0000H
	DW	0000H,0000H
	DW	DIRBF,DPBLK
	DW	CHK03,ALL03
;	Disk parameter header for disk 04 : 4mb-hd
     	DW	0000H,0000H
	DW	0000H,0000H
	DW	DIRBF,HDBLK
	DW	CHK04,ALL04
;	Disk parameter header for disk 05 : 4mb-hd
     	DW	0000H,0000H
	DW	0000H,0000H
	DW	DIRBF,HDBLK
	DW	CHK05,ALL05

;
;	Sector translate vector for the IBM 8" SD disks.
;	Logical (0..25) to Physical.
;
TRANS:	DB	1,7,13,19
	DB	25,5,11,17
	DB	23,3,9,15
	DB	21,2,8,14
	DB	20,26,6,12
	DB	18,24,4,10
	DB	16,22	
;
;	Disk parameter block, common to all ibm-3740 8" SD disks.
;       128 bytes/sector. 26 sectors/track. 77 tracks. 2 reserved.
;
DPBLK:  DW	26		; Sectors per track.
	DB	3		; Block shift factor.
	DB	7		; Block mask.
	DB	0		; Extent mask.
	DW	242		; Disk size-1 : ((77-2)*26*128/1024)-1
	DW	63		; Directory max
	DB	192		; Alloc 0 : 2 bits => 32*2 direntries.(1K BLS)
	DB	0		; Alloc 1
	DW	16		; Check size
	DW	2		; Track offset
;
;       Disk parameter block, common to all 4mb-hd.
;	128 bytes/sector. 32 sectors/track. 1024 tracks. No reserved.
;
HDBLK:  DW	32		; Sectors per track.
	DB	4		; Block shift factor.
	DB	15		; Block mask.
	DB	0		; Extent mask.
	DW	2047		; Disk size-1 : 1024*32*128/2048 - 1
	DW	1023		; Directory max.
	DB	255		; Alloc 0 : 16 bits => 64*16 direntries.(2K BLS)
	DB	255		; Alloc 1
	DW	0		; Check size.
	DW	0		; Track offset.
;
;	Message strings.
;
SIGNON: DB	'64K CP/M Vers. 2.2 (CBIOS V1.2a for cb_emu_8080) ',0DH,0AH,0
LDERR:	DB	'BIOS: error booting',0DH,0AH,0
;
;	Print null terminated string.
;	Pointed to by HL.
;
PRTMSG:	MOV	A,M
	ORA	A
	RZ			; NULL terminated
	MOV	C,A
	CALL	CONOUT		; CONOUT expects in C.
	INX	H
	JMP	PRTMSG

;
;	Here we go ...
;
BOOT:   LXI	SP,80H		; Use space below buffer for stack.
	LXI	H,SIGNON	; Signon message.
	CALL	PRTMSG
	XRA	A
	STA	IOBYTE		; Clear iobyte.
	STA	CDISK		; Select disk 0.
	; Clear 8H to 0080H
	LXI	H,8		; Clear 8->127.
	MVI	C,119
CLR:	MOV 	M,A
	INX	H 
	DCR	C
	JNZ	CLR
	; Set a HALT instruction on each of the 7 restart vectors.
	MVI	A,76H		; HALT Opcode.
	STA	08H
	STA	10H
	STA	18H
	STA	20H
	STA	28H
	STA	30H
	STA	38H
	JMP	GOCPM		; Initialize and go to CP/M
;
;	Warm boot : Read the disk until all sectors loaded
;
WBOOT:  LXI	SP,80H		; Use space below buffer for stack.
	MVI	C,0		; Select disk 0.
	CALL	SELDSK
	CALL	HOME		; Go to track 00
	MVI	B,SECTS		; B : Nr sectors to load.
	MVI	C,0		; C : Current track. Starting 0.
	MVI	D,2		; D : Next sector. Starting 2. (1 = bootrecord)
	LXI	H,CCP		; Base of CP/M.
LOAD1:	; Load one more sector.
	PUSH	B 
	PUSH	D 
	PUSH	H 
	MOV	C,D	
	CALL	SETSEC		; Set sector address from register c
	POP	B 		; Recall DMA address to BC
	PUSH	B 		; Replace on stack for later recall
	CALL	SETDMA		; Set DMA address from BC
	CALL	READ		; Read a sector.
	ORA	A
	JZ	LOAD2		; NZ => Error !
	; Error branch : message and halt.
	LXI	H,LDERR	
	CALL	PRTMSG
	DI		
	HLT
	; No error branch : move to next sector.
LOAD2:	POP	H 		; HL : DMA
	LXI	D,128		;
	DAD	D   		; DMA += 128 in HL
	POP	D 		; D : Sector address.
	POP	B 		; BC : Nr Sectors, Current track.
	DCR	B		; Sectors--
	JZ	GOCPM		; Transfer to CP/M if all loaded.
	; More sectors to load.
	INR	D
	MOV	A,D		; Sector=27 ?
	CPI	27
	JC	LOAD1		; Carry generated if sector<27
	; Next track.
	MVI	D,1		; Sector = 1
	INR	C		; Track++
     	PUSH	B 		; We need to remember B (loop counter)
        MVI	B,0		; While B must be 0 as BC is the full track add.
	CALL	SETTRK		; Set track from C
        POP 	B 
	JMP	LOAD1		
;
;	End of load operation.
;
;	Set parameters and go to CP/M
;
GOCPM:
	; Construct JMP WBOOT at 0.
	MVI	A, 0C3H		; C3H = JMP
	STA	0
	LXI	H, WBOOTE
	SHLD	1	
	; Construct JMP BDOS at 5.
	STA	5	
	LXI	H, BDOS	
	SHLD	6	
	; Set default DMA address on 80H
	LXI	B, 80H		;DEFAULT DMA ADDRESS IS 80H
	CALL	SETDMA
	; Enable interrupt and go to CP/M's CCP for further processing.
	EI	
	LDA	CDISK
	MOV	C, A		; Have Current disk number in C.
	JMP	CCP	

;
;	Simple I/O handlers.
;
;
;	Console status : Return 0FFH (character ready) or 00H
;
CONST:	IN	KBDSTAT
	RET
;
;	Console character into register A
;
CONIN:	IN	KBDSTAT
	ORA	A
	JZ	CONIN
	IN	KBDDATA
	RET
;
;	Console character output from register C
;
CONOUT: MOV	A,C
	OUT	TTY
	RET
;
;	List character from register C
;
LIST:	MOV	A,C
        OUT     LST
	RET
;
;	Return list status (00h if not ready, 0ffh if ready)
;
LISTST: 
	MVI	A,0FFH		; Always ready.
	RET
;
;	Punch character from register C
;	XXX CB TODO
;
PUNCH:	MOV	A,C
	;OUT	AUXDAT
	RET
;
;	Read character into register A from reader device
;	XXX CB TODO
;
READER: 
	;IN	AUXDAT
	RET
;
;
;	I/O drivers for the disk.i/o drivers for the disk follow
;
HOME:	LXI	B,0		; Select track 0.
	JMP	SETTRK		; We will move to 00 on first read/write.
;
;	Select disk given by register C
;

SELDSK:	
	LXI	H, 0000H	; Error return code.
	MOV	A, C
	STA	DISKNO
	CPI	6		; Must be between 0 and 5.
	RNC			; NC if 4, 5,...
	; Disk number in range.
	OUT	DRIVE
	; Compute proper disk parameter header address.
	MOV	L, A		; L = Disk number 0, 1, 2, 3
	MVI	H, 0
	DAD	H		; *2
	DAD	H		; *4
	DAD	H		; *8
	DAD	H		; *16 (Size of each header)
	LXI	D, DPBASE
	DAD	D		; HL=DPBASE+(DISKNO*16)
	RET
;
;	Set track given by register C
;
SETTRK: MOV	A,C
	STA	TRACK
	OUT	FTRACKL
        MOV	A,B
	STA	TRACK+1
	OUT	FTRACKH
	RET
;
;	Set sector given by register C
;
SETSEC: MOV	A,C
	STA	SECTOR
	OUT	FSECTOR
	RET
;
;	Translate the sector given by BC using the
;	translate table given by DE. (DE=0 => No translation)
;
SECTRAN:
	MOV	A,D		
	ORA	D
	JNZ	SECT1
	MOV	A,E
	ORA	E
	JNZ	SECT1
	; Return untranslated branch.
	MOV	L,C
	MOV	H,B
	INR	L		; Logical 0 => Physical 1.
	RNZ
	INR	H
	RET
	; Return translated branch.
SECT1:	XCHG			; HL => Translate.
	DAD	B		; HL += Sector
	MOV	L, M		; Look it up.
	MVI	H, 0		; HL => Translated sector.
	RET		
;
;	Set DMA address given by registers BC
;
SETDMA:	MOV	L, C		
	MOV	H, B	
	SHLD	DMAAD
	MOV	A,L
	OUT	FDMAL
	MOV	A,H
	OUT	FDMAH
	RET
;
;	Perform read operation.
;
READ:	XRA	A		; 0 is read command.
	JMP	WAITIO	
;
;	Perform a write operation.
;
WRITE:	MVI	A,1		; 1 is write command.
;
;	Enter here from read and write to perform the actual i/o
;	operation.  return a 00h in register a if the operation completes
;	properly, and 01h if an error occurs during the read or write
;
WAITIO: OUT	FDCCMD
WAIT:	IN	FDCCMD
	ORA	A		; Ready when not 0.
	JZ	WAIT
	IN	FDCSTA		; Status of i/o operation -> A
	RET

;
;	The remainder of the CBIOS is reserved unitialized
;	DATA area ,and does not need to be a part of the
;	system memory image (the space must be available,
;	however, between "BEGDAT" and "ENDDAT").
;

TRACK:	DS	2
SECTOR:	DS	2
DMAAD:	DS	2
DISKNO:	DS	1
;
;	SCRATCH RAM AREA FOR BDOS USE
BEGDAT	EQU	$
DIRBF:	DS	128	 	; Scratch directory area.
ALL00:	DS	31	 	; Allocation vectors 0.
ALL01:	DS	31	 	; Allocation vectors 1.
ALL02:	DS	31	 	; Allocation vectors 2.
ALL03:	DS	31	 	; Allocation vectors 3.
ALL04:	DS	255 		; Allocation vectors 3.
ALL05:	DS	255	 	; Allocation vectors 3.
CHK00:	DS	16		; Check vector 0.
CHK01:	DS	16		; Check vector 1.
CHK02:	DS	16		; Check vector 2.
CHK03:	DS	16		; Check vector 3.
CHK04:	DS	0       	; Check vector 4.	// Not needed for fixed.
CHK05:	DS	0       	; Check vector 5.	// Not needed for fixed.
;
ENDDAT	EQU	$	 
DATSIZ	EQU	$-BEGDAT;	; Size of data area.
	END

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; vim: syntax=asm ts=8 sw=8 sts=8 sr et columns=100 lines=45
