



label1
	nop

label2
	nop


	db	label2 - label1
	db	label2-label1
	db	label2 - label1, label2 - label1
	db	label2-label1,label2-label1
	db	'Hello!'
	db	'Hello!', 'Hello!'
	db	'Hello!', 'Hello!', label2 - label1
	db	'Hello!','Hello!',label2-label1
label3
	db	'Hello!','Hello!',label2-label1
label4:
	db	'Hello!','Hello!',label2-label1
label5
	db	'Hello!','Hello!',label2-label1
	db	$, 1







