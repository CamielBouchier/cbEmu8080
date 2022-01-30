

;- Ref.: 8080-8085_assembly_language_programming__1977__intel__pdf_.pdf, 
;  page 185.


dsubminustart	ds	8
dsubminu:		equ	$ - 1

dsubsbtrastart	ds	8
dsubsbtra		equ	$ - 1


dsub
	lxi	d, dsubminu
	lxi	h, dsubsbtra
	mvi	c, 8
	stc

dsub00
	mvi	a, 99H
	aci	0
	sub	m
	xchg
	add	m
	daa
	mov	m, a
	xchg
	dcr	c
	jz	dsub01

	inx	d
	inx	h
	jmp	dsub00

dsub01
	nop






