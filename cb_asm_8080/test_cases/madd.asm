

;- Ref.: 8080-8085_assembly_language_programming__1977__intel__pdf_.pdf, 
;  page 180.


madd:
	lxi	b, madd02
	lxi	h, madd03
	xra	a

madd00:
	ldax	b
	adc	m
	stax	b
	dcr	e
	jz	madd01

	inx	b
	inx	h
	jmp	madd00
madd01:


madd02:
	db	90H
	db	0BAH
	db	84H

madd03:
	db	8AH
	db	0AFH
	db	32H




