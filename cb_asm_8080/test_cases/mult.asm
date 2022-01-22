

;- Ref.: 8080-8085_assembly_language_programming__1977__intel__pdf_.pdf, 
;  page 177.


mult:	mvi	b, 0
	mvi	e, 9

mult00:
	mov	a, c
	rar
	mov	c, a
	dcr	e
	jz	multend
	mov	a, b
	jnc	mult01
	add	d

mult01:
	rar
	mov	b, a
	jmp	mult00

multend:
	ret






