;******************************************************************************
;Filename:	test_dw_1.asm
;Description:	"DW" assembler directive test.
;Copyright(c):
;Author(s):	Claude Sylvain
;Created:	December 2010
;Last modified:	6 January 2012
;******************************************************************************


label1
	nop

label2
	nop


	dw	label2
	dw	label2 - label1
	dw	label2-label1
	dw	label2 - label1, label2 - label1
	dw	label2-label1,label2-label1

	;Each characters pair must appear as a Little Endian word.
	;---------------------------------------------------------
	dw	'ABCD'
	dw	'ABCD'

	;Single character must appear as LSB on a Little Endian word.
	;------------------------------------------------------------
	dw	'A'
	dw	'A'
	dw	'ABC'
	dw	'ABC'

	dw	'Hello!'
	dw	'Hello!', 'Hello!'
	dw	'Hello!', 'Hello!', label2 - label1
	dw	'Hello!','Hello!',label2-label1
label3
	dw	'Hello!','Hello!',label2-label1
label4:
	dw	'Hello!','Hello!',label2-label1
label5
	dw	'Hello!','Hello!',label2-label1
	dw	$, 1






